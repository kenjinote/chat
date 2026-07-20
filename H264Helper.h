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
bool IsHardwareAccelerationEnabled();

class H264Encoder {
    IMFTransform* pEncoder = NULL;
    IMFTransform* pColorConv = NULL;
    IMFDXGIDeviceManager* pDXGIManager = NULL;
    UINT m_dxgiToken = 0;
    DWORD inputId = 0;
    DWORD outputId = 0;
    UINT32 m_width = 0;
    UINT32 m_height = 0;
    UINT32 m_origWidth = 0;
    UINT32 m_origHeight = 0;
    LONGLONG m_rtStart = 0;
    bool m_useHW = false;
    bool m_isScreenShare = false;
public:
    H264Encoder();
    ~H264Encoder();
    bool Initialize(UINT32 width, UINT32 height, bool isScreenShare, ID3D11Device* pD3DDevice = nullptr);
    bool EncodeTexture(ID3D11Texture2D* pTexture, std::vector<uint8_t>& outH264);
    bool EncodeFrame(const BYTE* pNv12Data, DWORD cbData, std::vector<uint8_t>& outH264);
    bool IsHardware() const { return m_useHW; }
private:
    bool InitializeInternal(UINT32 width, UINT32 height, bool isScreenShare, bool useHW, ID3D11Device* pD3DDevice);
};

class H264Decoder {
    IMFTransform* pDecoder = NULL;
    IMFTransform* pConverter = NULL;
    UINT32 m_width = 0;
    UINT32 m_height = 0;
    LONGLONG m_rtStart = 0;
    bool m_useHW = false;
public:
    H264Decoder();
    ~H264Decoder();
    bool Initialize(UINT32 width, UINT32 height);
    bool DecodeFrame(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRgb32);
    bool IsHardware() const { return m_useHW; }
    UINT32 GetWidth() const { return m_width; }
    UINT32 GetHeight() const { return m_height; }
private:
    bool InitializeInternal(UINT32 width, UINT32 height, bool useHW);
};
