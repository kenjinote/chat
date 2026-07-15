#include "H264Helper.h"

H264Encoder::H264Encoder() {}
H264Encoder::~H264Encoder() {
    if (pEncoder) pEncoder->Release();
    if (pConverter) pConverter->Release();
}

bool H264Encoder::Initialize(UINT32 width, UINT32 height) {
    if (pEncoder) pEncoder->Release();
    if (pConverter) pConverter->Release();
    pEncoder = NULL;
    pConverter = NULL;
    m_width = width;
    m_height = height;
    m_rtStart = 0;

    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEncoder));
    if (FAILED(hr)) return false;

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
    
    std::cout << "E1" << std::endl;
    IMFSample* pSample = NULL;
    MFCreateSample(&pSample);
    
    std::cout << "E2" << std::endl;
    IMFMediaBuffer* pBuffer = NULL;
    MFCreateMemoryBuffer(cbData, &pBuffer);
    
    std::cout << "E3" << std::endl;
    BYTE* pData = NULL;
    pBuffer->Lock(&pData, NULL, NULL);
    memcpy(pData, pRgb32Data, cbData);
    pBuffer->Unlock();
    pBuffer->SetCurrentLength(cbData);
    
    std::cout << "E4" << std::endl;
    pSample->AddBuffer(pBuffer);
    pSample->SetSampleTime(m_rtStart);
    pSample->SetSampleDuration(333333);
    m_rtStart += 333333;
    
    std::cout << "E5" << std::endl;
    HRESULT hr = pConverter->ProcessInput(0, pSample, 0);
    pBuffer->Release();
    pSample->Release();
    if (FAILED(hr)) { std::cout << "pConverter ProcessInput failed: " << std::hex << hr << std::endl; return false; }

    std::cout << "E6" << std::endl;
    while (true) {
        MFT_OUTPUT_DATA_BUFFER convOutput = {0};
        convOutput.dwStreamID = 0;
        
        IMFSample* pConvSample = NULL;
        MFCreateSample(&pConvSample);
        IMFMediaBuffer* pConvBuffer = NULL;
        MFCreateMemoryBuffer(m_width * m_height * 2, &pConvBuffer); // NV12
        pConvSample->AddBuffer(pConvBuffer);
        convOutput.pSample = pConvSample;
        
        DWORD status = 0;
        hr = pConverter->ProcessOutput(0, 1, &convOutput, &status);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            pConvBuffer->Release();
            pConvSample->Release();
            break;
        }
        
        if (SUCCEEDED(hr) && convOutput.pSample) {
            hr = pEncoder->ProcessInput(0, convOutput.pSample, 0);
            if (SUCCEEDED(hr)) {
                while (true) {
                    MFT_OUTPUT_DATA_BUFFER encOutput = {0};
                    encOutput.dwStreamID = 0;
                    
                    IMFSample* pEncSample = NULL;
                    MFCreateSample(&pEncSample);
                    IMFMediaBuffer* pEncBuffer = NULL;
                    MFCreateMemoryBuffer(m_width * m_height * 4, &pEncBuffer);
                    pEncSample->AddBuffer(pEncBuffer);
                    encOutput.pSample = pEncSample;
                    
                    DWORD encStatus = 0;
                    hr = pEncoder->ProcessOutput(0, 1, &encOutput, &encStatus);
                    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
                        pEncBuffer->Release();
                        pEncSample->Release();
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
                        encOutput.pSample->Release();
                    } else {
                        if (encOutput.pSample) encOutput.pSample->Release();
                        pEncBuffer->Release();
                        pEncSample->Release();
                        break;
                    }
                    if (encOutput.pEvents) encOutput.pEvents->Release();
                }
            }
            convOutput.pSample->Release();
        } else {
            if (convOutput.pSample) convOutput.pSample->Release();
            pConvBuffer->Release();
            pConvSample->Release();
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
    if (pDecoder) pDecoder->Release();
    if (pConverter) pConverter->Release();
    pDecoder = NULL;
    pConverter = NULL;
    m_width = width;
    m_height = height;
    m_rtStart = 0;

    HRESULT hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDecoder));
    if (FAILED(hr)) { std::cout << "Dec Create Failed\n"; return false; }

    IMFMediaType* pDecInputType = NULL;
    MFCreateMediaType(&pDecInputType);
    pDecInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pDecInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(pDecInputType, MF_MT_FRAME_SIZE, width, height);
    hr = pDecoder->SetInputType(0, pDecInputType, 0);
    pDecInputType->Release();
    if (FAILED(hr)) { std::cout << "Dec SetInput Failed\n"; return false; }

    IMFMediaType* pDecOutputType = NULL;
    MFCreateMediaType(&pDecOutputType);
    pDecOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pDecOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(pDecOutputType, MF_MT_FRAME_SIZE, width, height);
    hr = pDecoder->SetOutputType(0, pDecOutputType, 0);
    pDecOutputType->Release();
    if (FAILED(hr)) { std::cout << "Dec SetOutput Failed\n"; return false; }

    hr = CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pConverter));
    if (FAILED(hr)) { std::cout << "Dec Conv Create Failed\n"; return false; }

    IMFMediaType* pConvInputType = NULL;
    MFCreateMediaType(&pConvInputType);
    pConvInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(pConvInputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetInputType(0, pConvInputType, 0);
    pConvInputType->Release();
    if (FAILED(hr)) { std::cout << "Dec Conv SetInput Failed\n"; return false; }

    IMFMediaType* pConvOutputType = NULL;
    MFCreateMediaType(&pConvOutputType);
    pConvOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pConvOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    MFSetAttributeSize(pConvOutputType, MF_MT_FRAME_SIZE, width, height);
    hr = pConverter->SetOutputType(0, pConvOutputType, 0);
    pConvOutputType->Release();
    if (FAILED(hr)) { std::cout << "Dec Conv SetOutput Failed\n"; return false; }

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
    pSample->SetSampleTime(m_rtStart);
    m_rtStart += 333333;
    
    HRESULT hr = pDecoder->ProcessInput(0, pSample, 0);
    pBuffer->Release();
    pSample->Release();
    
    outRgb32.clear();
    
    while (true) {
        MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {0};
        outputDataBuffer.dwStreamID = 0;
        
        IMFSample* pOutSample = NULL;
        MFCreateSample(&pOutSample);
        IMFMediaBuffer* pOutBuffer = NULL;
        MFCreateMemoryBuffer(m_width * m_height * 2, &pOutBuffer); // NV12
        pOutSample->AddBuffer(pOutBuffer);
        outputDataBuffer.pSample = pOutSample;
        
        DWORD status = 0;
        hr = pDecoder->ProcessOutput(0, 1, &outputDataBuffer, &status);
        
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            pOutBuffer->Release();
            pOutSample->Release();
            break;
        }
        
        if (SUCCEEDED(hr) && outputDataBuffer.pSample) {
            // Pass NV12 to converter
            hr = pConverter->ProcessInput(0, outputDataBuffer.pSample, 0);
            if (SUCCEEDED(hr)) {
                while (true) {
                    MFT_OUTPUT_DATA_BUFFER convOutput = {0};
                    convOutput.dwStreamID = 0;
                    
                    IMFSample* pConvSample = NULL;
                    MFCreateSample(&pConvSample);
                    IMFMediaBuffer* pConvBuffer = NULL;
                    MFCreateMemoryBuffer(m_width * m_height * 4, &pConvBuffer); // RGB32
                    pConvSample->AddBuffer(pConvBuffer);
                    convOutput.pSample = pConvSample;
                    
                    DWORD convStatus = 0;
                    hr = pConverter->ProcessOutput(0, 1, &convOutput, &convStatus);
                    
                    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
                        pConvBuffer->Release();
                        pConvSample->Release();
                        break;
                    }
                    
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
                        pConvBuffer->Release();
                        pConvSample->Release();
                        break;
                    }
                    if (convOutput.pEvents) convOutput.pEvents->Release();
                }
            }
            outputDataBuffer.pSample->Release();
        } else {
            if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();
            pOutBuffer->Release();
            pOutSample->Release();
            break;
        }
        
        if (outputDataBuffer.pEvents) outputDataBuffer.pEvents->Release();
    }
    
    return !outRgb32.empty();
}
