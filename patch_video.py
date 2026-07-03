import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add WIC headers and globals
video_globals = """#include <wincodec.h>
#include <mutex>
#include <map>
#pragma comment(lib, "windowscodecs.lib")

// --- Video Globals ---
IWICImagingFactory* g_pWICFactory = NULL;
bool g_isVideoActive = false;
uint16_t g_videoSeq = 0;
HWND g_hVideoView = NULL;

std::vector<uint8_t> g_videoPixels;
UINT g_videoWidth = 0;
UINT g_videoHeight = 0;
std::mutex g_videoMutex;

struct VideoFrameBuffer {
    std::map<int, std::vector<uint8_t>> fragments;
    int totalFragments = 0;
    DWORD lastUpdate = 0;
};
std::map<uint32_t, std::map<uint16_t, VideoFrameBuffer>> g_videoBuffers;

bool CaptureScreenJPEG(std::vector<uint8_t>& outJpeg) {
    HDC hScreen = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreen);
    int width = 640;
    int height = 480;
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);
    
    IWICBitmap* pWicBitmap = NULL;
    if (FAILED(g_pWICFactory->CreateBitmapFromHBITMAP(hBitmap, NULL, WICBitmapUseAlpha, &pWicBitmap))) {
        DeleteObject(hBitmap); DeleteDC(hMemDC); ReleaseDC(NULL, hScreen); return false;
    }
    
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
    DeleteObject(hBitmap); DeleteDC(hMemDC); ReleaseDC(NULL, hScreen);
    return true;
}

DWORD WINAPI VideoCaptureThread(LPVOID lpParam) {
    while (true) {
        if (!g_isVideoActive || g_sock == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }
        
        std::vector<uint8_t> jpegData;
        if (CaptureScreenJPEG(jpegData)) {
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
                mhdr->timestamp = i; // Store fragment index here
                
                memcpy(udpPkt + sizeof(MediaHeader), jpegData.data() + offset, len);
                
                sockaddr_in serverUdp = {0};
                serverUdp.sin_family = AF_INET;
                serverUdp.sin_port = htons(4001);
                inet_pton(AF_INET, g_serverIp.c_str(), &serverUdp.sin_addr);
                sendto(g_sock, udpPkt, sizeof(MediaHeader) + len, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
            }
        }
        Sleep(100); 
    }
    return 0;
}

void DecodeJPEG(const std::vector<uint8_t>& jpegData) {
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
                g_videoWidth = width;
                g_videoHeight = height;
                g_videoPixels = std::move(pixels);
            }
            pConverter->Release();
            pFrame->Release();
        }
        pDecoder->Release();
    }
    pStream->Release();
    InvalidateRect(g_hVideoView, NULL, FALSE);
}
"""

content = content.replace('// --- Audio Globals ---', video_globals + '\n// --- Audio Globals ---')

# 2. Modify struct MediaHeader field timestamp -> timestamp
content = content.replace("uint32_t timestamp;", "uint32_t timestamp; // fragIndex for Video")

# 3. Add Video Decoding to RecvThread
recv_video = """else if (mhdr->payloadType == 2) {
                        uint32_t sid = mhdr->sessionId;
                        uint16_t seq = mhdr->seqNum;
                        int fragIdx = mhdr->timestamp;
                        int len = res - sizeof(MediaHeader);
                        
                        auto& vbuf = g_videoBuffers[sid][seq];
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
                            DecodeJPEG(completeJpeg);
                            g_videoBuffers[sid].clear();
                        }
                    }"""

old_recv = """if (mhdr->payloadType == 1) { // Audio
                        PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - sizeof(MediaHeader));
                    }"""
content = content.replace(old_recv, old_recv + "\n                    " + recv_video)

# 4. Create Video View Window Class
video_wndproc = """
LRESULT CALLBACK VideoViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        static ID2D1HwndRenderTarget* pRT = NULL;
        if (!pRT) {
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            g_pD2DFactory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)), &pRT);
        }
        
        if (pRT) {
            pRT->BeginDraw();
            pRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));
            {
                std::lock_guard<std::mutex> lock(g_videoMutex);
                if (!g_videoPixels.empty()) {
                    ID2D1Bitmap* pBitmap = NULL;
                    pRT->CreateBitmap(D2D1::SizeU(g_videoWidth, g_videoHeight), g_videoPixels.data(), g_videoWidth * 4, 
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
        // resize rt? simple prototype ignores resize RT
        return 0;
    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
"""
content = content.replace("LRESULT CALLBACK WndProc(", video_wndproc + "\nLRESULT CALLBACK WndProc(")

# 5. Register class and create window
reg_class = """
    WNDCLASSEX wcexVideo = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, VideoViewProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"D2DVideoViewClass", NULL };
    RegisterClassEx(&wcexVideo);
"""
content = content.replace("RegisterClassEx(&wcex2);", "RegisterClassEx(&wcex2);\n" + reg_class)

# Initialize WIC
init_wic = """
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory));
"""
content = content.replace("D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);", 
                          init_wic + "\nD2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);")

# Create g_hVideoView in InitNetwork
create_vid = """
        g_hVideoView = CreateWindowEx(WS_EX_CLIENTEDGE, L"D2DVideoViewClass", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        CreateThread(NULL, 0, VideoCaptureThread, NULL, 0, NULL);
"""
content = content.replace("InitAudio();", "InitAudio();\n" + create_vid)

# Layout inside WM_SIZE
# We allocate top-right corner for video: width 320, height 240
layout_mods = """
        int videoW = 320;
        int videoH = 240;
        int logW = width - 20 - videoW;
        MoveWindow(g_hLog, 10, 50, logW, height - 100, TRUE);
        MoveWindow(g_hVideoView, 10 + logW + 10, 50, videoW, videoH, TRUE);
        MoveWindow(g_hUserList, 10 + logW + 10, 50 + videoH + 10, videoW, height - 100 - videoH - 10, TRUE);
"""
content = re.sub(r'MoveWindow\(g_hLog.*?\);\s*MoveWindow\(g_hUserList.*?\);', layout_mods, content, flags=re.DOTALL)

# Handle button toggle for Video
btn_vid = """else if (id == 1010 && code == BN_CLICKED) {
            g_isVideoActive = !g_isVideoActive;
            if (g_isVideoActive) {
                SetWindowText(g_hBtnVideo, L"Video: ON");
            } else {
                SetWindowText(g_hBtnVideo, L"Video: OFF");
            }
        }"""
content = content.replace("// Video toggle (TODO)", btn_vid)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Video Patch Applied")
