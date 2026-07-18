#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <d3d11.h>
#include <vector>
#include <mutex>
#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "d3d11.lib")

bool InitHardwareAcceleration();
void CleanupHardwareAcceleration();

class H264Encoder {
    IMFTransform* pEncoder = NULL;
    IMFTransform* pConverter = NULL;
    DWORD inputId = 0;
    DWORD outputId = 0;
    UINT32 m_width = 0;
    UINT32 m_height = 0;
    LONGLONG m_rtStart = 0;
public:
    H264Encoder();
    ~H264Encoder();
    bool Initialize(UINT32 width, UINT32 height);
    bool EncodeFrame(const BYTE* pRgb32Data, DWORD cbData, std::vector<uint8_t>& outH264);
};

class H264Decoder {
    IMFTransform* pDecoder = NULL;
    IMFTransform* pConverter = NULL;
    UINT32 m_width = 0;
    UINT32 m_height = 0;
    LONGLONG m_rtStart = 0;
public:
    H264Decoder();
    ~H264Decoder();
    bool Initialize(UINT32 width, UINT32 height);
    bool DecodeFrame(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRgb32);
};
