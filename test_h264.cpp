#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

int main() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    IMFTransform* pEncoder = NULL;
    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEncoder));
    if (FAILED(hr)) { std::cout << "Failed to create H.264 encoder MFT\n"; return 1; }

    IMFMediaType* pOutputType = NULL;
    MFCreateMediaType(&pOutputType);
    pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    pOutputType->SetUINT32(MF_MT_AVG_BITRATE, 1000000); // 1 Mbps
    MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, 640, 480);
    MFSetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pOutputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    pOutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    
    hr = pEncoder->SetOutputType(0, pOutputType, 0);
    if (FAILED(hr)) { std::cout << "Failed to set output type. hr=" << std::hex << hr << "\n"; return 1; }
    else { std::cout << "Set output type success!\n"; }

    IMFMediaType* pInputType = NULL;
    MFCreateMediaType(&pInputType);
    pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12); // H.264 encoder usually wants NV12
    pInputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, 640, 480);
    MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    
    hr = pEncoder->SetInputType(0, pInputType, 0);
    if (FAILED(hr)) { std::cout << "Failed to set input type RGB32. hr=" << std::hex << hr << "\n"; }
    else { std::cout << "Set input type RGB32 success!\n"; }

    // Let's try YUY2 if RGB32 fails
    if (FAILED(hr)) {
        pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
        hr = pEncoder->SetInputType(0, pInputType, 0);
        if (FAILED(hr)) {
            std::cout << "Failed to set input type YUY2. hr=" << std::hex << hr << "\n";
            pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            hr = pEncoder->SetInputType(0, pInputType, 0);
        }
    }
    
    std::cout << "Final Input Type HR: " << std::hex << hr << "\n";

    IMFTransform* pDecoder = NULL;
    hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDecoder));
    if (FAILED(hr)) { std::cout << "Failed to create H.264 decoder MFT\n"; return 1; }

    IMFMediaType* pDecInputType = NULL;
    MFCreateMediaType(&pDecInputType);
    pDecInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pDecInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(pDecInputType, MF_MT_FRAME_SIZE, 640, 480);
    hr = pDecoder->SetInputType(0, pDecInputType, 0);
    if (FAILED(hr)) { std::cout << "Decoder: Failed to set input type. hr=" << std::hex << hr << "\n"; }
    
    DWORD typeIndex = 0;
    while (true) {
        IMFMediaType* pAvailableType = NULL;
        hr = pDecoder->GetOutputAvailableType(0, typeIndex, &pAvailableType);
        if (hr == MF_E_NO_MORE_TYPES) {
            break;
        }
        if (SUCCEEDED(hr)) {
            GUID subtype = { 0 };
            pAvailableType->GetGUID(MF_MT_SUBTYPE, &subtype);
            
            char guidStr[40];
            sprintf_s(guidStr, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                subtype.Data1, subtype.Data2, subtype.Data3,
                subtype.Data4[0], subtype.Data4[1], subtype.Data4[2], subtype.Data4[3],
                subtype.Data4[4], subtype.Data4[5], subtype.Data4[6], subtype.Data4[7]);
                
            std::cout << "Available Output Type " << typeIndex << ": " << guidStr << "\n";
            pAvailableType->Release();
        } else {
            std::cout << "GetOutputAvailableType failed at index " << typeIndex << " hr=" << std::hex << hr << "\n";
            break;
        }
        typeIndex++;
    }
    pDecInputType->Release();
    pDecoder->Release();
    pInputType->Release();
    pEncoder->Release();
    MFShutdown();
    CoUninitialize();
    return 0;
}
