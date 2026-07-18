#include "H264Helper.h"
#include <fstream>
#include <string>
#include <codecapi.h>

static void Log(const std::string& msg) {
    static std::ofstream ofs("h264_debug.log", std::ios::app);
    ofs << GetTickCount() << " " << msg << std::endl;
}

ID3D11Device* g_pD3D11Device = NULL;
ID3D11DeviceContext* g_pD3D11Context = NULL;
IMFDXGIDeviceManager* g_pDXGIManager = NULL;
UINT g_resetToken = 0;

bool InitHardwareAcceleration() {
    UINT creationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &g_pD3D11Device, &featureLevel, &g_pD3D11Context);
    
    if (SUCCEEDED(hr)) {
        ID3D10Multithread* pMultithread = NULL;
        if (SUCCEEDED(g_pD3D11Device->QueryInterface(IID_PPV_ARGS(&pMultithread)))) {
            pMultithread->SetMultithreadProtected(TRUE);
            pMultithread->Release();
        }
        
        hr = MFCreateDXGIDeviceManager(&g_resetToken, &g_pDXGIManager);
        if (SUCCEEDED(hr)) {
            hr = g_pDXGIManager->ResetDevice(g_pD3D11Device, g_resetToken);
            if (SUCCEEDED(hr)) {
                Log("Hardware Acceleration Initialized successfully.");
                return true;
            }
        }
    }
    Log("Hardware Acceleration Initialization failed. hr=" + std::to_string(hr));
    CleanupHardwareAcceleration();
    return false;
}

void CleanupHardwareAcceleration() {
    if (g_pDXGIManager) { g_pDXGIManager->Release(); g_pDXGIManager = NULL; }
    if (g_pD3D11Context) { g_pD3D11Context->Release(); g_pD3D11Context = NULL; }
    if (g_pD3D11Device) { g_pD3D11Device->Release(); g_pD3D11Device = NULL; }
}

bool IsHardwareAccelerationEnabled() {
    return g_pDXGIManager != NULL;
}

H264Encoder::H264Encoder() {}
H264Encoder::~H264Encoder() {
    if (pEncoder) pEncoder->Release();
    if (pConverter) pConverter->Release();
}

bool H264Encoder::Initialize(UINT32 width, UINT32 height) {
    Log("H264Encoder::Initialize");
    if (pEncoder) pEncoder->Release();
    if (pConverter) pConverter->Release();
    pEncoder = NULL;
    pConverter = NULL;
    m_width = width;
    m_height = height;
    m_rtStart = 0;

    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEncoder));
    if (FAILED(hr)) return false;

    if (g_pDXGIManager) {
        pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)g_pDXGIManager);
    }

    IMFAttributes* pAttributes = NULL;
    if (SUCCEEDED(pEncoder->GetAttributes(&pAttributes))) {
        pAttributes->SetUINT32(MF_LOW_LATENCY, 1);
        pAttributes->Release();
    }

    IMFMediaType* pOutputType = NULL;
    MFCreateMediaType(&pOutputType);
    pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    pOutputType->SetUINT32(MF_MT_AVG_BITRATE, 1000000); 
    MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pOutputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    pOutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    
    hr = pEncoder->SetOutputType(0, pOutputType, 0);
    pOutputType->Release();
    if (FAILED(hr)) return false;

    ICodecAPI* pCodecAPI = NULL;
    if (SUCCEEDED(pEncoder->QueryInterface(IID_PPV_ARGS(&pCodecAPI)))) {
        VARIANT var;
        VariantInit(&var);
        
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        pCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);
        
        var.vt = VT_UI4;
        var.ulVal = 0;
        pCodecAPI->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &var);
        
        var.vt = VT_UI4;
        var.ulVal = eAVEncCommonRateControlMode_CBR;
        pCodecAPI->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var);
        
        pCodecAPI->Release();
    }

    IMFMediaType* pInputType = NULL;
    MFCreateMediaType(&pInputType);
    pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    pInputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    
    hr = pEncoder->SetInputType(0, pInputType, 0);
    pInputType->Release();
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pConverter));
    if (FAILED(hr)) return false;

    if (g_pDXGIManager) {
        pConverter->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)g_pDXGIManager);
    }

    IMFMediaType* pConvInputType = NULL;
    MFCreateMediaType(&pConvInputType);
    pConvInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    MFSetAttributeSize(pConvInputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetInputType(0, pConvInputType, 0);
    pConvInputType->Release();
    if (FAILED(hr)) return false;

    IMFMediaType* pConvOutputType = NULL;
    MFCreateMediaType(&pConvOutputType);
    pConvOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(pConvOutputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetOutputType(0, pConvOutputType, 0);
    pConvOutputType->Release();
    if (FAILED(hr)) return false;

    pEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
    pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
    pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
    
    pConverter->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
    pConverter->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
    pConverter->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
    return true;
}

bool H264Encoder::EncodeFrame(const BYTE* pRgb32Data, DWORD cbData, std::vector<uint8_t>& outH264) {
    if (!pEncoder || !pConverter) return false;
    outH264.clear();
    
    IMFSample* pSample = NULL;
    MFCreateSample(&pSample);
    
    IMFMediaBuffer* pBuffer = NULL;
    MFCreateMemoryBuffer(cbData, &pBuffer);
    
    BYTE* pData = NULL;
    pBuffer->Lock(&pData, NULL, NULL);
    memcpy(pData, pRgb32Data, cbData);
    pBuffer->Unlock();
    pBuffer->SetCurrentLength(cbData);
    
    pSample->AddBuffer(pBuffer);
    pBuffer->Release();
    pSample->SetSampleTime(m_rtStart);
    pSample->SetSampleDuration(333333);
    m_rtStart += 333333;
    
    HRESULT hr = pConverter->ProcessInput(0, pSample, 0);
    pSample->Release();
    if (FAILED(hr)) { return false; }

    while (true) {
        MFT_OUTPUT_DATA_BUFFER convOutput = {0};
        convOutput.dwStreamID = 0;
        
        MFT_OUTPUT_STREAM_INFO convInfo = {0};
        pConverter->GetOutputStreamInfo(0, &convInfo);
        bool convProvides = (convInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

        IMFSample* pConvSample = NULL;
        IMFMediaBuffer* pConvBuffer = NULL;
        if (!convProvides) {
            MFCreateSample(&pConvSample);
            MFCreateMemoryBuffer(m_width * m_height * 2, &pConvBuffer); // NV12
            pConvSample->AddBuffer(pConvBuffer);
            pConvBuffer->Release();
            convOutput.pSample = pConvSample;
        }

        DWORD status = 0;
        hr = pConverter->ProcessOutput(0, 1, &convOutput, &status);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            if (pConvSample) pConvSample->Release();
            break;
        }
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            if (pConvSample) pConvSample->Release();
            IMFMediaType* pNewType = NULL;
            if (SUCCEEDED(pConverter->GetOutputAvailableType(0, 0, &pNewType))) {
                pConverter->SetOutputType(0, pNewType, 0);
                pNewType->Release();
            }
            continue;
        }
        
        if (SUCCEEDED(hr) && convOutput.pSample) {
            hr = pEncoder->ProcessInput(0, convOutput.pSample, 0);
            if (SUCCEEDED(hr)) {
                while (true) {
                    MFT_OUTPUT_DATA_BUFFER encOutput = {0};
                    encOutput.dwStreamID = 0;
                    
                    MFT_OUTPUT_STREAM_INFO encInfo = {0};
                    pEncoder->GetOutputStreamInfo(0, &encInfo);
                    bool encProvides = (encInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

                    IMFSample* pEncSample = NULL;
                    IMFMediaBuffer* pEncBuffer = NULL;
                    if (!encProvides) {
                        MFCreateSample(&pEncSample);
                        MFCreateMemoryBuffer(m_width * m_height * 4, &pEncBuffer); // H264
                        pEncSample->AddBuffer(pEncBuffer);
                        pEncBuffer->Release();
                        encOutput.pSample = pEncSample;
                    }

                    DWORD encStatus = 0;
                    hr = pEncoder->ProcessOutput(0, 1, &encOutput, &encStatus);
                    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
                        if (pEncSample) pEncSample->Release();
                        break;
                    }
                    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
                        if (pEncSample) pEncSample->Release();
                        IMFMediaType* pNewType = NULL;
                        if (SUCCEEDED(pEncoder->GetOutputAvailableType(0, 0, &pNewType))) {
                            pEncoder->SetOutputType(0, pNewType, 0);
                            pNewType->Release();
                        }
                        continue;
                    }
                    
                    if (SUCCEEDED(hr) && encOutput.pSample) {
                        IMFMediaBuffer* pResBuffer = NULL;
                        encOutput.pSample->ConvertToContiguousBuffer(&pResBuffer);
                        BYTE* pResData = NULL;
                        DWORD cbResData = 0;
                        pResBuffer->Lock(&pResData, NULL, &cbResData);
                        
                        size_t oldSize = outH264.size();
                        outH264.resize(oldSize + cbResData);
                        memcpy(outH264.data() + oldSize, pResData, cbResData);
                        
                        pResBuffer->Unlock();
                        pResBuffer->Release();
                        encOutput.pSample->Release();
                    } else {
                        if (encOutput.pSample) encOutput.pSample->Release();
                        if (pEncSample) pEncSample->Release();
                        break;
                    }
                    if (encOutput.pEvents) encOutput.pEvents->Release();
                }
            }
            convOutput.pSample->Release();
        } else {
            if (convOutput.pSample) convOutput.pSample->Release();
            if (pConvSample) pConvSample->Release();
            break;
        }
        if (convOutput.pEvents) convOutput.pEvents->Release();
    }
    
    return !outH264.empty();
}

H264Decoder::H264Decoder() {}
H264Decoder::~H264Decoder() {
    if (pDecoder) pDecoder->Release();
    if (pConverter) pConverter->Release();
}

bool H264Decoder::Initialize(UINT32 width, UINT32 height) {
    Log("H264Decoder::Initialize");
    if (pDecoder) pDecoder->Release();
    if (pConverter) pConverter->Release();
    pDecoder = NULL;
    pConverter = NULL;
    m_width = width;
    m_height = height;
    m_rtStart = 0;

    HRESULT hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDecoder));
    if (FAILED(hr)) return false;

    if (g_pDXGIManager) {
        pDecoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)g_pDXGIManager);
    }

    IMFMediaType* pDecInputType = NULL;
    MFCreateMediaType(&pDecInputType);
    pDecInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pDecInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(pDecInputType, MF_MT_FRAME_SIZE, width, height);
    hr = pDecoder->SetInputType(0, pDecInputType, 0);
    pDecInputType->Release();
    if (FAILED(hr)) return false;

    ICodecAPI* pDecCodecAPI = NULL;
    if (SUCCEEDED(pDecoder->QueryInterface(IID_PPV_ARGS(&pDecCodecAPI)))) {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        pDecCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);
        
        if (!g_pDXGIManager) {
            var.boolVal = VARIANT_FALSE;
            pDecCodecAPI->SetValue(&CODECAPI_AVDecVideoAcceleration_H264, &var);
        }
        
        pDecCodecAPI->Release();
    }


    IMFMediaType* pDecOutputType = NULL;
    MFCreateMediaType(&pDecOutputType);
    pDecOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pDecOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(pDecOutputType, MF_MT_FRAME_SIZE, width, height);
    hr = pDecoder->SetOutputType(0, pDecOutputType, 0);
    pDecOutputType->Release();
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pConverter));
    if (FAILED(hr)) return false;

    if (g_pDXGIManager) {
        pConverter->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)g_pDXGIManager);
    }

    IMFMediaType* pConvInputType = NULL;
    MFCreateMediaType(&pConvInputType);
    pConvInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(pConvInputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetInputType(0, pConvInputType, 0);
    pConvInputType->Release();
    if (FAILED(hr)) return false;

    IMFMediaType* pConvOutputType = NULL;
    MFCreateMediaType(&pConvOutputType);
    pConvOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    MFSetAttributeSize(pConvOutputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetOutputType(0, pConvOutputType, 0);
    pConvOutputType->Release();
    if (FAILED(hr)) return false;

    pDecoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
    pDecoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
    pDecoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);

    pConverter->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
    pConverter->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
    pConverter->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
    
    return true;
}

bool H264Decoder::DecodeFrame(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRgb32) {
    if (!pDecoder || !pConverter || inH264.empty()) return false;
    Log("DecodeFrame START size=" + std::to_string(inH264.size()));
    
    IMFSample* pSample = NULL;
    MFCreateSample(&pSample);
    
    IMFMediaBuffer* pBuffer = NULL;
    MFCreateMemoryBuffer(inH264.size(), &pBuffer);
    
    BYTE* pData = NULL;
    pBuffer->Lock(&pData, NULL, NULL);
    memcpy(pData, inH264.data(), inH264.size());
    pBuffer->Unlock();
    pBuffer->SetCurrentLength(inH264.size());
    
    pSample->AddBuffer(pBuffer);
    pBuffer->Release();
    pSample->SetSampleTime(m_rtStart);
    pSample->SetSampleDuration(333333);
    m_rtStart += 333333;
    
    HRESULT hr = pDecoder->ProcessInput(0, pSample, 0);
    pSample->Release();
    
    if (FAILED(hr)) Log("Dec ProcessInput FAILED hr=" + std::to_string(hr));
    
    outRgb32.clear();
    
    while (true) {
        MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {0};
        outputDataBuffer.dwStreamID = 0;
        
        MFT_OUTPUT_STREAM_INFO decInfo = {0};
        pDecoder->GetOutputStreamInfo(0, &decInfo);
        bool decProvides = (decInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

        IMFSample* pOutSample = NULL;
        IMFMediaBuffer* pOutBuffer = NULL;
        if (!decProvides) {
            MFCreateSample(&pOutSample);
            DWORD cbSize = decInfo.cbSize > 0 ? decInfo.cbSize : (m_width * m_height * 2);
            DWORD cbAlignment = decInfo.cbAlignment > 0 ? decInfo.cbAlignment : MF_16_BYTE_ALIGNMENT;
            MFCreateAlignedMemoryBuffer(cbSize, cbAlignment, &pOutBuffer);
            pOutSample->AddBuffer(pOutBuffer);
            pOutBuffer->Release();
            outputDataBuffer.pSample = pOutSample;
        }
        
        DWORD status = 0;
        hr = pDecoder->ProcessOutput(0, 1, &outputDataBuffer, &status);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            if (pOutSample) pOutSample->Release();
            break;
        }
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            Log("Dec ProcessOutput STREAM_CHANGE");
            if (pOutSample) pOutSample->Release();
            IMFMediaType* pNewType = NULL;
            if (SUCCEEDED(pDecoder->GetOutputAvailableType(0, 0, &pNewType))) {
                pDecoder->SetOutputType(0, pNewType, 0);
                pConverter->SetInputType(0, pNewType, 0); // UPDATE CONVERTER TOO!
                pNewType->Release();
            }
            continue;
        }
        
        if (FAILED(hr)) Log("Dec ProcessOutput FAILED hr=" + std::to_string(hr));
        
        if (SUCCEEDED(hr) && outputDataBuffer.pSample) {
            hr = pConverter->ProcessInput(0, outputDataBuffer.pSample, 0);
            if (FAILED(hr)) Log("Conv ProcessInput FAILED hr=" + std::to_string(hr));
            if (SUCCEEDED(hr)) {
                while (true) {
                    MFT_OUTPUT_DATA_BUFFER convOutput = {0};
                    convOutput.dwStreamID = 0;
                    
                    MFT_OUTPUT_STREAM_INFO convInfo = {0};
                    pConverter->GetOutputStreamInfo(0, &convInfo);
                    bool convProvides = (convInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

                    IMFSample* pConvSample = NULL;
                    IMFMediaBuffer* pConvBuffer = NULL;
                    if (!convProvides) {
                        MFCreateSample(&pConvSample);
                        MFCreateMemoryBuffer(m_width * m_height * 4, &pConvBuffer); // RGB32
                        pConvSample->AddBuffer(pConvBuffer);
                        pConvBuffer->Release();
                        convOutput.pSample = pConvSample;
                    }
                    
                    DWORD convStatus = 0;
                    hr = pConverter->ProcessOutput(0, 1, &convOutput, &convStatus);
                    
                    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
                        if (pConvSample) pConvSample->Release();
                        break;
                    }
                    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
                        if (pConvSample) pConvSample->Release();
                        IMFMediaType* pNewType = NULL;
                        if (SUCCEEDED(pConverter->GetOutputAvailableType(0, 0, &pNewType))) {
                            pConverter->SetOutputType(0, pNewType, 0);
                            pNewType->Release();
                        }
                        continue;
                    }
                    
                    if (FAILED(hr)) Log("Conv ProcessOutput FAILED hr=" + std::to_string(hr));
                    
                    if (SUCCEEDED(hr) && convOutput.pSample) {
                        IMFMediaBuffer* pResBuffer = NULL;
                        convOutput.pSample->ConvertToContiguousBuffer(&pResBuffer);
                        BYTE* pResData = NULL;
                        DWORD cbResData = 0;
                        pResBuffer->Lock(&pResData, NULL, &cbResData);
                        
                        size_t oldSize = outRgb32.size();
                        outRgb32.resize(oldSize + cbResData);
                        memcpy(outRgb32.data() + oldSize, pResData, cbResData);
                        
                        pResBuffer->Unlock();
                        pResBuffer->Release();
                        convOutput.pSample->Release();
                    } else {
                        if (convOutput.pSample) convOutput.pSample->Release();
                        if (pConvSample) pConvSample->Release();
                        break;
                    }
                    if (convOutput.pEvents) convOutput.pEvents->Release();
                }
            }
            outputDataBuffer.pSample->Release();
        } else {
            if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();
            if (pOutSample) pOutSample->Release();
            break;
        }
        
        if (outputDataBuffer.pEvents) outputDataBuffer.pEvents->Release();
    }
    
    if (outRgb32.empty()) Log("DecodeFrame END EMPTY");
    return !outRgb32.empty();
}
