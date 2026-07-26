#include "H264Helper.h"
#include <fstream>
#include <string>
#include <codecapi.h>

static void Log(const std::string& msg) {
#ifdef _DEBUG
    static std::ofstream ofs("h264_debug.log", std::ios::app);
    ofs << GetTickCount() << " " << msg << std::endl;
#endif
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

static void SafeReleaseMFT(IMFTransform** ppMFT) {
    if (ppMFT) {
        IMFTransform* pMFT = (IMFTransform*)InterlockedExchangePointer((PVOID*)ppMFT, NULL);
        if (pMFT) {
            pMFT->Release();
        }
    }
}

static void SafeReleaseDXGI(IMFDXGIDeviceManager** ppDXGI) {
    if (ppDXGI) {
        IMFDXGIDeviceManager* pDXGI = (IMFDXGIDeviceManager*)InterlockedExchangePointer((PVOID*)ppDXGI, NULL);
        if (pDXGI) {
            pDXGI->Release();
        }
    }
}

H264Encoder::H264Encoder() {}
H264Encoder::~H264Encoder() {
    SafeReleaseMFT(&pEncoder);
    SafeReleaseMFT(&pColorConv);
    SafeReleaseDXGI(&pDXGIManager);
}

bool H264Encoder::Initialize(UINT32 width, UINT32 height, bool isScreenShare, ID3D11Device* pD3D11Device) {
    if (IsHardwareAccelerationEnabled()) {
        if (InitializeInternal(width, height, isScreenShare, true, pD3D11Device)) return true;
        Log("H264Encoder HW init failed, falling back to SW");
    }
    return InitializeInternal(width, height, isScreenShare, false, nullptr);
}

static HRESULT SafeCoCreateH264Encoder(IMFTransform** ppEncoder) {
    __try {
        return CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(ppEncoder));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return E_FAIL;
    }
}

bool H264Encoder::InitializeInternal(UINT32 width, UINT32 height, bool isScreenShare, bool useHW, ID3D11Device* pD3DDevice) {
    Log("H264Encoder::InitializeInternal");
    if (pEncoder) { pEncoder->Release(); pEncoder = NULL; }
    if (pColorConv) { pColorConv->Release(); pColorConv = NULL; }
    m_origWidth = width;
    m_origHeight = height;
    m_isScreenShare = isScreenShare;

    UINT32 encWidth = width;
    UINT32 encHeight = height;
    if (isScreenShare) {
        if (encWidth > 1280 || encHeight > 720) {
            float ratioX = 1280.0f / encWidth;
            float ratioY = 720.0f / encHeight;
            float ratio = min(ratioX, ratioY);
            encWidth = (UINT32)(encWidth * ratio);
            encHeight = (UINT32)(encHeight * ratio);
        }
    }

    // H264 requires even width and height
    if (encWidth % 2 != 0) encWidth--;
    if (encHeight % 2 != 0) encHeight--;

    m_width = encWidth;
    m_height = encHeight;
    m_rtStart = 0;
    m_useHW = useHW;

    if (useHW && g_pDXGIManager) {
        pDXGIManager = g_pDXGIManager;
        pDXGIManager->AddRef();
    } else if (pD3DDevice) {
        MFCreateDXGIDeviceManager(&m_dxgiToken, &pDXGIManager);
        if (pDXGIManager) pDXGIManager->ResetDevice(pD3DDevice, m_dxgiToken);
    }

    HRESULT hr = SafeCoCreateH264Encoder(&pEncoder);
    if (FAILED(hr)) {
        Log("Exception or failure caught during CoCreateInstance for H264 Encoder MFT!");
        return false;
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
    
    // Dynamic bitrate calculation based on resolution
    UINT32 pixels = encWidth * encHeight;
    UINT32 targetBitrate = 1000000; // Base 1Mbps
    if (pixels >= 3840 * 2160) targetBitrate = 6000000; // 4K -> 6Mbps
    else if (pixels >= 2560 * 1440) targetBitrate = 4000000; // 1440p -> 4Mbps
    else if (pixels >= 1920 * 1080) targetBitrate = 2500000; // 1080p -> 2.5Mbps
    else if (pixels >= 1280 * 720) targetBitrate = 1500000; // 720p -> 1.5Mbps
    else targetBitrate = (pixels * 30 * 2) / 10; // Dynamic fallback (~1.5-2.0 bits per pixel)
    if (targetBitrate < 500000) targetBitrate = 500000; // Minimum 500Kbps
    
    pOutputType->SetUINT32(MF_MT_AVG_BITRATE, targetBitrate);
    MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, encWidth, encHeight);
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
        
        // Set dynamic bitrate to CodecAPI as well
        var.vt = VT_UI4;
        var.ulVal = targetBitrate;
        pCodecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var);

        // Explicitly set MaxBitRate and BufferSize to prevent the initial I-frame from exploding in size
        // A smaller VBV buffer forces the encoder to compress the I-frame heavily, keeping it under the network queue limit
        var.vt = VT_UI4;
        var.ulVal = targetBitrate; 
        pCodecAPI->SetValue(&CODECAPI_AVEncCommonMaxBitRate, &var);
        
        var.vt = VT_UI4;
        var.ulVal = targetBitrate; // VBV buffer size = 1 second of bitrate
        pCodecAPI->SetValue(&CODECAPI_AVEncCommonBufferSize, &var);

        var.vt = VT_UI4;
        var.ulVal = 0;
        pCodecAPI->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &var);
        
        // Force periodic I-frames to allow recovery from packet loss
        var.vt = VT_UI4;
        var.ulVal = 60; // GOP size = 60 (I-frame every 2 seconds at 30fps)
        pCodecAPI->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);
        
        // Use CBR (Constant Bitrate) for both screen share and webcam
        // This strictly enforces the targetBitrate, preventing any single I-frame from spiking
        // and overflowing our 8Mbps network UDP queue.
        var.vt = VT_UI4;
        var.ulVal = eAVEncCommonRateControlMode_CBR;
        pCodecAPI->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var);
        
        // Force an I-frame every 300 frames (10 seconds) instead of 2 seconds
        // Generating I-frames too frequently causes network congestion at 8Mbps
        var.vt = VT_UI4;
        var.ulVal = 300;
        pCodecAPI->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);
        
        pCodecAPI->Release();
    }

    IMFMediaType* pInputType = NULL;
    MFCreateMediaType(&pInputType);
    pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    pInputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, encWidth, encHeight);
    MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    
    hr = pEncoder->SetInputType(0, pInputType, 0);
    pInputType->Release();
    if (FAILED(hr)) return false;

    if (pDXGIManager) {
        hr = CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pColorConv));
        if (SUCCEEDED(hr)) {
            pColorConv->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)pDXGIManager);
            
            IMFMediaType* pVpIn = NULL;
            MFCreateMediaType(&pVpIn);
            pVpIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVpIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
            MFSetAttributeSize(pVpIn, MF_MT_FRAME_SIZE, width, height); // Native width/height
            pColorConv->SetInputType(0, pVpIn, 0);
            pVpIn->Release();
            
            IMFMediaType* pVpOut = NULL;
            MFCreateMediaType(&pVpOut);
            pVpOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVpOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            MFSetAttributeSize(pVpOut, MF_MT_FRAME_SIZE, encWidth, encHeight); // Scaled width/height
            pColorConv->SetOutputType(0, pVpOut, 0);
            pVpOut->Release();
            
            pColorConv->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
            pColorConv->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
            pColorConv->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
        }
    }

    Log("H264Encoder::InitializeInternal Setup D3D Manager for Encoder");
    if (pDXGIManager && useHW) {
        hr = pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)pDXGIManager);
        if (FAILED(hr)) {
            Log("H264Encoder::InitializeInternal MFT_MESSAGE_SET_D3D_MANAGER FAILED hr=" + std::to_string(hr));
            return false;
        }
    }

    pEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
    pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
    pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
    return true;
}

bool H264Encoder::EncodeFrame(const BYTE* pNv12Data, DWORD cbData, std::vector<uint8_t>& outH264) {
    if (!pEncoder) return false;
    outH264.clear();
    
    IMFSample* pSample = NULL;
    MFCreateSample(&pSample);
    
    IMFMediaBuffer* pBuffer = NULL;
    if (SUCCEEDED(MFCreate2DMediaBuffer(m_width, m_height, MFVideoFormat_NV12.Data1, FALSE, &pBuffer))) {
        IMF2DBuffer* p2D = NULL;
        if (SUCCEEDED(pBuffer->QueryInterface(IID_PPV_ARGS(&p2D)))) {
            BYTE* pScanline0 = NULL;
            LONG lPitch = 0;
            if (SUCCEEDED(p2D->Lock2D(&pScanline0, &lPitch))) {
                for (UINT32 y = 0; y < m_height; y++) {
                    memcpy(pScanline0 + y * lPitch, pNv12Data + y * m_width, m_width);
                }
                BYTE* pUVScanline0 = pScanline0 + m_height * lPitch;
                const uint8_t* pSrcUV = pNv12Data + m_width * m_height;
                for (UINT32 y = 0; y < m_height / 2; y++) {
                    memcpy(pUVScanline0 + y * lPitch, pSrcUV + y * m_width, m_width);
                }
                p2D->Unlock2D();
            }
            p2D->Release();
        }
        DWORD cbMaxLength = 0;
        pBuffer->GetMaxLength(&cbMaxLength);
        pBuffer->SetCurrentLength(cbMaxLength);
    } else {
        MFCreateMemoryBuffer(cbData, &pBuffer);
        BYTE* pData = NULL;
        pBuffer->Lock(&pData, NULL, NULL);
        memcpy(pData, pNv12Data, cbData);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(cbData);
    }
    
    pSample->AddBuffer(pBuffer);
    pBuffer->Release();
    pSample->SetSampleTime(m_rtStart);
    pSample->SetSampleDuration(333333);
    m_rtStart += 333333;
    
    HRESULT hr = pEncoder->ProcessInput(0, pSample, 0);
    pSample->Release();
    if (FAILED(hr)) {
        Log("H264Encoder EncodeFrame ProcessInput FAILED hr=" + std::to_string(hr) + " HW=" + std::to_string(m_useHW));
        if (m_useHW) {
            Log("H264Encoder EncodeFrame HW failed, falling back to SW dynamically");
            if (InitializeInternal(m_origWidth, m_origHeight, m_isScreenShare, false, nullptr)) {
                return EncodeFrame(pNv12Data, cbData, outH264);
            }
        }
        return false;
    }

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
            DWORD cbSize = encInfo.cbSize > 0 ? encInfo.cbSize : (m_width * m_height * 4);
            DWORD cbAlignment = encInfo.cbAlignment > 0 ? encInfo.cbAlignment : MF_16_BYTE_ALIGNMENT;
            MFCreateAlignedMemoryBuffer(cbSize, cbAlignment, &pEncBuffer);
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
            HRESULT hrType = pEncoder->GetOutputAvailableType(0, 0, &pNewType);
            if (SUCCEEDED(hrType)) {
                hrType = pEncoder->SetOutputType(0, pNewType, 0);
                pNewType->Release();
                if (SUCCEEDED(hrType)) continue;
            }
            break;
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
            if (encProvides && encOutput.pSample) encOutput.pSample->Release();
            if (pEncSample) pEncSample->Release();
        } else {
            if (encProvides && encOutput.pSample) encOutput.pSample->Release();
            if (pEncSample) pEncSample->Release();
            break;
        }
        if (encOutput.pEvents) encOutput.pEvents->Release();
    }
    
    return true; // Successfully processed, output might be buffered (empty)
}

bool H264Encoder::EncodeTexture(ID3D11Texture2D* pTexture, std::vector<uint8_t>& outH264) {
    if (!pEncoder || !pDXGIManager || !pColorConv) return false;
    outH264.clear();

    IMFSample* pSample = NULL;
    MFCreateSample(&pSample);
    
    IMFMediaBuffer* pBuffer = NULL;
    MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), pTexture, 0, FALSE, &pBuffer);
    
    pSample->AddBuffer(pBuffer);
    pBuffer->Release();
    pSample->SetSampleTime(m_rtStart);
    pSample->SetSampleDuration(333333);
    m_rtStart += 333333;

    // Process through Color Converter MFT (BGRA -> NV12)
    HRESULT hr = pColorConv->ProcessInput(0, pSample, 0);
    pSample->Release();
    if (FAILED(hr)) return false;
    
    IMFSample* pNv12Sample = NULL;
    while (true) {
        MFT_OUTPUT_DATA_BUFFER vpOutput = {0};
        vpOutput.dwStreamID = 0;
        
        MFT_OUTPUT_STREAM_INFO vpInfo = {0};
        pColorConv->GetOutputStreamInfo(0, &vpInfo);
        bool vpProvides = (vpInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

        IMFMediaBuffer* pVpBuffer = NULL;
        if (!vpProvides) {
            MFCreateSample(&pNv12Sample);
            DWORD cbSize = vpInfo.cbSize > 0 ? vpInfo.cbSize : (m_width * m_height * 2); // NV12 is 1.5x but give some leeway
            DWORD cbAlignment = vpInfo.cbAlignment > 0 ? vpInfo.cbAlignment : MF_16_BYTE_ALIGNMENT;
            MFCreateAlignedMemoryBuffer(cbSize, cbAlignment, &pVpBuffer);
            pNv12Sample->AddBuffer(pVpBuffer);
            pVpBuffer->Release();
            vpOutput.pSample = pNv12Sample;
        }

        DWORD vpStatus = 0;
        hr = pColorConv->ProcessOutput(0, 1, &vpOutput, &vpStatus);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            if (!vpProvides && pNv12Sample) { pNv12Sample->Release(); pNv12Sample = NULL; }
            break;
        }
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            if (!vpProvides && pNv12Sample) { pNv12Sample->Release(); pNv12Sample = NULL; }
            IMFMediaType* pNewType = NULL;
            HRESULT hrType = pColorConv->GetOutputAvailableType(0, 0, &pNewType);
            if (SUCCEEDED(hrType)) {
                hrType = pColorConv->SetOutputType(0, pNewType, 0);
                pNewType->Release();
                if (SUCCEEDED(hrType)) continue;
            }
            break;
        }
        
        if (SUCCEEDED(hr) && vpOutput.pSample) {
            if (vpProvides) {
                pNv12Sample = vpOutput.pSample;
                pNv12Sample->AddRef();
                vpOutput.pSample->Release();
            } else {
                // pNv12Sample is already the one we allocated, refcount is 1
            }
            break; // We got our converted sample
        } else {
            if (vpProvides && vpOutput.pSample) vpOutput.pSample->Release();
            if (!vpProvides && pNv12Sample) { pNv12Sample->Release(); pNv12Sample = NULL; }
            break;
        }
    }
    
    if (!pNv12Sample) return false;
    
    // Now pass NV12 sample to H.264 Encoder
    hr = pEncoder->ProcessInput(0, pNv12Sample, 0);
    pNv12Sample->Release();
    if (FAILED(hr)) return false;

    // Process output from encoder (same as EncodeFrame)
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
            DWORD cbSize = encInfo.cbSize > 0 ? encInfo.cbSize : (m_width * m_height * 4);
            DWORD cbAlignment = encInfo.cbAlignment > 0 ? encInfo.cbAlignment : MF_16_BYTE_ALIGNMENT;
            MFCreateAlignedMemoryBuffer(cbSize, cbAlignment, &pEncBuffer);
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
            HRESULT hrType = pEncoder->GetOutputAvailableType(0, 0, &pNewType);
            if (SUCCEEDED(hrType)) {
                hrType = pEncoder->SetOutputType(0, pNewType, 0);
                pNewType->Release();
                if (SUCCEEDED(hrType)) continue;
            }
            break;
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
            if (encProvides && encOutput.pSample) encOutput.pSample->Release();
            if (pEncSample) pEncSample->Release();
        } else {
            if (encProvides && encOutput.pSample) encOutput.pSample->Release();
            if (pEncSample) pEncSample->Release();
            break;
        }
        if (encOutput.pEvents) encOutput.pEvents->Release();
    }
    
    return true; // Successfully processed, output might be buffered (empty)
}

H264Decoder::H264Decoder() {}
H264Decoder::~H264Decoder() {
    if (pDecoder) pDecoder->Release();
    if (pConverter) pConverter->Release();
}

bool H264Decoder::Initialize(UINT32 width, UINT32 height) {
    if (g_pDXGIManager) {
        if (InitializeInternal(width, height, true)) return true;
        Log("H264Decoder hardware init failed, falling back to software");
    }
    return InitializeInternal(width, height, false);
}

bool H264Decoder::InitializeInternal(UINT32 width, UINT32 height, bool useHW) {
    Log("H264Decoder::InitializeInternal");
    if (pDecoder) pDecoder->Release();
    if (pConverter) pConverter->Release();
    pDecoder = NULL;
    pConverter = NULL;
    m_width = width;
    m_height = height;
    m_rtStart = 0;
    m_useHW = useHW;

    HRESULT hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDecoder));
    if (FAILED(hr)) return false;

    if (useHW && g_pDXGIManager) {
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
        
        if (!useHW || !g_pDXGIManager) {
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

    if (useHW && g_pDXGIManager) {
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
    pConvOutputType->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)(width * 4));
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
    if (FAILED(hr)) {
        if (m_useHW) {
            Log("H264Decoder DecodeFrame HW failed, falling back to SW dynamically");
            if (InitializeInternal(m_width, m_height, false)) {
                return DecodeFrame(inH264, outRgb32);
            }
        }
        Log("Dec ProcessInput FAILED hr=" + std::to_string(hr));
        return false;
    }
    
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
            HRESULT hrType = pDecoder->GetOutputAvailableType(0, 0, &pNewType);
            if (SUCCEEDED(hrType)) {
                hrType = pDecoder->SetOutputType(0, pNewType, 0);
                if (SUCCEEDED(hrType)) {
                    hrType = pConverter->SetInputType(0, pNewType, 0); // UPDATE CONVERTER TOO!
                }
                
                MFGetAttributeSize(pNewType, MF_MT_FRAME_SIZE, &m_width, &m_height);
                
                IMFMediaType* pConvOutType = NULL;
                if (SUCCEEDED(hrType) && SUCCEEDED(MFCreateMediaType(&pConvOutType))) {
                    pConvOutType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
                    pConvOutType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
                    MFSetAttributeSize(pConvOutType, MF_MT_FRAME_SIZE, m_width, m_height);
                    pConvOutType->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)(m_width * 4));
                    hrType = pConverter->SetOutputType(0, pConvOutType, 0);
                    pConvOutType->Release();
                }
                
                pNewType->Release();
                if (SUCCEEDED(hrType)) continue;
            }
            break;
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
                        HRESULT hrType = MFCreateMediaType(&pNewType);
                        if (SUCCEEDED(hrType)) {
                            pNewType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
                            pNewType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
                            MFSetAttributeSize(pNewType, MF_MT_FRAME_SIZE, m_width, m_height);
                            pNewType->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)(m_width * 4));
                            hrType = pConverter->SetOutputType(0, pNewType, 0);
                            pNewType->Release();
                            if (SUCCEEDED(hrType)) continue;
                        }
                        break;
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
    
    return true; // Output might be buffered (empty), but processing succeeded
}
