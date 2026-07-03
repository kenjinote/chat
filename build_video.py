import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add headers and globals
video_globals = """#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <mutex>
#include <map>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")

// --- Video Globals ---
IWICImagingFactory* g_pWICFactory = NULL;
bool g_isWebcamActive = false;
uint16_t g_videoSeq = 0;

std::map<std::wstring, DWORD> g_videoStreamers;
std::map<std::wstring, HWND> g_videoWindows;
std::map<std::wstring, std::vector<uint8_t>> g_videoPixelsMap;
std::map<std::wstring, std::pair<int, int>> g_videoSizeMap;
std::mutex g_videoMutex;

struct VideoFrameBuffer {
    std::map<int, std::vector<uint8_t>> fragments;
    int totalFragments = 0;
    DWORD lastUpdate = 0;
};
std::map<std::wstring, std::map<uint16_t, VideoFrameBuffer>> g_videoBuffers;

bool EncodeJPEG(UINT width, UINT height, BYTE* pData, DWORD cbData, std::vector<uint8_t>& outJpeg) {
    IWICBitmap* pWicBitmap = NULL;
    if (FAILED(g_pWICFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, cbData, pData, &pWicBitmap))) return false;
    
    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    IWICBitmapEncoder* pEncoder = NULL;
    g_pWICFactory->CreateEncoder(GUID_ContainerFormatJpeg, NULL, &pEncoder);
    pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    
    IWICBitmapFrameEncode* pFrame = NULL;
    pEncoder->CreateNewFrame(&pFrame, NULL);
    pFrame->Initialize(NULL);
    pFrame->SetSize(width, height);
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    pFrame->SetPixelFormat(&format);
    pFrame->WriteSource(pWicBitmap, NULL);
    pFrame->Commit();
    pEncoder->Commit();
    
    STATSTG stat;
    pStream->Stat(&stat, STATFLAG_NONAME);
    int jpegSize = stat.cbSize.LowPart;
    outJpeg.resize(jpegSize);
    
    LARGE_INTEGER liZero = {0};
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    ULONG bytesRead;
    pStream->Read(outJpeg.data(), jpegSize, &bytesRead);
    
    pFrame->Release(); pEncoder->Release(); pStream->Release(); pWicBitmap->Release();
    return true;
}

DWORD WINAPI WebcamThread(LPVOID lpParam) {
    MFStartup(MF_VERSION);
    IMFMediaSource* pSource = NULL;
    IMFAttributes* pAttributes = NULL;
    MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    IMFActivate** ppDevices = NULL;
    UINT32 count = 0;
    MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (count > 0) ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
    for (UINT32 i = 0; i < count; i++) ppDevices[i]->Release();
    CoTaskMemFree(ppDevices);
    pAttributes->Release();

    if (!pSource) return 0;

    IMFSourceReader* pReader = NULL;
    MFCreateSourceReaderFromMediaSource(pSource, NULL, &pReader);
    IMFMediaType* pType = NULL;
    MFCreateMediaType(&pType);
    pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
    pType->Release();

    while (true) {
        if (!g_isWebcamActive || g_sock == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }

        IMFSample* pSample = NULL;
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        HRESULT hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &pSample);
        if (SUCCEEDED(hr) && pSample) {
            IMFMediaBuffer* pBuffer = NULL;
            pSample->ConvertToContiguousBuffer(&pBuffer);
            BYTE* pData = NULL;
            DWORD cbData = 0;
            pBuffer->Lock(&pData, NULL, &cbData);

            IMFMediaType* pCurrentType = NULL;
            pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
            UINT32 width = 0, height = 0;
            MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &width, &height);
            pCurrentType->Release();

            std::vector<uint8_t> jpegData;
            if (EncodeJPEG(width, height, pData, cbData, jpegData)) {
                const int MAX_PAYLOAD = 1000;
                int totalFragments = (jpegData.size() + MAX_PAYLOAD - 1) / MAX_PAYLOAD;
                g_videoSeq++;
                for (int i = 0; i < totalFragments; i++) {
                    int offset = i * MAX_PAYLOAD;
                    int len = min(MAX_PAYLOAD, (int)jpegData.size() - offset);
                    
                    char udpPkt[2048];
                    MediaHeader* mhdr = (MediaHeader*)udpPkt;
                    mhdr->sessionId = g_sessionId;
                    mhdr->seqNum = g_videoSeq;
                    mhdr->payloadType = 2; 
                    mhdr->flags = (i == 0 ? 1 : 0) | (i == totalFragments - 1 ? 2 : 0);
                    mhdr->timestamp = i; 
                    wcsncpy_s(mhdr->username, g_username.c_str(), 31);
                    
                    memcpy(udpPkt + sizeof(MediaHeader), jpegData.data() + offset, len);
                    
                    sockaddr_in serverUdp = {0};
                    serverUdp.sin_family = AF_INET;
                    serverUdp.sin_port = htons(4001);
                    inet_pton(AF_INET, g_serverIp.c_str(), &serverUdp.sin_addr);
                    sendto(g_sock, udpPkt, sizeof(MediaHeader) + len, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                }
            }
            pBuffer->Unlock();
            pBuffer->Release();
            pSample->Release();
        } else {
            Sleep(10);
        }
    }
    return 0;
}

void DecodeJPEG(const std::vector<uint8_t>& jpegData, const std::wstring& username) {
    IStream* pStream = NULL;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, jpegData.size());
    void* pData = GlobalLock(hGlobal);
    memcpy(pData, jpegData.data(), jpegData.size());
    GlobalUnlock(hGlobal);
    CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
    
    IWICBitmapDecoder* pDecoder = NULL;
    if (SUCCEEDED(g_pWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder))) {
        IWICBitmapFrameDecode* pFrame = NULL;
        if (SUCCEEDED(pDecoder->GetFrame(0, &pFrame))) {
            UINT width, height;
            pFrame->GetSize(&width, &height);
            
            IWICFormatConverter* pConverter = NULL;
            g_pWICFactory->CreateFormatConverter(&pConverter);
            pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
            
            std::vector<uint8_t> pixels(width * height * 4);
            pConverter->CopyPixels(NULL, width * 4, pixels.size(), pixels.data());
            
            {
                std::lock_guard<std::mutex> lock(g_videoMutex);
                g_videoSizeMap[username] = {width, height};
                g_videoPixelsMap[username] = std::move(pixels);
            }
            pConverter->Release();
            pFrame->Release();
        }
        pDecoder->Release();
    }
    pStream->Release();
    HWND hView = g_videoWindows[username];
    if (hView && IsWindow(hView)) {
        InvalidateRect(hView, NULL, FALSE);
    }
}
"""

content = content.replace('// --- Audio Globals ---', video_globals + '\n// --- Audio Globals ---')

# 2. Modify struct MediaHeader
old_mh = """struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t payloadType; // 1 = Audio, 2 = Video, 255 = HolePunch
    uint8_t flags;
    uint32_t timestamp;
};"""
new_mh = """struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t payloadType; // 1 = Audio, 2 = Video, 255 = HolePunch
    uint8_t flags;
    uint32_t timestamp;
    wchar_t username[32];
};"""
content = content.replace(old_mh, new_mh)

# 3. Add Video Decoding to RecvThread
recv_video = """else if (mhdr->payloadType == 2) {
                        std::wstring wname = mhdr->username;
                        uint16_t seq = mhdr->seqNum;
                        int fragIdx = mhdr->timestamp;
                        int len = res - sizeof(MediaHeader);
                        
                        g_videoStreamers[wname] = GetTickCount();
                        InvalidateRect(g_hUserList, NULL, FALSE);

                        auto& vbuf = g_videoBuffers[wname][seq];
                        vbuf.lastUpdate = GetTickCount();
                        vbuf.fragments[fragIdx].assign(udpBuf + sizeof(MediaHeader), udpBuf + sizeof(MediaHeader) + len);
                        
                        if (mhdr->flags & 2) { 
                            vbuf.totalFragments = fragIdx + 1;
                        }
                        
                        if (vbuf.totalFragments > 0 && vbuf.fragments.size() == vbuf.totalFragments) {
                            std::vector<uint8_t> completeJpeg;
                            for (int i = 0; i < vbuf.totalFragments; i++) {
                                completeJpeg.insert(completeJpeg.end(), vbuf.fragments[i].begin(), vbuf.fragments[i].end());
                            }
                            DecodeJPEG(completeJpeg, wname);
                            g_videoBuffers[wname].clear();
                        }
                    }"""

old_recv = """if (mhdr->payloadType == 1) { // Audio
                        PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - sizeof(MediaHeader));
                    }"""
content = content.replace(old_recv, old_recv + "\n                    " + recv_video)

# 4. Create Video View Window Class
video_wndproc = """
LRESULT CALLBACK VideoViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::wstring* pUser = (std::wstring*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_NCCREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        std::wstring* u = new std::wstring((LPCWSTR)pcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)u);
        return TRUE;
    }
    case WM_NCDESTROY: {
        if (pUser) delete pUser;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        static std::map<HWND, ID2D1HwndRenderTarget*> rts;
        if (!rts[hwnd]) {
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            g_pD2DFactory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)), &rts[hwnd]);
        }
        
        ID2D1HwndRenderTarget* pRT = rts[hwnd];
        if (pRT && pUser) {
            pRT->BeginDraw();
            pRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));
            {
                std::lock_guard<std::mutex> lock(g_videoMutex);
                auto& pixels = g_videoPixelsMap[*pUser];
                auto sz = g_videoSizeMap[*pUser];
                if (!pixels.empty()) {
                    ID2D1Bitmap* pBitmap = NULL;
                    pRT->CreateBitmap(D2D1::SizeU(sz.first, sz.second), pixels.data(), sz.first * 4, 
                        D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), &pBitmap);
                    if (pBitmap) {
                        pRT->DrawBitmap(pBitmap, D2D1::RectF(0, 0, rc.right, rc.bottom));
                        pBitmap->Release();
                    }
                }
            }
            pRT->EndDraw();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE: {
        return 0;
    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
"""
content = content.replace("LRESULT CALLBACK WndProc(", video_wndproc + "\nLRESULT CALLBACK WndProc(")

# 5. Register class and create listbox subclass
reg_class = """
    WNDCLASSEX wcexVideo = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, VideoViewProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"D2DVideoViewClass", NULL };
    RegisterClassEx(&wcexVideo);
"""
content = content.replace("RegisterClassEx(&wcex2);", "RegisterClassEx(&wcex2);\n" + reg_class)

init_wic = """
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory));
    CreateThread(NULL, 0, WebcamThread, NULL, 0, NULL);
"""
content = content.replace("D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);", 
                          init_wic + "\nD2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);")

# 6. ListBox subclass to handle Right-click and Double-click
subclass_proc = """
WNDPROC g_oldListProc = NULL;
LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_RBUTTONDOWN) {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        BOOL bOutside = FALSE;
        int item = (int)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
        if (item >= 0) {
            int len = (int)SendMessage(hwnd, LB_GETTEXTLEN, item, 0);
            std::vector<WCHAR> buf(len + 1);
            SendMessage(hwnd, LB_GETTEXT, item, (LPARAM)buf.data());
            std::wstring clickedUser(buf.data());
            if (clickedUser == g_username) {
                g_isWebcamActive = !g_isWebcamActive;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    return CallWindowProc(g_oldListProc, hwnd, msg, wParam, lParam);
}
"""
content = content.replace("LRESULT CALLBACK WndProc(", subclass_proc + "\nLRESULT CALLBACK WndProc(")

subclass_install = """
        g_oldListProc = (WNDPROC)SetWindowLongPtr(g_hUserList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);
"""
content = content.replace("InitAudio();", "InitAudio();\n" + subclass_install)

# 7. ListBox Double Click -> Spawn Video Window
dbl_click = """        } else if (id == 1050 && code == LBN_DBLCLK) {
            int sel = (int)SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                int len = (int)SendMessage(g_hUserList, LB_GETTEXTLEN, sel, 0);
                std::vector<WCHAR> buf(len + 1);
                SendMessage(g_hUserList, LB_GETTEXT, sel, (LPARAM)buf.data());
                std::wstring user(buf.data());
                
                if (GetTickCount() - g_videoStreamers[user] < 5000) {
                    if (!g_videoWindows[user] || !IsWindow(g_videoWindows[user])) {
                        HWND hVid = CreateWindowEx(0, L"D2DVideoViewClass", (L"Video: " + user).c_str(),
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandle(NULL), (LPVOID)user.c_str());
                        g_videoWindows[user] = hVid;
                    } else {
                        SetForegroundWindow(g_videoWindows[user]);
                    }
                }
            }"""
content = content.replace("        } else if (id == 1009 && code == BN_CLICKED) {", dbl_click + "\n        } else if (id == 1009 && code == BN_CLICKED) {")


# 8. TV Frame logic in WM_DRAWITEM 1050
tv_logic = """
            std::wstring user(buf.data());
            
            bool isStreaming = (GetTickCount() - g_videoStreamers[user] < 5000);
            if (user == g_username && g_isWebcamActive) isStreaming = true;

            if (isStreaming) {
                // Draw TV Frame
                Gdiplus::Graphics graphics(hdc);
                Gdiplus::Pen tvPen(Gdiplus::Color(255, 50, 50, 50), 3);
                graphics.DrawRectangle(&tvPen, 3, rc.top + 8, 20, 20);
                
                // Antenna
                Gdiplus::Pen antennaPen(Gdiplus::Color(255, 100, 100, 100), 2);
                graphics.DrawLine(&antennaPen, 15, rc.top + 8, 5, rc.top + 2);
                graphics.DrawLine(&antennaPen, 15, rc.top + 8, 25, rc.top + 2);
                
                // Inner TV Screen color
                Gdiplus::SolidBrush tvBg(Gdiplus::Color(255, 20, 20, 40));
                graphics.FillRectangle(&tvBg, 4, rc.top + 9, 18, 18);
            }
"""
content = content.replace("std::wstring user(buf.data());", tv_logic)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patch Applied successfully.")
