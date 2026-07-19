#include <windows.h>
#include <mfapi.h>
#include <mftransform.h>
#include <iostream>
#include <wmcodecdsp.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

int main() {
    CoInitialize(NULL);
    MFStartup(MF_VERSION);
    IMFTransform* pVP = NULL;
    HRESULT hr = CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pVP));
    if (SUCCEEDED(hr)) {
        IMFMediaType* pIn = NULL;
        MFCreateMediaType(&pIn);
        pIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
        MFSetAttributeSize(pIn, MF_MT_FRAME_SIZE, 1920, 1080);
        hr = pVP->SetInputType(0, pIn, 0);
        std::cout << "SetInputType hr: " << std::hex << hr << std::endl;
        
        IMFMediaType* pOut = NULL;
        hr = pVP->GetOutputAvailableType(0, 0, &pOut);
        std::cout << "GetOutputAvailableType hr: " << std::hex << hr << std::endl;
        if (pOut) pOut->Release();
        pIn->Release();
        pVP->Release();
    }
    MFShutdown();
    CoUninitialize();
    return 0;
}
