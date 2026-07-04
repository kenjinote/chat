// Visual styles removed for classic look

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <d2d1.h>
#include <dwrite.h>
#include <imm.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#include <vector>
#include <string>
#include <set>
#include <cmath>
#include <algorithm>
#include "json11.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#pragma comment(lib, "ole32.lib")


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "imm32.lib")

#include "resource.h"

std::wstring GetString(UINT id) {
    WCHAR buffer[512];
    int len = LoadStringW(GetModuleHandle(NULL), id, buffer, 512);
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

#define PORT 54321
#define WM_USER_MSG (WM_APP + 2)
#define WM_NEW_ROOM (WM_APP + 3)
#define WM_REVERT_ROOM (WM_APP + 4)

HWND g_hLog, g_hInput, g_hSendBtn, g_hUserList;
HWND g_hBtnB, g_hBtnI, g_hBtnU, g_hComboSize, g_hBtnColor;
HWND g_hBtnMoveRoom;
HWND g_hBtnVoice, g_hBtnVideo, g_hVolumeIndicator, g_hSpeakerIcon, g_hVolumeSlider;
IAudioClient* g_pAudioClient = nullptr;
ISimpleAudioVolume* g_pSimpleAudioVolume = nullptr;
IAudioSessionControl* g_pAudioSessionControl = nullptr;

int g_currentMicVolume = 0;
COLORREF g_currentColor = RGB(0, 0, 0);
class CAudioSessionEvents : public IAudioSessionEvents {
    LONG _cRef;
public:
    CAudioSessionEvents() : _cRef(1) {}
    ~CAudioSessionEvents() {}

    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&_cRef); }
    ULONG STDMETHODCALLTYPE Release() {
        ULONG ulRef = InterlockedDecrement(&_cRef);
        if (0 == ulRef) { delete this; }
        return ulRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) {
        if (IID_IUnknown == riid) { *ppvInterface = (IUnknown*)this; }
        else if (__uuidof(IAudioSessionEvents) == riid) { *ppvInterface = (IAudioSessionEvents*)this; }
        else { *ppvInterface = NULL; return E_NOINTERFACE; }
        AddRef(); return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR, LPCGUID) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR, LPCGUID) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(DWORD, float[], DWORD, LPCGUID) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID, LPCGUID) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnSessionDisconnected(AudioSessionDisconnectReason) { return S_OK; }
};
CAudioSessionEvents* g_pAudioEvents = nullptr;

#define WM_APP_VOLUME_CHANGED (WM_APP + 1)

HRESULT STDMETHODCALLTYPE CAudioSessionEvents::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext) {
    if (g_hVolumeSlider) {
        PostMessage(GetParent(g_hVolumeSlider), WM_APP_VOLUME_CHANGED, (WPARAM)(NewVolume * 100), 0);
    }
    return S_OK;
}

#include <Ws2tcpip.h>

SOCKET g_sock = INVALID_SOCKET;
SOCKET g_tcpSocket = INVALID_SOCKET;

// Helper function to resolve DNS names to IP addresses
bool ResolveIP(const std::string& host, IN_ADDR& outAddr) {
    addrinfo hints = {0}, *res = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) == 0 && res != nullptr) {
        outAddr = ((sockaddr_in*)res->ai_addr)->sin_addr;
        freeaddrinfo(res);
        return true;
    }
    return false;
}

std::string g_serverIp = "chat-lb-2026.japaneast.cloudapp.azure.com";
IN_ADDR g_serverUdpAddr = {0};
bool g_isServerUdpAddrResolved = false;

uint32_t g_sessionId = 0;
std::wstring g_username;
int g_dpi = 96;
inline int Scale(int x) { return MulDiv(x, g_dpi, 96); }

HFONT g_hFont;
HFONT g_hFontBold;
HFONT g_hFontI_btn;
HFONT g_hFontU_btn;

std::map<std::wstring, int> g_publicRooms;
std::wstring g_currentRoom = L"";
bool g_isPrivateRoom = false;

ID2D1Factory* g_pD2DFactory = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;

#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <mutex>
#include <map>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")

// --- Video Globals ---
IWICImagingFactory* g_pWICFactory = NULL;
bool g_isWebcamActive = false;
uint16_t g_videoSeq = 0;

std::map<std::wstring, DWORD> g_videoStreamers;
std::map<std::wstring, DWORD> g_voiceUsers;
std::map<std::wstring, int> g_voiceVolumes;
std::mutex g_voiceMutex;
std::mutex g_streamerMutex;
std::map<std::wstring, HWND> g_videoWindows;
std::map<std::wstring, std::vector<uint8_t>> g_videoPixelsMap;
std::map<std::wstring, std::pair<int, int>> g_videoSizeMap;
std::map<std::wstring, std::map<std::wstring, DWORD>> g_videoViewers;
std::mutex g_videoMutex;

struct VideoFrameBuffer {
    std::map<int, std::vector<uint8_t>> fragments;
    int totalFragments = 0;
    DWORD lastUpdate = 0;
};
std::map<std::wstring, std::map<uint16_t, VideoFrameBuffer>> g_videoBuffers;

std::set<std::wstring> g_ignoredUsers;
std::mutex g_ignoredMutex;

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

// --- Audio Globals ---
HWAVEIN g_hWaveIn = NULL;
HWAVEOUT g_hWaveOut = NULL;
bool g_isVoiceActive = false;

#define AUDIO_BUF_SIZE 1280 // 40ms of 16000Hz 16-bit Mono
#define AUDIO_BUF_COUNT 4
WAVEHDR g_waveInHdr[AUDIO_BUF_COUNT];
char g_waveInBuf[AUDIO_BUF_COUNT][AUDIO_BUF_SIZE];

#define AUDIO_OUT_BUF_COUNT 16
WAVEHDR g_waveOutHdr[AUDIO_OUT_BUF_COUNT];
char g_waveOutBuf[AUDIO_OUT_BUF_COUNT][AUDIO_BUF_SIZE];

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\RetroChat", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR buf[256] = { 0 };
        DWORD type = REG_SZ;
        DWORD cbData = sizeof(buf);
        if (RegQueryValueExW(hKey, L"Username", NULL, &type, (LPBYTE)buf, &cbData) == ERROR_SUCCESS) {
            if (wcslen(buf) > 0) g_username = buf;
        }
        RegCloseKey(hKey);
    }
}

void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\RetroChat", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Username", 0, REG_SZ, (const BYTE*)g_username.c_str(), (DWORD)((g_username.length() + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey);
    }
}

#pragma pack(push, 1)
struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t payloadType; // 1 = Audio, 2 = Video, 255 = HolePunch
    uint8_t flags;
    uint32_t timestamp;
    wchar_t username[32];
};
#pragma pack(pop)

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
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);
    IMFSourceReader* pReader = NULL;

    while (true) {
        if (!g_isWebcamActive || g_sock == INVALID_SOCKET) {
            if (pReader) {
                pReader->Release();
                pReader = NULL;
            }
            Sleep(100);
            continue;
        }

        if (!pReader) {
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

            if (pSource) {
                IMFAttributes* pReaderConfig = NULL;
                MFCreateAttributes(&pReaderConfig, 1);
                pReaderConfig->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
                MFCreateSourceReaderFromMediaSource(pSource, pReaderConfig, &pReader);
                pReaderConfig->Release();
                IMFMediaType* pType = NULL;
                MFCreateMediaType(&pType);
                pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
                pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
                pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
                pType->Release();
                pSource->Release();
            } else {
                g_isWebcamActive = false;
                continue;
            }
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
                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);
                long long delay_counts = freq.QuadPart / 1000; // 1 ms pacing delay

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
                    wcsncpy_s(mhdr->username, 32, g_username.c_str(), _TRUNCATE);
                    
                    memcpy(udpPkt + sizeof(MediaHeader), jpegData.data() + offset, len);
                    
                    sockaddr_in serverUdp = {0};
                    serverUdp.sin_family = AF_INET;
                    serverUdp.sin_port = htons(4001);
                    if (!g_isServerUdpAddrResolved) {
                        g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                    }
                    serverUdp.sin_addr = g_serverUdpAddr;
                    sendto(g_sock, udpPkt, sizeof(MediaHeader) + len, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));

                    LARGE_INTEGER start, current;
                    QueryPerformanceCounter(&start);
                    do {
                        QueryPerformanceCounter(&current);
                    } while (current.QuadPart - start.QuadPart < delay_counts);
                }
                DecodeJPEG(jpegData, g_username);
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


void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pHdr = (WAVEHDR*)dwParam1;
        if (pHdr->dwBytesRecorded > 0 && g_isVoiceActive && g_sock != INVALID_SOCKET) {
            short* samples = (short*)pHdr->lpData;
            int numSamples = pHdr->dwBytesRecorded / sizeof(short);
            long long sumSquares = 0;
            for(int i=0; i<numSamples; i++){
                sumSquares += samples[i] * samples[i];
            }
            if (numSamples > 0) {
                int rms = sqrt(sumSquares / numSamples);
                g_currentMicVolume = (rms * 100) / 32768;
                if (g_currentMicVolume > 100) g_currentMicVolume = 100;
                if (g_currentMicVolume < 0) g_currentMicVolume = 0;
                // Boost visual indicator slightly for low voices
                g_currentMicVolume = g_currentMicVolume * 3;
                if (g_currentMicVolume > 100) g_currentMicVolume = 100;
                InvalidateRect(g_hVolumeIndicator, NULL, FALSE);
            }

            char udpPkt[2048];
            MediaHeader* mhdr = (MediaHeader*)udpPkt;
            mhdr->sessionId = g_sessionId;
            mhdr->seqNum = 0;
            mhdr->payloadType = 1;
            mhdr->flags = 0;
            mhdr->timestamp = GetTickCount();
            wcsncpy_s(mhdr->username, 32, g_username.c_str(), _TRUNCATE);
            
            memcpy(udpPkt + sizeof(MediaHeader), pHdr->lpData, pHdr->dwBytesRecorded);
            
            sockaddr_in serverUdp = {0};
            serverUdp.sin_family = AF_INET;
            serverUdp.sin_port = htons(4001);
            if (!g_isServerUdpAddrResolved) {
                g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
            }
            serverUdp.sin_addr = g_serverUdpAddr;
            
            sendto(g_sock, udpPkt, sizeof(MediaHeader) + pHdr->dwBytesRecorded, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
        }
        waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR));
    }
}

void InitVolumeSync(HWND hWnd) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* pEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        IMMDevice* pDevice = nullptr;
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (SUCCEEDED(hr)) {
            hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&g_pAudioClient);
            if (SUCCEEDED(hr)) {
                WAVEFORMATEX* pwfx = nullptr;
                g_pAudioClient->GetMixFormat(&pwfx);
                hr = g_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST, 10000000, 0, pwfx, NULL);
                if (SUCCEEDED(hr)) {
                    g_pAudioClient->Start();
                    
                    g_pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&g_pSimpleAudioVolume);
                    g_pAudioClient->GetService(__uuidof(IAudioSessionControl), (void**)&g_pAudioSessionControl);
                    
                    if (g_pAudioSessionControl) {
                        g_pAudioEvents = new CAudioSessionEvents();
                        g_pAudioSessionControl->RegisterAudioSessionNotification(g_pAudioEvents);
                    }
                    if (g_pSimpleAudioVolume) {
                        float vol = 0.0f;
                        g_pSimpleAudioVolume->GetMasterVolume(&vol);
                        SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, (LPARAM)(vol * 100));
                    }
                }
                CoTaskMemFree(pwfx);
            }
            pDevice->Release();
        }
        pEnumerator->Release();
    }
}

void InitAudio() {
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 16000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    waveInOpen(&g_hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

    for (int i = 0; i < AUDIO_BUF_COUNT; i++) {
        g_waveInHdr[i].lpData = g_waveInBuf[i];
        g_waveInHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
        g_waveInHdr[i].dwFlags = 0;
        waveInPrepareHeader(g_hWaveIn, &g_waveInHdr[i], sizeof(WAVEHDR));
        waveInAddBuffer(g_hWaveIn, &g_waveInHdr[i], sizeof(WAVEHDR));
    }
    
    for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
        g_waveOutHdr[i].lpData = g_waveOutBuf[i];
        g_waveOutHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
        g_waveOutHdr[i].dwFlags = 0;
        waveOutPrepareHeader(g_hWaveOut, &g_waveOutHdr[i], sizeof(WAVEHDR));
    }
}

void PlayAudioFrame(const char* data, int len) {
    if (!g_hWaveOut) return;
    
    int freeIdx = -1;
    for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
        if ((g_waveOutHdr[i].dwFlags & WHDR_DONE) || !(g_waveOutHdr[i].dwFlags & WHDR_INQUEUE)) {
            freeIdx = i;
            break;
        }
    }
    if (freeIdx == -1) return; 
    
    memcpy(g_waveOutBuf[freeIdx], data, min(len, AUDIO_BUF_SIZE));
    g_waveOutHdr[freeIdx].dwBufferLength = len;
    waveOutWrite(g_hWaveOut, &g_waveOutHdr[freeIdx], sizeof(WAVEHDR));
}





struct MsgData {
    std::wstring user;
    std::wstring msg;
    bool b;
    bool i;
    bool u;
    int size;
    COLORREF color;
    bool isSystemEvent = false;
};

struct LogEntry {
    MsgData data;
    IDWriteTextLayout* pLayout = nullptr;
    float height = 0;
};
std::vector<LogEntry> g_logEntries;
float g_logScrollY = 0.0f;
float g_logTotalHeight = 0.0f;
bool g_autoScroll = true;

std::wstring g_inputText;
int g_caretPos = 0;
int g_anchorPos = 0;
bool g_caretVisible = true;
bool g_isIMEComposing = false;
UINT_PTR g_caretTimer = 0;
bool g_isDragging = false;

struct SelectionState {
    int startEntryIndex = -1;
    int startCharIndex = -1;
    int endEntryIndex = -1;
    int endCharIndex = -1;
    bool isSelecting = false;
};
SelectionState g_logSelection;

bool GetTextPositionFromPoint(float x, float y, int& outEntryIndex, int& outCharIndex) {
    float currentY = -g_logScrollY;
    for (size_t i = 0; i < g_logEntries.size(); ++i) {
        auto& entry = g_logEntries[i];
        if (entry.pLayout) {
            if (y >= currentY && y <= currentY + entry.height) {
                outEntryIndex = (int)i;
                BOOL isTrailingHit = FALSE;
                BOOL isInside = FALSE;
                DWRITE_HIT_TEST_METRICS metrics;
                entry.pLayout->HitTestPoint(x, y - currentY, &isTrailingHit, &isInside, &metrics);
                outCharIndex = metrics.textPosition + (isTrailingHit ? 1 : 0);
                return true;
            }
            currentY += entry.height;
        }
    }
    if (!g_logEntries.empty() && y > currentY) {
        outEntryIndex = (int)g_logEntries.size() - 1;
        auto& entry = g_logEntries[outEntryIndex];
        if (entry.pLayout) {
            BOOL isTrailingHit = FALSE, isInside = FALSE;
            DWRITE_HIT_TEST_METRICS metrics;
            entry.pLayout->HitTestPoint(9999.0f, 9999.0f, &isTrailingHit, &isInside, &metrics);
            outCharIndex = metrics.textPosition + (isTrailingHit ? 1 : 0);
            return true;
        }
    }
    if (!g_logEntries.empty() && y < -g_logScrollY) {
        outEntryIndex = 0;
        outCharIndex = 0;
        return true;
    }
    return false;
}

int GetCharType(wchar_t c) {
    if (iswspace(c)) return 0; // Space
    if (iswalnum(c)) return 1; // Alphanumeric
    if (c >= 0x3041 && c <= 0x309F) return 2; // Hiragana
    if (c >= 0x30A0 && c <= 0x30FF) return 3; // Katakana
    if ((c >= 0x4E00 && c <= 0x9FFF) || (c >= 0x3400 && c <= 0x4DBF)) return 4; // Kanji
    return 5; // Symbols/Punctuation
}

void CopySelectedText(HWND hWnd) {
    if (g_logSelection.startEntryIndex != -1 && g_logSelection.endEntryIndex != -1) {
        int sEntry = g_logSelection.startEntryIndex;
        int sChar = g_logSelection.startCharIndex;
        int eEntry = g_logSelection.endEntryIndex;
        int eChar = g_logSelection.endCharIndex;
        if (sEntry > eEntry || (sEntry == eEntry && sChar > eChar)) {
            std::swap(sEntry, eEntry);
            std::swap(sChar, eChar);
        }
        std::wstring copyText;
        for (int i = sEntry; i <= eEntry; ++i) {
            if (i >= 0 && i < g_logEntries.size()) {
                auto& entry = g_logEntries[i];
                std::wstring fullText = entry.data.user.empty() ? entry.data.msg : (entry.data.user + L": " + entry.data.msg);
                if (fullText.empty()) fullText = L" ";
                int startC = (i == sEntry) ? sChar : 0;
                int endC = (i == eEntry) ? eChar : (int)fullText.length();
                if (startC < 0) startC = 0;
                if (endC > (int)fullText.length()) endC = (int)fullText.length();
                if (startC < endC) {
                    copyText += fullText.substr(startC, endC - startC);
                }
                if (i < eEntry) copyText += L"\r\n";
            }
        }
        if (!copyText.empty() && OpenClipboard(hWnd)) {
            EmptyClipboard();
            HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copyText.length() + 1) * sizeof(WCHAR));
            if (hGlob) {
                memcpy(GlobalLock(hGlob), copyText.c_str(), (copyText.length() + 1) * sizeof(WCHAR));
                GlobalUnlock(hGlob);
                SetClipboardData(CF_UNICODETEXT, hGlob);
            }
            CloseClipboard();
        }
    }
}

LRESULT CALLBACK D2DLogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static ID2D1HwndRenderTarget* pRT = nullptr;
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        
        if (!pRT) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            g_pD2DFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hWnd, size),
                &pRT);
            if (pRT) {
                pRT->SetDpi((float)g_dpi, (float)g_dpi);
            }
        }
        
        if (pRT) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            float width = (float)(rc.right - rc.left) * 96.0f / g_dpi;
            float viewH = (float)(rc.bottom - rc.top) * 96.0f / g_dpi;
            if (width <= 0) width = 1;
            
            g_logTotalHeight = 0;
            for (auto& entry : g_logEntries) {
                if (!entry.pLayout) {
                    IDWriteTextFormat* pFormat = nullptr;
                    HRESULT hr = g_pDWriteFactory->CreateTextFormat(
                        L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                        12.0f * 1.5f, L"ja-JP", &pFormat); // MS PGothic
                        
                    if (SUCCEEDED(hr) && pFormat) {
                        std::wstring fullText;
                        int msgStart = 0;
                        std::wstring displayUser = entry.data.user;
                        size_t hashPos = displayUser.find(L'#');
                        if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);

                        if (displayUser.empty()) {
                            fullText = entry.data.msg;
                        } else {
                            if (entry.data.isSystemEvent) {
                                fullText = displayUser + entry.data.msg;
                                msgStart = (int)displayUser.length();
                            } else {
                                fullText = displayUser + L": " + entry.data.msg;
                                msgStart = (int)displayUser.length() + 2;
                            }
                        }
                        
                        if (fullText.empty()) fullText = L" ";
                        
                        hr = g_pDWriteFactory->CreateTextLayout(
                            fullText.c_str(), (UINT32)fullText.length(), pFormat, width, 10000.0f, &entry.pLayout);
                            
                        if (SUCCEEDED(hr) && entry.pLayout) {
                            DWRITE_TEXT_RANGE range = {(UINT32)msgStart, (UINT32)entry.data.msg.length()};
                            
                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                            if (entry.data.i) entry.pLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, range);
                            if (entry.data.u) entry.pLayout->SetUnderline(TRUE, range);
                            
                            entry.pLayout->SetFontSize(entry.data.size * 1.5f, range);
                            
                            ID2D1SolidColorBrush* pMsgBrush = nullptr;
                            if (SUCCEEDED(pRT->CreateSolidColorBrush(
                                D2D1::ColorF(GetRValue(entry.data.color)/255.0f, GetGValue(entry.data.color)/255.0f, GetBValue(entry.data.color)/255.0f), 
                                &pMsgBrush)) && pMsgBrush) {
                                entry.pLayout->SetDrawingEffect(pMsgBrush, range);
                                pMsgBrush->Release();
                            }
                            
                            if (msgStart > 0) {
                                DWRITE_TEXT_RANGE userRange = {0, (UINT32)(entry.data.isSystemEvent ? msgStart : msgStart - 2)};
                                size_t hash = 0;
                                for (wchar_t c : entry.data.user) hash = hash * 31 + c;
                                COLORREF palette[] = {
                                    RGB(200, 50, 50), RGB(50, 150, 50), RGB(50, 50, 200),
                                    RGB(180, 180, 50), RGB(180, 50, 180), RGB(50, 180, 180),
                                    RGB(255, 128, 0), RGB(128, 0, 255), RGB(0, 128, 255),
                                    RGB(128, 200, 0), RGB(255, 0, 128), RGB(0, 200, 128)
                                };
                                COLORREF uCol = palette[hash % 12];
                                ID2D1SolidColorBrush* pUserBrush = nullptr;
                                if (SUCCEEDED(pRT->CreateSolidColorBrush(
                                    D2D1::ColorF(GetRValue(uCol)/255.0f, GetGValue(uCol)/255.0f, GetBValue(uCol)/255.0f), 
                                    &pUserBrush)) && pUserBrush) {
                                    entry.pLayout->SetDrawingEffect(pUserBrush, userRange);
                                    entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, userRange);
                                    pUserBrush->Release();
                                }
                            }
                            
                            DWRITE_TEXT_METRICS metrics;
                            entry.pLayout->GetMetrics(&metrics);
                            entry.height = metrics.height;
                        }
                        pFormat->Release();
                    }
                }
                g_logTotalHeight += entry.height;
            }
            
            float maxScroll = g_logTotalHeight - viewH;
            if (maxScroll < 0) maxScroll = 0;
            
            if (g_autoScroll) {
                g_logScrollY = maxScroll;
            } else {
                if (g_logScrollY >= maxScroll - 1.0f) {
                    g_logScrollY = maxScroll;
                    g_autoScroll = true;
                }
                if (g_logScrollY < 0) g_logScrollY = 0;
            }

            SCROLLINFO si = { sizeof(si) };
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = (int)g_logTotalHeight;
            si.nPage = (int)viewH;
            si.nPos = (int)g_logScrollY;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

            pRT->BeginDraw();
            pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

            float y = -g_logScrollY;
            for (size_t i = 0; i < g_logEntries.size(); ++i) {
                auto& entry = g_logEntries[i];
                if (entry.pLayout && y + entry.height > 0 && y < viewH) {
                    
                    bool isSelected = false;
                    int startC = 0, endC = 0;
                    int sEntry = g_logSelection.startEntryIndex;
                    int sChar = g_logSelection.startCharIndex;
                    int eEntry = g_logSelection.endEntryIndex;
                    int eChar = g_logSelection.endCharIndex;
                    if (sEntry != -1 && eEntry != -1) {
                        if (sEntry > eEntry || (sEntry == eEntry && sChar > eChar)) {
                            std::swap(sEntry, eEntry);
                            std::swap(sChar, eChar);
                        }
                        if ((int)i >= sEntry && (int)i <= eEntry) {
                            isSelected = true;
                            std::wstring displayUser = entry.data.user;
                            size_t hashPos = displayUser.find(L'#');
                            if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);

                            std::wstring fullText;
                            if (displayUser.empty()) {
                                fullText = entry.data.msg;
                            } else {
                                if (entry.data.isSystemEvent) {
                                    fullText = displayUser + entry.data.msg;
                                } else {
                                    fullText = displayUser + L": " + entry.data.msg;
                                }
                            }
                            if (fullText.empty()) fullText = L" ";
                            startC = ((int)i == sEntry) ? sChar : 0;
                            endC = ((int)i == eEntry) ? eChar : (int)fullText.length();
                            if (startC < endC) {
                                UINT32 actualHitTestCount = 0;
                                entry.pLayout->HitTestTextRange(startC, endC - startC, 0, 0, nullptr, 0, &actualHitTestCount);
                                if (actualHitTestCount > 0) {
                                    std::vector<DWRITE_HIT_TEST_METRICS> hitMetrics(actualHitTestCount);
                                    entry.pLayout->HitTestTextRange(startC, endC - startC, 0, 0, hitMetrics.data(), actualHitTestCount, &actualHitTestCount);
                                    DWORD hlColor = GetSysColor(COLOR_HIGHLIGHT);
                                    float r = GetRValue(hlColor) / 255.0f;
                                    float g = GetGValue(hlColor) / 255.0f;
                                    float b = GetBValue(hlColor) / 255.0f;
                                    ID2D1SolidColorBrush* pBgBrush = nullptr;
                                    if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f), &pBgBrush))) {
                                        for (UINT32 j = 0; j < actualHitTestCount; ++j) {
                                            D2D1_RECT_F rect = D2D1::RectF(
                                                hitMetrics[j].left,
                                                y + hitMetrics[j].top,
                                                hitMetrics[j].left + hitMetrics[j].width,
                                                y + hitMetrics[j].top + hitMetrics[j].height
                                            );
                                            pRT->FillRectangle(&rect, pBgBrush);
                                        }
                                        pBgBrush->Release();
                                    }

                                    DWORD hlText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                                    float tr = GetRValue(hlText) / 255.0f;
                                    float tg = GetGValue(hlText) / 255.0f;
                                    float tb = GetBValue(hlText) / 255.0f;
                                    ID2D1SolidColorBrush* pTextBrush = nullptr;
                                    if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(tr, tg, tb, 1.0f), &pTextBrush)) && pTextBrush) {
                                        DWRITE_TEXT_RANGE selRange = { (UINT32)startC, (UINT32)(endC - startC) };
                                        entry.pLayout->SetDrawingEffect(pTextBrush, selRange);
                                        pTextBrush->Release();
                                    }
                                }
                            }
                        }
                    }

                    ID2D1SolidColorBrush* pBrush = nullptr;
                    if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrush)) && pBrush) {
                        pRT->DrawTextLayout(D2D1::Point2F(0, y), entry.pLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                        pBrush->Release();
                    }

                    if (isSelected && startC < endC) {
                        std::wstring displayUser = entry.data.user;
                        size_t hashPos = displayUser.find(L'#');
                        if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);

                        std::wstring fullText;
                        int msgStart = 0;
                        if (displayUser.empty()) {
                            fullText = entry.data.msg;
                        } else {
                            if (entry.data.isSystemEvent) {
                                fullText = displayUser + entry.data.msg;
                                msgStart = (int)displayUser.length();
                            } else {
                                fullText = displayUser + L": " + entry.data.msg;
                                msgStart = (int)displayUser.length() + 2;
                            }
                        }
                        if (fullText.empty()) fullText = L" ";

                        DWRITE_TEXT_RANGE fullRange = { 0, (UINT32)fullText.length() };
                        entry.pLayout->SetDrawingEffect(nullptr, fullRange);

                        if (msgStart < fullText.length()) {
                            DWRITE_TEXT_RANGE msgRange = { (UINT32)msgStart, (UINT32)entry.data.msg.length() };
                            ID2D1SolidColorBrush* pMsgBrush = nullptr;
                            if (SUCCEEDED(pRT->CreateSolidColorBrush(
                                D2D1::ColorF(GetRValue(entry.data.color)/255.0f, GetGValue(entry.data.color)/255.0f, GetBValue(entry.data.color)/255.0f), 
                                &pMsgBrush)) && pMsgBrush) {
                                entry.pLayout->SetDrawingEffect(pMsgBrush, msgRange);
                                pMsgBrush->Release();
                            }
                        }
                        
                        if (msgStart > 0) {
                            DWRITE_TEXT_RANGE userRange = {0, (UINT32)(entry.data.isSystemEvent ? msgStart : msgStart - 2)};
                            size_t hash = 0;
                            for (wchar_t c : entry.data.user) hash = hash * 31 + c;
                            COLORREF palette[] = {
                                RGB(200, 50, 50), RGB(50, 150, 50), RGB(50, 50, 200),
                                RGB(180, 180, 50), RGB(180, 50, 180), RGB(50, 180, 180),
                                RGB(255, 128, 0), RGB(128, 0, 255), RGB(0, 128, 255),
                                RGB(128, 200, 0), RGB(255, 0, 128), RGB(0, 200, 128)
                            };
                            COLORREF uCol = palette[hash % 12];
                            ID2D1SolidColorBrush* pUserBrush = nullptr;
                            if (SUCCEEDED(pRT->CreateSolidColorBrush(
                                D2D1::ColorF(GetRValue(uCol)/255.0f, GetGValue(uCol)/255.0f, GetBValue(uCol)/255.0f), 
                                &pUserBrush)) && pUserBrush) {
                                entry.pLayout->SetDrawingEffect(pUserBrush, userRange);
                                entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, userRange);
                                pUserBrush->Release();
                            }
                        }
                    }
                }
                y += entry.height;
            }
            
            HRESULT hr = pRT->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET) {
                pRT->Release(); pRT = nullptr;
                for (auto& entry : g_logEntries) {
                    if (entry.pLayout) { entry.pLayout->Release(); entry.pLayout = nullptr; }
                }
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SIZE:
        if (pRT) {
            pRT->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        }
        for (auto& entry : g_logEntries) {
            if (entry.pLayout) { entry.pLayout->Release(); entry.pLayout = nullptr; }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_VSCROLL: {
        int action = LOWORD(wParam);
        if (action == SB_LINEUP) g_logScrollY -= 20;
        else if (action == SB_LINEDOWN) g_logScrollY += 20;
        else if (action == SB_PAGEUP) g_logScrollY -= 100;
        else if (action == SB_PAGEDOWN) g_logScrollY += 100;
        else if (action == SB_THUMBTRACK) {
            SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
            GetScrollInfo(hWnd, SB_VERT, &si);
            g_logScrollY = si.nTrackPos;
        }
        g_autoScroll = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        short delta = HIWORD(wParam);
        g_logScrollY -= (delta / 120.0f) * 40.0f;
        g_autoScroll = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        SetFocus(hWnd);
        float x = (float)GET_X_LPARAM(lParam) * 96.0f / g_dpi;
        float y = (float)GET_Y_LPARAM(lParam) * 96.0f / g_dpi;
        int entryIndex, charIndex;
        if (GetTextPositionFromPoint(x, y, entryIndex, charIndex)) {
            g_logSelection.startEntryIndex = entryIndex;
            g_logSelection.startCharIndex = charIndex;
            g_logSelection.endEntryIndex = entryIndex;
            g_logSelection.endCharIndex = charIndex;
            g_logSelection.isSelecting = true;
            SetCapture(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        } else {
            g_logSelection.startEntryIndex = -1;
            g_logSelection.isSelecting = false;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_logSelection.isSelecting) {
            float x = (float)GET_X_LPARAM(lParam) * 96.0f / g_dpi;
            float y = (float)GET_Y_LPARAM(lParam) * 96.0f / g_dpi;
            int entryIndex, charIndex;
            if (GetTextPositionFromPoint(x, y, entryIndex, charIndex)) {
                g_logSelection.endEntryIndex = entryIndex;
                g_logSelection.endCharIndex = charIndex;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (g_logSelection.isSelecting) {
            g_logSelection.isSelecting = false;
            ReleaseCapture();
        }
        return 0;
    }
    case WM_KEYDOWN: {
        if (wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            CopySelectedText(hWnd);
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        float x = (float)GET_X_LPARAM(lParam) * 96.0f / g_dpi;
        float y = (float)GET_Y_LPARAM(lParam) * 96.0f / g_dpi;
        int entryIndex, charIndex;
        if (GetTextPositionFromPoint(x, y, entryIndex, charIndex)) {
            if (entryIndex >= 0 && entryIndex < g_logEntries.size()) {
                auto& entry = g_logEntries[entryIndex];
                std::wstring fullText = entry.data.user.empty() ? entry.data.msg : (entry.data.user + L": " + entry.data.msg);
                if (!fullText.empty() && charIndex >= 0 && charIndex < fullText.length()) {
                    int cType = GetCharType(fullText[charIndex]);
                    int startC = charIndex;
                    int endC = charIndex + 1;
                    while (startC > 0 && GetCharType(fullText[startC - 1]) == cType) {
                        startC--;
                    }
                    while (endC < fullText.length() && GetCharType(fullText[endC]) == cType) {
                        endC++;
                    }
                    g_logSelection.startEntryIndex = entryIndex;
                    g_logSelection.startCharIndex = startC;
                    g_logSelection.endEntryIndex = entryIndex;
                    g_logSelection.endCharIndex = endC;
                    g_logSelection.isSelecting = false;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
        }
        return 0;
    }
    case WM_CONTEXTMENU: {
        bool hasSelection = (g_logSelection.startEntryIndex != -1 && g_logSelection.endEntryIndex != -1);
        if (hasSelection && g_logSelection.startEntryIndex == g_logSelection.endEntryIndex && g_logSelection.startCharIndex == g_logSelection.endCharIndex) {
            hasSelection = false;
        }
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING | (hasSelection ? 0 : MF_GRAYED), 1001, L"コピー(C)");
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        if (xPos == -1 && yPos == -1) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            POINT pt = { rc.left, rc.top };
            ClientToScreen(hWnd, &pt);
            xPos = pt.x;
            yPos = pt.y;
        }
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, xPos, yPos, 0, hWnd, NULL);
        DestroyMenu(hMenu);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1001) {
            CopySelectedText(hWnd);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

IDWriteTextLayout* CreateInputLayout(HWND hWnd) {
    if (!g_pDWriteFactory) return nullptr;
    RECT rc;
    GetClientRect(hWnd, &rc);
    float width = (float)(rc.right - rc.left);
    float height = (float)(rc.bottom - rc.top);
    if (width <= 0) width = 1;
    
    bool b = SendMessage(g_hBtnB, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool i = SendMessage(g_hBtnI, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    int sizeIndex = (int)SendMessage(g_hComboSize, CB_GETCURSEL, 0, 0);
    int tsize = 12;
    if (sizeIndex != CB_ERR) {
        WCHAR szText[32];
        SendMessage(g_hComboSize, CB_GETLBTEXT, sizeIndex, (LPARAM)szText);
        tsize = _wtoi(szText);
    }
    
    IDWriteTextFormat* pFormat = nullptr;
    DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE style = i ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    
    HRESULT hrFormat = g_pDWriteFactory->CreateTextFormat(
        L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF", NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
        tsize * 1.5f, L"ja-JP", &pFormat);
        
    IDWriteTextLayout* pLayout = nullptr;
    if (SUCCEEDED(hrFormat) && pFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), pFormat, width, height, &pLayout);
        pFormat->Release();
    }
    return pLayout;
}

LRESULT CALLBACK D2DInputProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static ID2D1HwndRenderTarget* pRT = nullptr;
    switch (msg) {
    case WM_CREATE:
        g_caretTimer = SetTimer(hWnd, 1, 500, NULL);
        return 0;
    case WM_TIMER:
        g_caretVisible = !g_caretVisible;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        SetFocus(hWnd);
        SetCapture(hWnd);
        g_isDragging = true;
        
        int x = GET_X_LPARAM(lParam) - 5;
        int y = GET_Y_LPARAM(lParam) - 5;
        
        IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
        if (pLayout) {
            BOOL isTrailingHit = FALSE;
            BOOL isInside = FALSE;
            DWRITE_HIT_TEST_METRICS hm = {0};
            if (SUCCEEDED(pLayout->HitTestPoint((FLOAT)x, (FLOAT)y, &isTrailingHit, &isInside, &hm))) {
                g_caretPos = hm.textPosition;
                if (isTrailingHit) g_caretPos += hm.length;
                if (g_caretPos > g_inputText.length()) g_caretPos = (int)g_inputText.length();
                
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (!shift) g_anchorPos = g_caretPos;
            }
            pLayout->Release();
        }
        g_caretVisible = true;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_isDragging) {
            int x = GET_X_LPARAM(lParam) - 5;
            int y = GET_Y_LPARAM(lParam) - 5;
            
            IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
            if (pLayout) {
                BOOL isTrailingHit = FALSE;
                BOOL isInside = FALSE;
                DWRITE_HIT_TEST_METRICS hm = {0};
                if (SUCCEEDED(pLayout->HitTestPoint((FLOAT)x, (FLOAT)y, &isTrailingHit, &isInside, &hm))) {
                    g_caretPos = hm.textPosition;
                    if (isTrailingHit) g_caretPos += hm.length;
                    if (g_caretPos > g_inputText.length()) g_caretPos = (int)g_inputText.length();
                }
                pLayout->Release();
            }
            g_caretVisible = true;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (g_isDragging) {
            g_isDragging = false;
            ReleaseCapture();
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        int x = GET_X_LPARAM(lParam) - 5;
        int y = GET_Y_LPARAM(lParam) - 5;
        
        IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
        if (pLayout) {
            BOOL isTrailingHit = FALSE;
            BOOL isInside = FALSE;
            DWRITE_HIT_TEST_METRICS hm = {0};
            if (SUCCEEDED(pLayout->HitTestPoint((FLOAT)x, (FLOAT)y, &isTrailingHit, &isInside, &hm))) {
                int clickedPos = hm.textPosition;
                
                auto IsWordBoundary = [](WCHAR c) {
                    return c == L' ' || c == L'　' || c == L'、' || c == L'。' || c == L',' || c == L'.' || c == L'!' || c == L'?' || c == L'！' || c == L'？' || c == L'(' || c == L')' || c == L'（' || c == L'）';
                };
                
                if (clickedPos < g_inputText.length()) {
                    if (IsWordBoundary(g_inputText[clickedPos])) {
                        g_anchorPos = clickedPos;
                        g_caretPos = clickedPos + 1;
                    } else {
                        int start = clickedPos;
                        while (start > 0 && !IsWordBoundary(g_inputText[start - 1])) {
                            start--;
                        }
                        int end = clickedPos;
                        while (end < g_inputText.length() && !IsWordBoundary(g_inputText[end])) {
                            end++;
                        }
                        g_anchorPos = start;
                        g_caretPos = end;
                    }
                }
            }
            pLayout->Release();
        }
        g_caretVisible = true;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_SETFOCUS:
        g_caretVisible = true;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_KILLFOCUS:
        g_caretVisible = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_IME_STARTCOMPOSITION:
        g_isIMEComposing = true;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_IME_ENDCOMPOSITION:
        g_isIMEComposing = false;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        
        if (!pRT) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            g_pD2DFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hWnd, size),
                &pRT);
        }
        
        if (pRT) {
            pRT->BeginDraw();
            pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
            
            IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
            if (pLayout) {
                bool u = SendMessage(g_hBtnU, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (u && g_inputText.length() > 0) {
                    DWRITE_TEXT_RANGE range = {0, (UINT32)g_inputText.length()};
                    pLayout->SetUnderline(TRUE, range);
                }
                
                if (g_caretPos != g_anchorPos) {
                    int selStart = min(g_caretPos, g_anchorPos);
                    int selLen = max(g_caretPos, g_anchorPos) - selStart;
                    if (selLen > 0) {
                        UINT32 actualHitTestCount = 0;
                        DWRITE_HIT_TEST_METRICS* hitTestMetrics = new DWRITE_HIT_TEST_METRICS[10];
                        HRESULT hrHit = pLayout->HitTestTextRange(selStart, selLen, 5, 5, hitTestMetrics, 10, &actualHitTestCount);
                        if (hrHit == E_NOT_SUFFICIENT_BUFFER) {
                            delete[] hitTestMetrics;
                            hitTestMetrics = new DWRITE_HIT_TEST_METRICS[actualHitTestCount];
                            pLayout->HitTestTextRange(selStart, selLen, 5, 5, hitTestMetrics, actualHitTestCount, &actualHitTestCount);
                        }
                        
                        DWORD hlColor = GetSysColor(COLOR_HIGHLIGHT);
                        float r = GetRValue(hlColor) / 255.0f;
                        float g = GetGValue(hlColor) / 255.0f;
                        float b = GetBValue(hlColor) / 255.0f;
                        ID2D1SolidColorBrush* pSelBrush = nullptr;
                        if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f), &pSelBrush)) && pSelBrush) {
                            for (UINT32 i = 0; i < actualHitTestCount; i++) {
                                D2D1_RECT_F rect = D2D1::RectF(
                                    hitTestMetrics[i].left,
                                    hitTestMetrics[i].top,
                                    hitTestMetrics[i].left + hitTestMetrics[i].width,
                                    hitTestMetrics[i].top + hitTestMetrics[i].height
                                );
                                pRT->FillRectangle(rect, pSelBrush);
                            }
                            pSelBrush->Release();
                        }

                        DWORD hlText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        float tr = GetRValue(hlText) / 255.0f;
                        float tg = GetGValue(hlText) / 255.0f;
                        float tb = GetBValue(hlText) / 255.0f;
                        ID2D1SolidColorBrush* pTextBrush = nullptr;
                        if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(tr, tg, tb, 1.0f), &pTextBrush)) && pTextBrush) {
                            DWRITE_TEXT_RANGE selRange = { (UINT32)selStart, (UINT32)selLen };
                            pLayout->SetDrawingEffect(pTextBrush, selRange);
                            pTextBrush->Release();
                        }
                        delete[] hitTestMetrics;
                    }
                }
                
                ID2D1SolidColorBrush* pBrush = nullptr;
                if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(GetRValue(g_currentColor)/255.0f, GetGValue(g_currentColor)/255.0f, GetBValue(g_currentColor)/255.0f), &pBrush)) && pBrush) {
                    pRT->DrawTextLayout(D2D1::Point2F(5, 5), pLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                    
                    if (g_caretVisible && GetFocus() == hWnd && !g_isIMEComposing) {
                        float cx = 5, cy = 5;
                        DWRITE_HIT_TEST_METRICS hm = {0};
                        if (g_inputText.length() == 0) {
                            int sizeIndex = (int)SendMessage(g_hComboSize, CB_GETCURSEL, 0, 0);
                            int tsize = 12;
                            if (sizeIndex != CB_ERR) {
                                WCHAR szText[32];
                                SendMessage(g_hComboSize, CB_GETLBTEXT, sizeIndex, (LPARAM)szText);
                                tsize = _wtoi(szText);
                            }
                            cx = 5; cy = 5; hm.height = tsize * 1.5f;
                        } else if (g_caretPos >= g_inputText.length()) {
                            pLayout->HitTestTextPosition((UINT32)g_inputText.length() - 1, TRUE, &cx, &cy, &hm);
                            cx += 5; cy += 5;
                        } else {
                            pLayout->HitTestTextPosition((UINT32)g_caretPos, FALSE, &cx, &cy, &hm);
                            cx += 5; cy += 5;
                        }
                        
                        ID2D1SolidColorBrush* pCaretBrush = nullptr;
                        if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pCaretBrush)) && pCaretBrush) {
                            D2D1_ANTIALIAS_MODE oldMode = pRT->GetAntialiasMode();
                            pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                            pRT->DrawLine(D2D1::Point2F(cx, cy), D2D1::Point2F(cx, cy + hm.height), pCaretBrush, 2.0f * 96.0f / g_dpi);
                            pRT->SetAntialiasMode(oldMode);
                            pCaretBrush->Release();
                        }

                        HIMC hImc = ImmGetContext(hWnd);
                        if (hImc) {
                            COMPOSITIONFORM cf = { 0 };
                            cf.dwStyle = CFS_POINT;
                            cf.ptCurrentPos.x = Scale((LONG)cx);
                            cf.ptCurrentPos.y = Scale((LONG)cy);
                            ImmSetCompositionWindow(hImc, &cf);

                            LOGFONTW lf = { 0 };
                            lf.lfHeight = -Scale((LONG)(hm.height));
                            wcscpy_s(lf.lfFaceName, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF");
                            ImmSetCompositionFontW(hImc, &lf);

                            ImmReleaseContext(hWnd, hImc);
                        }
                    }
                    pBrush->Release();
                }
                pLayout->Release();
            }
            
            HRESULT hr = pRT->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET) {
                pRT->Release(); pRT = nullptr;
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SIZE:
        if (pRT) {
            pRT->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_CHAR: {
        if (wParam >= 0x20) {
            if (g_caretPos != g_anchorPos) {
                int start = min(g_caretPos, g_anchorPos);
                int end = max(g_caretPos, g_anchorPos);
                g_inputText.erase(start, end - start);
                g_caretPos = start;
                g_anchorPos = start;
            }
            if (g_inputText.length() < 1024) {
                g_inputText.insert(g_caretPos, 1, (WCHAR)wParam);
                g_caretPos++;
                g_anchorPos = g_caretPos;
                g_caretVisible = true;
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                MessageBeep(MB_OK);
            }
        }
        return 0;
    }
    case WM_KEYDOWN: {
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool hasSelection = (g_caretPos != g_anchorPos);
        
        IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
        if (wParam == VK_LEFT) {
            if (!shift && hasSelection) {
                g_caretPos = min(g_caretPos, g_anchorPos);
                g_anchorPos = g_caretPos;
            } else if (g_caretPos > 0) {
                if (pLayout && g_inputText.length() > 0) {
                    DWRITE_HIT_TEST_METRICS hm = {0}; float cx, cy;
                    pLayout->HitTestTextPosition(g_caretPos - 1, FALSE, &cx, &cy, &hm);
                    g_caretPos = hm.textPosition;
                } else {
                    g_caretPos--;
                }
                if (!shift) g_anchorPos = g_caretPos;
            }
        } else if (wParam == VK_RIGHT) {
            if (!shift && hasSelection) {
                g_caretPos = max(g_caretPos, g_anchorPos);
                g_anchorPos = g_caretPos;
            } else if (g_caretPos < g_inputText.length()) {
                if (pLayout && g_inputText.length() > 0) {
                    DWRITE_HIT_TEST_METRICS hm = {0}; float cx, cy;
                    pLayout->HitTestTextPosition(g_caretPos, FALSE, &cx, &cy, &hm);
                    if (hm.length > 0) g_caretPos = hm.textPosition + hm.length;
                    else g_caretPos++;
                } else {
                    g_caretPos++;
                }
                if (!shift) g_anchorPos = g_caretPos;
            }
        } else if (wParam == VK_HOME) {
            g_caretPos = 0;
            if (!shift) g_anchorPos = g_caretPos;
        } else if (wParam == VK_END) {
            g_caretPos = (int)g_inputText.length();
            if (!shift) g_anchorPos = g_caretPos;
        } else if (wParam == VK_BACK) {
            if (hasSelection) {
                int start = min(g_caretPos, g_anchorPos);
                int end = max(g_caretPos, g_anchorPos);
                g_inputText.erase(start, end - start);
                g_caretPos = start;
                g_anchorPos = start;
            } else if (g_caretPos > 0) {
                if (pLayout && g_inputText.length() > 0) {
                    DWRITE_HIT_TEST_METRICS hm = {0}; float cx, cy;
                    pLayout->HitTestTextPosition(g_caretPos - 1, FALSE, &cx, &cy, &hm);
                    g_caretPos = hm.textPosition;
                    g_anchorPos = g_caretPos;
                    g_inputText.erase(hm.textPosition, hm.length);
                } else {
                    g_caretPos--;
                    g_anchorPos = g_caretPos;
                    g_inputText.erase(g_caretPos, 1);
                }
            }
        } else if (wParam == VK_DELETE) {
            if (hasSelection) {
                int start = min(g_caretPos, g_anchorPos);
                int end = max(g_caretPos, g_anchorPos);
                g_inputText.erase(start, end - start);
                g_caretPos = start;
                g_anchorPos = start;
            } else if (g_caretPos < g_inputText.length()) {
                if (pLayout && g_inputText.length() > 0) {
                    DWRITE_HIT_TEST_METRICS hm = {0}; float cx, cy;
                    pLayout->HitTestTextPosition(g_caretPos, FALSE, &cx, &cy, &hm);
                    g_inputText.erase(hm.textPosition, hm.length);
                } else {
                    g_inputText.erase(g_caretPos, 1);
                }
            }
        } else if (wParam == VK_RETURN) {
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(1001, BN_CLICKED), 0);
        } else if (wParam == 'V' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_PASTE, 0, 0);
        } else if (wParam == 'C' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_COPY, 0, 0);
        } else if (wParam == 'X' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_CUT, 0, 0);
        } else if (wParam == 'A' && GetKeyState(VK_CONTROL) < 0) {
            g_anchorPos = 0;
            g_caretPos = (int)g_inputText.length();
        }
        if (pLayout) pLayout->Release();
        
        g_caretVisible = true;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_CONTEXTMENU: {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1, L"切り取り(T)");
        AppendMenuW(hMenu, MF_STRING, 2, L"コピー(C)");
        AppendMenuW(hMenu, MF_STRING, 3, L"貼り付け(P)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 4, L"すべて選択(A)");
        AppendMenuW(hMenu, MF_STRING, 5, L"削除(D)");
        
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
        
        if (cmd == 1) SendMessage(hWnd, WM_CUT, 0, 0);
        else if (cmd == 2) SendMessage(hWnd, WM_COPY, 0, 0);
        else if (cmd == 3) SendMessage(hWnd, WM_PASTE, 0, 0);
        else if (cmd == 4) {
            g_anchorPos = 0;
            g_caretPos = (int)g_inputText.length();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (cmd == 5) {
            SendMessage(hWnd, WM_KEYDOWN, VK_DELETE, 0);
        }
        return 0;
    }
    case WM_PASTE: {
        if (OpenClipboard(hWnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                WCHAR* pszText = static_cast<WCHAR*>(GlobalLock(hData));
                if (pszText) {
                    if (g_caretPos != g_anchorPos) {
                        int start = min(g_caretPos, g_anchorPos);
                        int end = max(g_caretPos, g_anchorPos);
                        g_inputText.erase(start, end - start);
                        g_caretPos = start;
                        g_anchorPos = start;
                    }
                    std::wstring pasteText = pszText;
                    pasteText.erase(std::remove(pasteText.begin(), pasteText.end(), L'\r'), pasteText.end());
                    pasteText.erase(std::remove(pasteText.begin(), pasteText.end(), L'\n'), pasteText.end());
                    
                    if (g_inputText.length() + pasteText.length() > 1024) {
                        pasteText = pasteText.substr(0, 1024 - g_inputText.length());
                    }
                    if (pasteText.length() > 0) {
                        g_inputText.insert(g_caretPos, pasteText);
                        g_caretPos += (int)pasteText.length();
                        g_anchorPos = g_caretPos;
                    } else {
                        MessageBeep(MB_OK);
                    }
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_COPY: {
        int start = min(g_caretPos, g_anchorPos);
        int len = max(g_caretPos, g_anchorPos) - start;
        if (len > 0) {
            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(WCHAR));
                if (hData) {
                    WCHAR* pszText = static_cast<WCHAR*>(GlobalLock(hData));
                    wcsncpy_s(pszText, len + 1, g_inputText.c_str() + start, len);
                    GlobalUnlock(hData);
                    SetClipboardData(CF_UNICODETEXT, hData);
                }
                CloseClipboard();
            }
        }
        return 0;
    }
    case WM_CUT: {
        SendMessage(hWnd, WM_COPY, 0, 0);
        int start = min(g_caretPos, g_anchorPos);
        int end = max(g_caretPos, g_anchorPos);
        if (start != end) {
            g_inputText.erase(start, end - start);
            g_caretPos = start;
            g_anchorPos = start;
            g_caretVisible = true;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Dialogs

std::wstring g_changeNameResult;
bool g_changeNameOK = false;

LRESULT CALLBACK ChangeNameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit, hBtnOK, hBtnCancel;
    switch (msg) {
    case WM_CREATE: {
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            Scale(10), Scale(10), Scale(260), Scale(24), hWnd, 0, GetModuleHandle(0), 0);
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            Scale(100), Scale(45), Scale(80), Scale(24), hWnd, (HMENU)1, GetModuleHandle(0), 0);
        hBtnCancel = CreateWindow(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            Scale(190), Scale(45), Scale(80), Scale(24), hWnd, (HMENU)2, GetModuleHandle(0), 0);
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hBtnOK, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, FALSE);
        
        std::wstring dispName = g_username;
        size_t pos = dispName.find(L'#');
        if (pos != std::wstring::npos) dispName = dispName.substr(0, pos);
        SetWindowTextW(hEdit, dispName.c_str());
        SetFocus(hEdit);
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // OK
            WCHAR buf[256] = {0};
            GetWindowTextW(hEdit, buf, 255);
            std::wstring newName = buf;
            if (!newName.empty() && newName.find(L'#') == std::wstring::npos) {
                srand(GetTickCount());
                wchar_t tag[16];
                swprintf_s(tag, L"#%04d", rand() % 10000);
                newName += tag;
            }
            g_changeNameResult = newName;
            g_changeNameOK = true;
            DestroyWindow(hWnd);
        } else if (LOWORD(wParam) == 2) { // Cancel
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_DESTROY:
        EnableWindow(GetParent(hWnd), TRUE);
        SetForegroundWindow(GetParent(hWnd));
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ShowChangeNameDialog(HWND hParent, std::wstring& outResult) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ChangeNameProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = L"ChangeNameClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&wc);

    g_changeNameOK = false;
    g_changeNameResult = L"";

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);

    HWND hWnd = CreateWindowEx(WS_EX_DLGMODALFRAME, L"ChangeNameClass", GetString(IDS_CHANGE_NAME_TITLE).c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcParent.left + (rcParent.right - rcParent.left - Scale(300)) / 2, 
        rcParent.top + (rcParent.bottom - rcParent.top - Scale(120)) / 2, 
        Scale(300), Scale(120), hParent, 0, GetModuleHandle(0), 0);
    
    EnableWindow(hParent, FALSE);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    outResult = g_changeNameResult;
    return g_changeNameOK;
}



std::wstring g_moveBoxResult;
bool g_moveBoxPrivate = false;
bool g_moveBoxOK = false;
bool g_ignoreEditChange = false;

LRESULT CALLBACK MoveRoomProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit, hList, hChk, hBtnOK, hBtnCancel;
    static std::vector<std::wstring> s_roomNames;
    switch (msg) {
    case WM_CREATE: {
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            Scale(10), Scale(10), Scale(260), Scale(24), hWnd, (HMENU)101, GetModuleHandle(0), 0);
        hList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
            Scale(10), Scale(40), Scale(260), Scale(100), hWnd, (HMENU)102, GetModuleHandle(0), 0);
        hChk = CreateWindow(L"BUTTON", GetString(IDS_PRIVATE).c_str(), 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            Scale(10), Scale(145), Scale(80), Scale(24), hWnd, (HMENU)103, GetModuleHandle(0), 0);
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            Scale(100), Scale(145), Scale(80), Scale(24), hWnd, (HMENU)1, GetModuleHandle(0), 0);
        hBtnCancel = CreateWindow(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            Scale(190), Scale(145), Scale(80), Scale(24), hWnd, (HMENU)2, GetModuleHandle(0), 0);
        
        s_roomNames.clear();
        for (const auto& r : g_publicRooms) {
            std::wstring display = r.first + L" (" + std::to_wstring(r.second) + L")";
            int idx = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
            s_roomNames.push_back(r.first);
            SendMessage(hList, LB_SETITEMDATA, idx, (LPARAM)(s_roomNames.size() - 1));
        }

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hList, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hChk, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hBtnOK, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, FALSE);
        SetFocus(hEdit);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == 1) { // OK
            int len = GetWindowTextLength(hEdit);
            std::vector<WCHAR> buf(len + 1);
            GetWindowText(hEdit, buf.data(), len + 1);
            g_moveBoxResult = buf.data();
            g_moveBoxPrivate = SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED;
            g_moveBoxOK = true;
            DestroyWindow(hWnd);
        } else if (id == 2) { // Cancel
            g_moveBoxOK = false;
            DestroyWindow(hWnd);
        } else if (id == 102 && code == LBN_SELCHANGE) {
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                int dataIdx = (int)SendMessage(hList, LB_GETITEMDATA, sel, 0);
                if (dataIdx >= 0 && dataIdx < (int)s_roomNames.size()) {
                    g_ignoreEditChange = true;
                    SetWindowText(hEdit, s_roomNames[dataIdx].c_str());
                    g_ignoreEditChange = false;
                }
            }
        } else if (id == 101 && code == EN_CHANGE) {
            if (g_ignoreEditChange) break;
            int len = GetWindowTextLength(hEdit);
            std::vector<WCHAR> buf(len + 1);
            GetWindowText(hEdit, buf.data(), len + 1);
            std::wstring search = buf.data();
            
            SendMessage(hList, LB_RESETCONTENT, 0, 0);
            s_roomNames.clear();
            for (const auto& r : g_publicRooms) {
                if (search.empty() || r.first.find(search) != std::wstring::npos) {
                    std::wstring display = r.first + L" (" + std::to_wstring(r.second) + L")";
                    int idx = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                    s_roomNames.push_back(r.first);
                    SendMessage(hList, LB_SETITEMDATA, idx, (LPARAM)(s_roomNames.size() - 1));
                }
            }
        }
        break;
    }
    case WM_CLOSE:
        g_moveBoxOK = false;
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ShowMoveRoomDialog(HWND hParent, std::wstring& outResult, bool& outPrivate) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MoveRoomProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = L"MoveRoomClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&wc);

    g_moveBoxOK = false;
    g_moveBoxResult = L"";
    g_moveBoxPrivate = false;

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);

    HWND hWnd = CreateWindowEx(WS_EX_DLGMODALFRAME, L"MoveRoomClass", GetString(IDS_MOVE_ROOM_TITLE).c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcParent.left + (rcParent.right - rcParent.left - Scale(300)) / 2, 
        rcParent.top + (rcParent.bottom - rcParent.top - Scale(220)) / 2, 
        Scale(300), Scale(220), hParent, 0, GetModuleHandle(0), 0);
    
    EnableWindow(hParent, FALSE);
    
    MSG msg;
    while (IsWindow(hWnd) && GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    
    if (g_moveBoxOK && !g_moveBoxResult.empty()) {
        outResult = g_moveBoxResult;
        outPrivate = g_moveBoxPrivate;
    }
    return g_moveBoxOK && !g_moveBoxResult.empty();
}

LPWSTR a2w(LPCSTR lpszText) {
    const DWORD dwSize = MultiByteToWideChar(CP_UTF8, 0, lpszText, -1, 0, 0);
    LPWSTR lpszReturnText = (LPWSTR)GlobalAlloc(0, dwSize * sizeof(WCHAR));
    if (lpszReturnText) {
        MultiByteToWideChar(CP_UTF8, 0, lpszText, -1, lpszReturnText, dwSize);
    }
    return lpszReturnText;
}

LPSTR w2a(LPCWSTR lpszText) {
    const DWORD dwSize = WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, 0, 0, 0, 0);
    LPSTR lpszReturnText = (char*)GlobalAlloc(GPTR, dwSize * sizeof(char));
    if (lpszReturnText) {
        WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, lpszReturnText, dwSize, 0, 0);
    }
    return lpszReturnText;
}

void UpdateUserList(const std::wstring& username) {
    int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)username.c_str());
    if (idx == LB_ERR) {
        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)username.c_str());
    }
}

void ClearLogAndUsers() {
    for (auto& entry : g_logEntries) {
        if (entry.pLayout) { entry.pLayout->Release(); entry.pLayout = nullptr; }
    }
    g_logEntries.clear();
    g_logScrollY = 0;
    g_autoScroll = true;
    g_logSelection.startEntryIndex = -1;
    g_logSelection.endEntryIndex = -1;
    g_logSelection.isSelecting = false;
    InvalidateRect(g_hLog, NULL, FALSE);
    
    SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
}

void SetWindowTitle(HWND hWnd) {
    std::wstring title = GetString(IDS_WINDOW_TITLE) + L" - [" + g_currentRoom + L"]" + (g_isPrivateRoom ? L" (" + GetString(IDS_PRIVATE) + L")" : L"");
    SetWindowText(hWnd, title.c_str());
}

void AppendLog(const MsgData& data) {
    if (!data.user.empty()) {
        {
            std::lock_guard<std::mutex> lock(g_ignoredMutex);
            if (g_ignoredUsers.count(data.user) > 0) return;
        }
        UpdateUserList(data.user);
    }
    LogEntry entry;
    entry.data = data;
    g_logEntries.push_back(entry);
    InvalidateRect(g_hLog, NULL, FALSE);
}


DWORD WINAPI RecvThread(LPVOID param) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HWND hWndParent = (HWND)param;
    char tcpBuf[4096];
    char udpBuf[65536];
    std::string tcpAccum;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        if (g_sock != INVALID_SOCKET) FD_SET(g_sock, &readfds);
        if (g_tcpSocket != INVALID_SOCKET) FD_SET(g_tcpSocket, &readfds);

        int max_fd = 0; // Windows select() ignores first arg

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            
            // Handle TCP (Signaling)
            if (g_tcpSocket != INVALID_SOCKET && FD_ISSET(g_tcpSocket, &readfds)) {
                int res = recv(g_tcpSocket, tcpBuf, sizeof(tcpBuf) - 1, 0);
                if (res > 0) {
                    tcpBuf[res] = 0;
                    tcpAccum += tcpBuf;
                    size_t pos;
                    while ((pos = tcpAccum.find('\n')) != std::string::npos) {
                        std::string line = tcpAccum.substr(0, pos);
                        tcpAccum.erase(0, pos + 1);

                        std::string err;
                        json11::Json json = json11::Json::parse(line, err);
                        if (err.empty()) {
                            std::string type = json["type"].string_value();
                            if (type == "welcome") {
                                g_sessionId = json["sessionId"].int_value();
                                
                                MediaHeader hdr = {0};
                                hdr.sessionId = g_sessionId;
                                hdr.payloadType = 255;
                                
                                sockaddr_in serverUdp = {0};
                                serverUdp.sin_family = AF_INET;
                                serverUdp.sin_port = htons(4001);
                                if (!g_isServerUdpAddrResolved) {
                        g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                    }
                    serverUdp.sin_addr = g_serverUdpAddr;
                                sendto(g_sock, (char*)&hdr, sizeof(hdr), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));

                                json11::Json getRoomsMsg = json11::Json::object {
                                    {"type", "get_rooms"}
                                };
                                std::string getRoomsStr = getRoomsMsg.dump() + "\n";
                                send(g_tcpSocket, getRoomsStr.c_str(), (int)getRoomsStr.length(), 0);
                            }
                            else if (type == "chat") {
                                MsgData* pData = new MsgData();
                                pData->user = a2w(json["user"].string_value().c_str());
                                pData->msg = a2w(json["msg"].string_value().c_str());
                                pData->b = json["b"].bool_value();
                                pData->i = json["i"].bool_value();
                                pData->u = json["u"].bool_value();
                                pData->size = json["sz"].is_number() ? json["sz"].int_value() : 12;
                                pData->color = json["clr"].is_number() ? json["clr"].int_value() : RGB(0,0,0);
                                PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
                            }
                            else if (type == "error") {
                                std::string errorMsg = json["msg"].string_value();
                                std::wstring wErrorMsg = a2w(errorMsg.c_str());
                                
                                if (errorMsg.find("full") != std::string::npos || errorMsg.find("Full") != std::string::npos) {
                                    wErrorMsg = GetString(IDS_ERROR_ROOM_FULL);
                                }
                                
                                MessageBoxW(hWndParent, wErrorMsg.c_str(), GetString(IDS_ERROR_TITLE).c_str(), MB_OK | MB_ICONERROR);
                                PostMessage(hWndParent, WM_REVERT_ROOM, 0, 0);
                            }
                            else if (type == "users") {
                                SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
                                auto arr = json["users"].array_items();
                                for (auto& item : arr) {
                                    std::wstring wname = a2w(item.string_value().c_str());
                                    int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wname.c_str());
                                    if (idx == LB_ERR) {
                                        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)wname.c_str());
                                    }
                                }
                                // Ensure local user is always in the list
                                int localIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_username.c_str());
                                if (localIdx == LB_ERR) {
                                    SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)g_username.c_str());
                                }
                            }
                            else if (type == "join") {
                                std::string uid = json["userId"].string_value();
                                std::wstring wuid = a2w(uid.c_str());
                                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wuid.c_str());
                                if (idx == LB_ERR) {
                                    SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)wuid.c_str());
                                    MsgData* pData = new MsgData();
                                    pData->user = wuid;
                                    pData->msg = GetString(IDS_ENTERED);
                                    pData->b = false; pData->i = false; pData->u = false;
                                    pData->size = 9;
                                    pData->color = RGB(128, 128, 128);
                                    pData->isSystemEvent = true;
                                    PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
                                }
                            }
                            else if (type == "redirect") {
                                std::string newIp = json["ip"].string_value();
                                int newPort = json["port"].int_value();
                                
                                if (!newIp.empty() && newPort > 0) {
                                    closesocket(g_tcpSocket);
                                    g_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                                    
                                    sockaddr_in serverAddr;
                                    serverAddr.sin_family = AF_INET;
                                    serverAddr.sin_port = htons(newPort);
                                    ResolveIP(newIp, serverAddr.sin_addr);
                                    g_serverIp = newIp; // Update global server IP
                                    g_isServerUdpAddrResolved = false; // Force re-resolve for UDP
    
                                    if (connect(g_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
                                        json11::Json joinMsg = json11::Json::object {
                                            {"type", "join"},
                                            {"userId", w2a(g_username.c_str())},
                                            {"roomName", w2a(g_currentRoom.c_str())}
                                        };
                                        std::string joinStr = joinMsg.dump() + "\n";
                                        send(g_tcpSocket, joinStr.c_str(), (int)joinStr.length(), 0);
                                    }
                                }
                            }
                            else if (type == "leave") {
                                std::string uid = json["userId"].string_value();
                                std::wstring wuid = a2w(uid.c_str());
                                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wuid.c_str());
                                if (idx != LB_ERR) {
                                    SendMessage(g_hUserList, LB_DELETESTRING, idx, 0);
                                    MsgData* pData = new MsgData();
                                    pData->user = wuid;
                                    pData->msg = GetString(IDS_LEFT);
                                    pData->b = false; pData->i = false; pData->u = false;
                                    pData->size = 9;
                                    pData->color = RGB(128, 128, 128);
                                    pData->isSystemEvent = true;
                                    PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
                                }
                            }
                            else if (type == "announce") {
                                std::string rmStr = json["rm"].string_value();
                                if (!rmStr.empty()) {
                                    std::wstring wrm = a2w(rmStr.c_str());
                                    if (g_publicRooms.find(wrm) == g_publicRooms.end()) {
                                        g_publicRooms[wrm] = 1;
                                    }
                                }
                            }
                            else if (type == "room_list") {
                                g_publicRooms.clear();
                                auto arr = json["rooms"].array_items();
                                for (auto& item : arr) {
                                    std::string name = item["name"].string_value();
                                    int count = item["count"].int_value();
                                    g_publicRooms[a2w(name.c_str())] = count;
                                }
                            }
                        }
                    }
                } else {
                    closesocket(g_tcpSocket);
                    g_tcpSocket = INVALID_SOCKET;
                }
            }


            if (g_sock != INVALID_SOCKET && FD_ISSET(g_sock, &readfds)) {
                sockaddr_in from;
                int fromlen = sizeof(from);
                int res = recvfrom(g_sock, udpBuf, sizeof(udpBuf), 0, (sockaddr*)&from, &fromlen);
                if (res != SOCKET_ERROR && res >= (int)sizeof(MediaHeader)) {
                    MediaHeader* mhdr = (MediaHeader*)udpBuf;
                    
                    bool isIgnored = false;
                    {
                        std::wstring fromUser = mhdr->username;
                        std::lock_guard<std::mutex> lock(g_ignoredMutex);
                        isIgnored = (g_ignoredUsers.count(fromUser) > 0);
                    }
                    if (isIgnored) continue;

                    if (mhdr->payloadType == 1) { // Audio
                        PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - sizeof(MediaHeader));
                        std::wstring wname = mhdr->username;
                        
                        int audioLen = res - sizeof(MediaHeader);
                        short* samples = (short*)(udpBuf + sizeof(MediaHeader));
                        int sampleCount = audioLen / 2;
                        int maxAmp = 0;
                        for (int i = 0; i < sampleCount; i++) {
                            int amp = abs(samples[i]);
                            if (amp > maxAmp) maxAmp = amp;
                        }
                        int volume = (maxAmp * 100) / 32767;
                        volume = volume * 3;
                        if (volume > 100) volume = 100;
                        
                        bool newlySpeaking = false;
                        {
                            std::lock_guard<std::mutex> lock(g_voiceMutex);
                            if (g_voiceUsers.find(wname) == g_voiceUsers.end() || (GetTickCount() - g_voiceUsers[wname] > 1000)) {
                                newlySpeaking = true;
                            }
                            g_voiceUsers[wname] = GetTickCount();
                            g_voiceVolumes[wname] = volume;
                        }
                        if (newlySpeaking && g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                    }
                    else if (mhdr->payloadType == 2) {
                        std::wstring wname = mhdr->username;
                        uint16_t seq = mhdr->seqNum;
                        int fragIdx = mhdr->timestamp;
                        int len = res - sizeof(MediaHeader);
                        
                        bool stateChanged = false;
                        {
                            std::lock_guard<std::mutex> lock(g_streamerMutex);
                            if (g_videoStreamers.find(wname) == g_videoStreamers.end() || (GetTickCount() - g_videoStreamers[wname] > 5000)) {
                                stateChanged = true;
                            }
                            g_videoStreamers[wname] = GetTickCount();
                        }
                        if (stateChanged) {
                            InvalidateRect(g_hUserList, NULL, FALSE);
                        }

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
                        
                        // Cleanup old incomplete frames
                        DWORD now = GetTickCount();
                        for (auto it = g_videoBuffers[wname].begin(); it != g_videoBuffers[wname].end(); ) {
                            if (now - it->second.lastUpdate > 2000) {
                                it = g_videoBuffers[wname].erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    else if (mhdr->payloadType == 3) {
                        std::wstring viewer = mhdr->username;
                        int len = res - sizeof(MediaHeader);
                        if (len > 0) {
                            std::string targetA((char*)(udpBuf + sizeof(MediaHeader)), len);
                            int wsize = MultiByteToWideChar(CP_UTF8, 0, targetA.c_str(), -1, NULL, 0);
                            if (wsize > 0) {
                                std::wstring target;
                                target.resize(wsize - 1);
                                MultiByteToWideChar(CP_UTF8, 0, targetA.c_str(), -1, &target[0], wsize - 1);
                                g_videoViewers[target][viewer] = GetTickCount();
                            }
                        }
                    }
                }
            }
        }

        // Auto-reconnect logic
        if (g_tcpSocket == INVALID_SOCKET && !g_username.empty()) {
            static DWORD lastReconnect = 0;
            DWORD now = GetTickCount();
            if (now - lastReconnect > 5000) {
                lastReconnect = now;
                
                g_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (g_tcpSocket != INVALID_SOCKET) {
                    sockaddr_in serverAddr = { 0 };
                    serverAddr.sin_family = AF_INET;
                    serverAddr.sin_port = htons(4000);
                    ResolveIP(g_serverIp, serverAddr.sin_addr);
                    
                    if (connect(g_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
                        g_isServerUdpAddrResolved = false;
                        json11::Json joinMsg = json11::Json::object {
                            {"type", "join"},
                            {"userId", w2a(g_username.c_str())},
                            {"roomName", w2a(g_currentRoom.c_str())}
                        };
                        std::string joinStr = joinMsg.dump() + "\n";
                        send(g_tcpSocket, joinStr.c_str(), (int)joinStr.length(), 0);
                    } else {
                        closesocket(g_tcpSocket);
                        g_tcpSocket = INVALID_SOCKET;
                    }
                }
            }
        }
    }
    return 0;
}

void UpdateInputFormat() {
    InvalidateRect(g_hInput, NULL, FALSE);
}

void SendLeaveRequest(const std::wstring& roomName) {
    LPSTR userA_ptr = w2a(g_username.c_str());
    LPSTR roomA_ptr = w2a(roomName.c_str());
    json11::Json leaveMsg = json11::Json::object {
        {"type", "leave"},
        {"userId", std::string(userA_ptr)},
        {"roomName", std::string(roomA_ptr)}
    };
    GlobalFree(userA_ptr);
    GlobalFree(roomA_ptr);
    
    std::string leaveStr = leaveMsg.dump() + "\n";
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, leaveStr.c_str(), (int)leaveStr.length(), 0);
    }
}

void SendJoinRequest(const std::wstring& roomName) {
    LPSTR userA_ptr = w2a(g_username.c_str());
    LPSTR roomA_ptr = w2a(roomName.c_str());
    json11::Json joinMsg = json11::Json::object {
        {"type", "join"},
        {"userId", std::string(userA_ptr)},
        {"roomName", std::string(roomA_ptr)}
    };
    GlobalFree(userA_ptr);
    GlobalFree(roomA_ptr);
    
    std::string joinStr = joinMsg.dump() + "\n";
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, joinStr.c_str(), (int)joinStr.length(), 0);
    }
}

void SendChatMessage(const std::wstring& msg) {
    LPSTR userA_ptr = w2a(g_username.c_str());
    LPSTR msgA_ptr = w2a(msg.c_str());
    
    bool b = SendMessage(g_hBtnB, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool i = SendMessage(g_hBtnI, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool u = SendMessage(g_hBtnU, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    int sizeIndex = (int)SendMessage(g_hComboSize, CB_GETCURSEL, 0, 0);
    int size = 12;
    if (sizeIndex != CB_ERR) {
        WCHAR szText[32];
        SendMessage(g_hComboSize, CB_GETLBTEXT, sizeIndex, (LPARAM)szText);
        size = _wtoi(szText);
    }
    
    json11::Json json = json11::Json::object {
        {"type", "chat"},
        {"user", std::string(userA_ptr)},
        {"msg", std::string(msgA_ptr)},
        {"b", b},
        {"i", i},
        {"u", u},
        {"sz", size},
        {"clr", (int)g_currentColor}
    };
    std::string jsonStr = json.dump() + "\n";
    
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, jsonStr.c_str(), (int)jsonStr.length(), 0);
    }
    
    // Local Echo
    MsgData* pData = new MsgData();
    pData->user = g_username;
    pData->msg = msg;
    pData->b = b;
    pData->i = i;
    pData->u = u;
    pData->size = size;
    pData->color = g_currentColor;
    PostMessage(GetParent(g_hInput), WM_USER_MSG, 0, (LPARAM)pData);
    
    GlobalFree(userA_ptr);
    GlobalFree(msgA_ptr);
}


std::map<HWND, ID2D1HwndRenderTarget*> g_videoRts;

LRESULT CALLBACK VideoViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::wstring* pUser = (std::wstring*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_NCCREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        std::wstring* u = new std::wstring((LPCWSTR)pcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)u);
        SetTimer(hwnd, 1, 2000, NULL);
        return TRUE;
    }
    case WM_NCDESTROY: {
        KillTimer(hwnd, 1);
        if (g_videoRts[hwnd]) {
            g_videoRts[hwnd]->Release();
            g_videoRts.erase(hwnd);
        }
        if (pUser) {
            if (*pUser == g_username) {
                g_isWebcamActive = false;
                InvalidateRect(g_hUserList, NULL, FALSE);
            }
            delete pUser;
        }
        return 0;
    }
    case WM_TIMER: {
        if (wParam == 1 && pUser && g_sock != INVALID_SOCKET) {
            std::string targetA;
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, pUser->c_str(), -1, NULL, 0, NULL, NULL);
            if (size_needed > 0) {
                targetA.resize(size_needed - 1);
                WideCharToMultiByte(CP_UTF8, 0, pUser->c_str(), -1, &targetA[0], size_needed - 1, NULL, NULL);
            }
            
            MediaHeader hdr = {0};
            hdr.sessionId = g_sessionId;
            hdr.payloadType = 3;
            hdr.timestamp = GetTickCount();
            wcsncpy_s(hdr.username, g_username.c_str(), 31);
            
            char pkt[1024];
            memcpy(pkt, &hdr, sizeof(hdr));
            memcpy(pkt + sizeof(hdr), targetA.c_str(), targetA.length());
            
            sockaddr_in serverUdp = {0};
            serverUdp.sin_family = AF_INET;
            serverUdp.sin_port = htons(4001);
            if (!g_isServerUdpAddrResolved) {
                g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
            }
            serverUdp.sin_addr = g_serverUdpAddr;
            sendto(g_sock, pkt, sizeof(hdr) + (int)targetA.length(), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
            
            g_videoViewers[*pUser][g_username] = GetTickCount();
            
            int count = 0;
            DWORD now = GetTickCount();
            for (auto& pair : g_videoViewers[*pUser]) {
                if (now - pair.second < 5000) count++;
            }
            WCHAR viewBuf[256];
            swprintf_s(viewBuf, GetString(IDS_VIEWING_COUNT).c_str(), count);
            std::wstring title = *pUser + viewBuf;
            SetWindowTextW(hwnd, title.c_str());
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        if (!g_videoRts[hwnd]) {
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            g_pD2DFactory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)), &g_videoRts[hwnd]);
        }
        
        ID2D1HwndRenderTarget* pRT = g_videoRts[hwnd];
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
                        float vW = (float)sz.first;
                        float vH = (float)sz.second;
                        float wW = (float)rc.right * 96.0f / g_dpi;
                        float wH = (float)rc.bottom * 96.0f / g_dpi;
                        float scale = min(wW / vW, wH / vH);
                        float drawW = vW * scale;
                        float drawH = vH * scale;
                        float offsetX = (wW - drawW) / 2.0f;
                        float offsetY = (wH - drawH) / 2.0f;
                        pRT->DrawBitmap(pBitmap, D2D1::RectF(offsetX, offsetY, offsetX + drawW, offsetY + drawH));
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
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (g_videoRts[hwnd]) {
            g_videoRts[hwnd]->Resize(D2D1::SizeU(width, height));
        }
        return 0;
    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


WNDPROC g_oldListProc = NULL;
LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_RBUTTONDOWN) {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        LRESULT res = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
        if (HIWORD(res) == 0) {
            int item = LOWORD(res);
            int len = (int)SendMessage(hwnd, LB_GETTEXTLEN, item, 0);
            if (len != LB_ERR && len >= 0) {
                std::vector<WCHAR> buf(len + 1, 0);
                SendMessage(hwnd, LB_GETTEXT, item, (LPARAM)buf.data());
                std::wstring clickedUser(buf.data());
                
                if (clickedUser == g_username) {
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenuW(hMenu, MF_STRING | (g_isWebcamActive ? MF_CHECKED : MF_UNCHECKED), 1, GetString(IDS_VIDEO_STREAM).c_str());
                    AppendMenuW(hMenu, MF_STRING, 3, GetString(IDS_CHANGE_NAME_TITLE).c_str()); 
                    ClientToScreen(hwnd, &pt);
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);
                    if (cmd == 1) {
                        if (g_isWebcamActive) {
                            g_isWebcamActive = false;
                            if (g_videoWindows[g_username] && IsWindow(g_videoWindows[g_username])) {
                                DestroyWindow(g_videoWindows[g_username]);
                            }
                        } else {
                            g_isWebcamActive = true;
                            if (!g_videoWindows[g_username] || !IsWindow(g_videoWindows[g_username])) {
                                HWND hVid = CreateWindowEx(0, L"D2DVideoViewClass", g_username.c_str(),
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, Scale(320), Scale(240), NULL, NULL, GetModuleHandle(NULL), (LPVOID)g_username.c_str());
                                g_videoWindows[g_username] = hVid;
                            } else {
                                SetForegroundWindow(g_videoWindows[g_username]);
                            }
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                    } else if (cmd == 3) {
                        std::wstring newName;
                        if (ShowChangeNameDialog(GetParent(hwnd), newName) && !newName.empty() && newName != g_username) {
                            std::wstring oldName = g_username;
                            SendLeaveRequest(g_currentRoom);
                            g_username = newName;
                            SaveSettings();
                            
                            int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)oldName.c_str());
                            if (idx != LB_ERR) {
                                SendMessage(g_hUserList, LB_DELETESTRING, idx, 0);
                            }
                            UpdateUserList(newName);
                            
                            SendJoinRequest(g_currentRoom);
                        }
                    }
                } else {
                    bool hasStreamer = false;
                    {
                        std::lock_guard<std::mutex> lock(g_streamerMutex);
                        hasStreamer = (g_videoStreamers.count(clickedUser) > 0 && (GetTickCount() - g_videoStreamers[clickedUser] < 5000));
                    }
                    bool isIgnored = false;
                    {
                        std::lock_guard<std::mutex> lock(g_ignoredMutex);
                        isIgnored = (g_ignoredUsers.count(clickedUser) > 0);
                    }
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenuW(hMenu, MF_STRING | (hasStreamer ? 0 : MF_GRAYED), 2, GetString(IDS_WATCH_VIDEO).c_str());
                    AppendMenuW(hMenu, MF_STRING | (isIgnored ? MF_CHECKED : MF_UNCHECKED), 4, GetString(IDS_IGNORE).c_str());
                    ClientToScreen(hwnd, &pt);
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);
                    if (cmd == 2 && hasStreamer) {
                        if (!g_videoWindows[clickedUser] || !IsWindow(g_videoWindows[clickedUser])) {
                            HWND hVid = CreateWindowEx(0, L"D2DVideoViewClass", clickedUser.c_str(),
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, Scale(320), Scale(240), NULL, NULL, GetModuleHandle(NULL), (LPVOID)clickedUser.c_str());
                            g_videoWindows[clickedUser] = hVid;
                        } else {
                            SetForegroundWindow(g_videoWindows[clickedUser]);
                        }
                    } else if (cmd == 4) {
                        std::lock_guard<std::mutex> lock(g_ignoredMutex);
                        if (g_ignoredUsers.count(clickedUser) > 0) {
                            g_ignoredUsers.erase(clickedUser);
                        } else {
                            g_ignoredUsers.insert(clickedUser);
                            if (g_videoWindows[clickedUser] && IsWindow(g_videoWindows[clickedUser])) {
                                DestroyWindow(g_videoWindows[clickedUser]);
                            }
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            }
        }
        return 0;
    }
    return CallWindowProc(g_oldListProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory));
    CreateThread(NULL, 0, WebcamThread, NULL, 0, NULL);

D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
        
        WNDCLASS d2dLogClass = { 0 };
        d2dLogClass.style = CS_DBLCLKS;
        d2dLogClass.lpfnWndProc = D2DLogProc;
        d2dLogClass.hInstance = GetModuleHandle(0);
        d2dLogClass.lpszClassName = L"D2DLogViewClass";
        d2dLogClass.hCursor = LoadCursor(0, IDC_ARROW);
        RegisterClass(&d2dLogClass);

        WNDCLASS d2dInputClass = { 0 };
        d2dInputClass.style = CS_DBLCLKS;
        d2dInputClass.lpfnWndProc = D2DInputProc;
        d2dInputClass.hInstance = GetModuleHandle(0);
        d2dInputClass.lpszClassName = L"D2DInputBoxClass";
        d2dInputClass.hCursor = LoadCursor(0, IDC_IBEAM);
        RegisterClass(&d2dInputClass);
        
        WNDCLASS d2dVideoClass = { 0 };
        d2dVideoClass.lpfnWndProc = VideoViewProc;
        d2dVideoClass.hInstance = GetModuleHandle(0);
        d2dVideoClass.lpszClassName = L"D2DVideoViewClass";
        d2dVideoClass.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(102)); // IDI_TV
        d2dVideoClass.hCursor = LoadCursor(0, IDC_ARROW);
        d2dVideoClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&d2dVideoClass);
        
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in localAddr = { 0 };
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(0); // Auto-assign UDP port
        localAddr.sin_addr.s_addr = INADDR_ANY;
        bind(g_sock, (sockaddr*)&localAddr, sizeof(localAddr));
        int rcvBufSize = 2097152;
        setsockopt(g_sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(rcvBufSize));

        g_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(4000);
        ResolveIP(g_serverIp, serverAddr.sin_addr);
        
        bool isConnected = true;
        if (connect(g_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            MessageBoxW(hWnd, GetString(IDS_ERROR_NETWORK).c_str(), GetString(IDS_ERROR_TITLE).c_str(), MB_OK | MB_ICONERROR);
            isConnected = false;
        }

        LoadSettings();
        if (g_username.empty()) {
            g_username = L"User_" + std::to_wstring(GetTickCount() % 1000);
        }
        
        // Ensure the username has a #XXXX tag to distinguish duplicates
        if (g_username.find(L'#') == std::wstring::npos) {
            srand(GetTickCount());
            wchar_t tag[16];
            swprintf_s(tag, L"#%04d", rand() % 10000);
            g_username += tag;
        }
        SaveSettings();

        g_hFont = CreateFontW(Scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF");
        g_hFontBold = CreateFontW(Scale(14), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF");
        g_hFontI_btn = CreateFontW(Scale(14), 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF");
        g_hFontU_btn = CreateFontW(Scale(14), 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF");

        // UI Creation
        g_hLog = CreateWindowEx(WS_EX_CLIENTEDGE, L"D2DLogViewClass", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hUserList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_SORT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
            0, 0, 0, 0, hWnd, (HMENU)1050, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        SetWindowTheme(g_hLog, L" ", L" ");
        SetWindowTheme(g_hUserList, L" ", L" ");



        g_hBtnMoveRoom = CreateWindow(L"BUTTON", L"", 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1008, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        g_hBtnVoice = CreateWindow(L"BUTTON", GetString(IDS_TALK).c_str(), 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1009, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hVolumeIndicator = CreateWindow(L"STATIC", L"", 
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1051, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hSpeakerIcon = CreateWindow(L"STATIC", L"", 
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1053, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hVolumeSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS,
            0, 0, 0, 0, hWnd, (HMENU)1052, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        SendMessage(g_hVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));

        g_hBtnVideo = CreateWindow(L"BUTTON", L"Video: OFF", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)1010, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        InitAudio();

        g_oldListProc = (WNDPROC)SetWindowLongPtr(g_hUserList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);


        g_hInput = CreateWindowEx(WS_EX_CLIENTEDGE, L"D2DInputBoxClass", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hSendBtn = CreateWindow(L"BUTTON", GetString(IDS_SEND).c_str(), 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)1001, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hBtnB = CreateWindow(L"BUTTON", L"B", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
            0, 0, 0, 0, hWnd, (HMENU)1003, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        g_hBtnI = CreateWindow(L"BUTTON", L"I", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
            0, 0, 0, 0, hWnd, (HMENU)1004, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        g_hBtnU = CreateWindow(L"BUTTON", L"U", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
            0, 0, 0, 0, hWnd, (HMENU)1005, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        g_hComboSize = CreateWindow(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            0, 0, 0, 0, hWnd, (HMENU)1006, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        
        g_hBtnColor = CreateWindow(L"BUTTON", L"", 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1002, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        const WCHAR* sizes[] = { L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"28", L"32" };
        for (auto sz : sizes) SendMessage(g_hComboSize, CB_ADDSTRING, 0, (LPARAM)sz);
        SendMessage(g_hComboSize, CB_SETCURSEL, 4, 0);

        SendMessage(g_hUserList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        SendMessage(g_hSendBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessage(g_hBtnB, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
        SendMessage(g_hBtnI, WM_SETFONT, (WPARAM)g_hFontI_btn, TRUE);
        SendMessage(g_hBtnU, WM_SETFONT, (WPARAM)g_hFontU_btn, TRUE);
        SendMessage(g_hComboSize, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        UpdateInputFormat();
        SetWindowTitle(hWnd);

        MsgData sysMsg;
        sysMsg.user = L"";
        if (isConnected) {
            std::wstring dispName = g_username;
            size_t pos = dispName.find(L'#');
            if (pos != std::wstring::npos) dispName = dispName.substr(0, pos);
            sysMsg.msg = GetString(IDS_JOINED_NETWORK) + L" (" + dispName + L")";
            sysMsg.color = RGB(128,128,128);
        } else {
            sysMsg.msg = GetString(IDS_SERVER_NOT_FOUND);
            sysMsg.color = RGB(255,0,0);
        }
        sysMsg.b = false; sysMsg.i = false; sysMsg.u = false;
        sysMsg.size = 9;
        AppendLog(sysMsg);

        // Send initial join after all UI controls are fully created
        SendJoinRequest(g_currentRoom);
        CreateThread(0, 0, RecvThread, hWnd, 0, 0);
        SetTimer(hWnd, 1, 3000, NULL);
        SetTimer(hWnd, 2, 100, NULL);
        InitVolumeSync(hWnd);

        return 0;
    }

    case WM_MEASUREITEM: {
        LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
        if (lpmis->CtlID == 1050) {
            lpmis->itemHeight = Scale(24);
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        
        if (lpdis->CtlID == 1009) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            if (g_isVoiceActive) {
                SetDCBrushColor(hdc, RGB(100, 255, 100));
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
            } else {
                FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
            }
            UINT edge = ((lpdis->itemState & ODS_SELECTED) || g_isVoiceActive) ? EDGE_SUNKEN : EDGE_RAISED;
            DrawEdge(hdc, &rc, edge, BF_RECT);
            
            if (lpdis->itemState & ODS_FOCUS) {
                RECT rcFocus = rc;
                InflateRect(&rcFocus, -4, -4);
                DrawFocusRect(hdc, &rcFocus);
            }

            GdiFlush(); // Ensure GDI background is drawn before GDI+ draws on top

            {
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                int cx = rc.left + Scale(14);
                int cy = rc.top + (rc.bottom - rc.top) / 2 - Scale(2);

                Gdiplus::SolidBrush micBrush(Gdiplus::Color(255, 0, 0, 0));
                Gdiplus::Pen micPen(Gdiplus::Color(255, 0, 0, 0), 1.5f);
                
                graphics.FillRectangle(&micBrush, cx - Scale(2), cy - Scale(4), Scale(4), Scale(6));
                graphics.FillEllipse(&micBrush, cx - Scale(2), cy - Scale(6), Scale(4), Scale(4));
                graphics.FillEllipse(&micBrush, cx - Scale(2), cy, Scale(4), Scale(4));
                graphics.DrawArc(&micPen, cx - Scale(5), cy - Scale(2), Scale(10), Scale(8), 0.0f, 180.0f);
                graphics.DrawLine(&micPen, cx, cy + Scale(6), cx, cy + Scale(9));
                graphics.DrawLine(&micPen, cx - Scale(3), cy + Scale(9), cx + Scale(3), cy + Scale(9));
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0,0,0));
            HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
            std::wstring talkStr = GetString(IDS_TALK);
            LPCWSTR text = talkStr.c_str();
            RECT rcText = rc;
            rcText.left += Scale(28);
            DrawTextW(hdc, text, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            SelectObject(hdc, hOldFont);
            return TRUE;
        }
        else if (lpdis->CtlID == 1008) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
            UINT edge = (lpdis->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_RAISED;
            DrawEdge(hdc, &rc, edge, BF_RECT);
            if (lpdis->itemState & ODS_FOCUS) {
                RECT rcFocus = rc; InflateRect(&rcFocus, -4, -4); DrawFocusRect(hdc, &rcFocus);
            }

            GdiFlush(); // Ensure GDI background is drawn before GDI+ draws on top

            HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
            std::wstring moveStr = GetString(IDS_MOVE_ROOM_BTN);
            LPCWSTR text = moveStr.c_str();
            SIZE size;
            GetTextExtentPoint32(hdc, text, lstrlen(text), &size);
            int iconWidth = Scale(16);
            int gap = Scale(6);
            int blockWidth = iconWidth + gap + size.cx;
            int startX = rc.left + (rc.right - rc.left - blockWidth) / 2;
            int cx = startX + iconWidth / 2;
            int cy = rc.top + (rc.bottom - rc.top) / 2;

            {
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                
                Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), 1.5f);
                Gdiplus::SolidBrush knobBrush(Gdiplus::Color(255, 0, 0, 0));
                graphics.DrawRectangle(&pen, cx - Scale(6), cy - Scale(6), Scale(8), Scale(12));
                graphics.FillEllipse(&knobBrush, cx - Scale(2), cy - Scale(1), Scale(2), Scale(2));
                graphics.DrawLine(&pen, cx + Scale(2), cy, cx + Scale(8), cy);
                graphics.DrawLine(&pen, cx + Scale(8), cy, cx + Scale(5), cy - Scale(3));
                graphics.DrawLine(&pen, cx + Scale(8), cy, cx + Scale(5), cy + Scale(3));
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0,0,0));
            RECT rcText = rc;
            rcText.left = startX + iconWidth + gap;
            DrawTextW(hdc, text, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            SelectObject(hdc, hOldFont);
            return TRUE;
        }
        else if (lpdis->CtlID == 1002) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
            UINT edge = (lpdis->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_RAISED;
            DrawEdge(hdc, &rc, edge, BF_RECT);
            if (lpdis->itemState & ODS_FOCUS) {
                RECT rcFocus = rc; InflateRect(&rcFocus, -4, -4); DrawFocusRect(hdc, &rcFocus);
            }
            
            GdiFlush(); // Ensure GDI background is drawn before GDI+ draws on top

            {
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                
                int cx = rc.left + (rc.right - rc.left) / 2;
                int cy = rc.top + (rc.bottom - rc.top) / 2 - 1;
                
                Gdiplus::SolidBrush palBrush(Gdiplus::Color(255, 245, 225, 170));
                graphics.FillEllipse(&palBrush, cx - Scale(10), cy - Scale(7), Scale(20), Scale(14));

                COLORREF btnFace = GetSysColor(COLOR_BTNFACE);
                Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, GetRValue(btnFace), GetGValue(btnFace), GetBValue(btnFace)));
                graphics.FillEllipse(&bgBrush, cx + Scale(3), cy - Scale(3), Scale(6), Scale(6));

                Gdiplus::SolidBrush yellow(Gdiplus::Color(255, 240, 200, 40));
                Gdiplus::SolidBrush red(Gdiplus::Color(255, 230, 50, 50));
                Gdiplus::SolidBrush blue(Gdiplus::Color(255, 50, 150, 230));
                Gdiplus::SolidBrush brown(Gdiplus::Color(255, 150, 80, 30));

                graphics.FillEllipse(&yellow, cx - Scale(2), cy - Scale(5), Scale(4), Scale(4));
                graphics.FillEllipse(&red, cx - Scale(6), cy - Scale(3), Scale(4), Scale(4));
                graphics.FillEllipse(&blue, cx - Scale(7), cy + Scale(1), Scale(4), Scale(4));
                graphics.FillEllipse(&brown, cx - Scale(3), cy + Scale(3), Scale(4), Scale(4));
            }
            
            return TRUE;
        }
        else if (lpdis->CtlID == 1051) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            RECT rcBg = rc;
            InflateRect(&rcBg, -1, -1);
            FillRect(hdc, &rcBg, (HBRUSH)(COLOR_BTNFACE + 1));
            
            if (g_isVoiceActive) {
                int maxVolWidth = rc.right - rc.left - 2;
                int volWidth = (maxVolWidth * g_currentMicVolume) / 100;
                if (volWidth > 0) {
                    Gdiplus::Graphics graphics(hdc);
                    Gdiplus::Rect fullRect(rc.left + 1, rc.top + 1, maxVolWidth, rc.bottom - rc.top - 2);
                    Gdiplus::LinearGradientBrush linGrBrush(
                        fullRect,
                        Gdiplus::Color(255, 0, 255, 0),
                        Gdiplus::Color(255, 255, 0, 0),
                        Gdiplus::LinearGradientModeHorizontal);
                        
                    Gdiplus::Color colors[] = {
                        Gdiplus::Color(255, 0, 255, 0),
                        Gdiplus::Color(255, 255, 255, 0),
                        Gdiplus::Color(255, 255, 0, 0)
                    };
                    Gdiplus::REAL positions[] = { 0.0f, 0.6f, 1.0f };
                    linGrBrush.SetInterpolationColors(colors, positions, 3);
                    
                    Gdiplus::Rect volRect(rc.left + 1, rc.top + 1, volWidth, rc.bottom - rc.top - 2);
                    graphics.FillRectangle(&linGrBrush, volRect);
                }
            }
            
            HBRUSH hGrayBrush = CreateSolidBrush(RGB(180, 180, 180));
            FrameRect(hdc, &rc, hGrayBrush);
            DeleteObject(hGrayBrush);
            return TRUE;
        }
        else if (lpdis->CtlID == 1053) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            // Draw background
            HBRUSH hBgBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 60, 60, 60));
            Gdiplus::Pen blackPen(Gdiplus::Color(255, 60, 60, 60), (Gdiplus::REAL)(Scale(1) * 1.5f));

            int cx = rc.left + Scale(7);
            int cy = rc.top + (rc.bottom - rc.top) / 2;

            // Speaker body (Outline shape)
            Gdiplus::Point pts[6] = {
                Gdiplus::Point(cx - Scale(5), cy - Scale(3)),
                Gdiplus::Point(cx - Scale(2), cy - Scale(3)),
                Gdiplus::Point(cx + Scale(3), cy - Scale(7)),
                Gdiplus::Point(cx + Scale(3), cy + Scale(7)),
                Gdiplus::Point(cx - Scale(2), cy + Scale(3)),
                Gdiplus::Point(cx - Scale(5), cy + Scale(3))
            };
            blackPen.SetLineJoin(Gdiplus::LineJoinRound);
            graphics.DrawPolygon(&blackPen, pts, 6);

            // Sound waves (natural circular arcs, slightly smaller)
            int waveCx = cx + Scale(2);
            int r1 = Scale(5);
            graphics.DrawArc(&blackPen, waveCx - r1, cy - r1, r1 * 2, r1 * 2, -45.0f, 90.0f);
            
            int r2 = Scale(9);
            graphics.DrawArc(&blackPen, waveCx - r2, cy - r2, r2 * 2, r2 * 2, -45.0f, 90.0f);

            return TRUE;
        }
        else if (lpdis->CtlID == 1050 && lpdis->itemID != (UINT)-1) {
            HDC hdcOriginal = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            HDC hdc = CreateCompatibleDC(hdcOriginal);
            HBITMAP hMemBmp = CreateCompatibleBitmap(hdcOriginal, rc.right - rc.left, rc.bottom - rc.top);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdc, hMemBmp);
            SetWindowOrgEx(hdc, rc.left, rc.top, NULL);
            
            WCHAR szText[256] = {0};
            SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)szText);
            std::wstring user(szText);

            bool isIgnored = false;
            {
                std::lock_guard<std::mutex> lock(g_ignoredMutex);
                isIgnored = (g_ignoredUsers.count(user) > 0);
            }

            COLORREF userColor = GetSysColor(COLOR_WINDOWTEXT);
            if (!isIgnored) {
                size_t hash = 0;
                for (wchar_t c : user) hash = hash * 31 + c;
                COLORREF palette[] = {
                    RGB(200, 50, 50), RGB(50, 150, 50), RGB(50, 50, 200),
                    RGB(180, 180, 50), RGB(180, 50, 180), RGB(50, 180, 180),
                    RGB(255, 128, 0), RGB(128, 0, 255), RGB(0, 128, 255),
                    RGB(128, 200, 0), RGB(255, 0, 128), RGB(0, 200, 128)
                };
                userColor = palette[hash % 12];
            }

            if (lpdis->itemState & ODS_SELECTED) {
                FillRect(hdc, &rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
                SetTextColor(hdc, isIgnored ? RGB(180, 180, 180) : GetSysColor(COLOR_HIGHLIGHTTEXT));
            } else {
                FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
                SetTextColor(hdc, isIgnored ? RGB(140, 140, 140) : userColor);
            }
            SetBkMode(hdc, TRANSPARENT);

            int iconSize = Scale(16);
            int cx = rc.left + Scale(2);
            int cy = rc.top + (rc.bottom - rc.top - iconSize) / 2;
            
            RECT rcText = rc;
            rcText.left = rc.left + Scale(26);
            rcText.right = rc.right - Scale(12);
            
            { // Start GDI+ scope
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            
            bool isStreaming = false;
            {
                std::lock_guard<std::mutex> lock(g_streamerMutex);
                if (g_videoStreamers.count(user)) {
                    if (GetTickCount() - g_videoStreamers[user] < 5000) isStreaming = true;
                }
            }
            if (user == g_username && g_isWebcamActive) isStreaming = true;
            if (isIgnored) isStreaming = false;

            if (isStreaming) {
                // TV Cabinet (Wood color)
                int tvW = iconSize + Scale(5);
                int tvH = iconSize + Scale(6);
                int tvX = cx - Scale(2);
                int tvY = cy - Scale(3);
                
                Gdiplus::SolidBrush cabinetBrush(Gdiplus::Color(255, 139, 69, 19)); 
                graphics.FillRectangle(&cabinetBrush, tvX, tvY, tvW, tvH);
                
                Gdiplus::Pen cabinetBorder(Gdiplus::Color(255, 80, 40, 10), 1.0f);
                graphics.DrawRectangle(&cabinetBorder, tvX, tvY, tvW, tvH);
                
                // TV Screen Bezel (Dark Gray)
                Gdiplus::SolidBrush bezelBrush(Gdiplus::Color(255, 40, 40, 40));
                graphics.FillRectangle(&bezelBrush, tvX + 1, tvY + 1, iconSize + 2, iconSize + 2);
                
                // Screen Glass (Dark Blue-Cyan)
                Gdiplus::SolidBrush screenBrush(Gdiplus::Color(255, 20, 30, 50));
                graphics.FillRectangle(&screenBrush, tvX + Scale(2), tvY + Scale(2), iconSize, iconSize);
                
                int panelX = tvX + iconSize + Scale(2);
                
                // Tiny Dials / Knobs
                Gdiplus::SolidBrush knobBrush(Gdiplus::Color(255, 180, 180, 180));
                graphics.FillEllipse(&knobBrush, panelX, tvY + Scale(3), Scale(2), Scale(2));
                graphics.FillEllipse(&knobBrush, panelX, tvY + Scale(8), Scale(2), Scale(2));
                
                // Speaker grill
                Gdiplus::Pen speakerPen(Gdiplus::Color(255, 80, 40, 10), 1.0f);
                graphics.DrawLine(&speakerPen, panelX, tvY + Scale(14), panelX + Scale(2), tvY + Scale(14));
                graphics.DrawLine(&speakerPen, panelX, tvY + Scale(16), panelX + Scale(2), tvY + Scale(16));
                
                // Antenna
                Gdiplus::Pen antennaPen(Gdiplus::Color(255, 150, 150, 150), 1.5f);
                int antBaseX = tvX + (iconSize + Scale(2)) / 2;
                graphics.DrawLine(&antennaPen, antBaseX, tvY, antBaseX - Scale(5), tvY - Scale(6));
                graphics.DrawLine(&antennaPen, antBaseX, tvY, antBaseX + Scale(5), tvY - Scale(6));
                
                Gdiplus::SolidBrush antTip(Gdiplus::Color(255, 200, 200, 200));
                graphics.FillEllipse(&antTip, antBaseX - Scale(6), tvY - Scale(7), Scale(3), Scale(3));
                graphics.FillEllipse(&antTip, antBaseX + Scale(4), tvY - Scale(7), Scale(3), Scale(3));
            }
            
            Gdiplus::SolidBrush faceBrush(isIgnored ? Gdiplus::Color(255, 180, 180, 180) : Gdiplus::Color(255, 255, 215, 0));
            graphics.FillEllipse(&faceBrush, cx, cy, iconSize, iconSize);
            
            Gdiplus::Pen facePen(isIgnored ? Gdiplus::Color(255, 120, 120, 120) : Gdiplus::Color(255, 200, 150, 0), 1.0f);
            graphics.DrawEllipse(&facePen, cx, cy, iconSize, iconSize);
            
            Gdiplus::SolidBrush eyeBrush(Gdiplus::Color(255, 0, 0, 0));
            graphics.FillEllipse(&eyeBrush, cx + Scale(4), cy + Scale(4), Scale(3), Scale(3));
            graphics.FillEllipse(&eyeBrush, cx + Scale(9), cy + Scale(4), Scale(3), Scale(3));
            
            Gdiplus::Pen mouthPen(Gdiplus::Color(255, 0, 0, 0), 1.5f);
            graphics.DrawArc(&mouthPen, cx + Scale(3), cy + Scale(4), Scale(10), Scale(8), 20.0f, 140.0f);
            
            bool isVoicing = false;
            int currentVolume = 0;
            {
                std::lock_guard<std::mutex> lock(g_voiceMutex);
                if (g_voiceUsers.count(user)) {
                    if (GetTickCount() - g_voiceUsers[user] < 1000) {
                        isVoicing = true;
                        currentVolume = g_voiceVolumes[user];
                    }
                }
            }
            if (user == g_username && g_isVoiceActive) {
                isVoicing = true;
                currentVolume = g_currentMicVolume;
            }
            if (isIgnored) isVoicing = false;
            
            if (isVoicing) {
                int micX = rc.right - Scale(8);
                int micY = cy + Scale(6);
                
                int micTop = micY - Scale(6);
                int micBottom = micY + Scale(8);
                int totalHeight = micBottom - micTop;
                int fillHeight = (totalHeight * currentVolume) / 100;
                if (fillHeight > totalHeight) fillHeight = totalHeight;
                
                Gdiplus::Rect fullMicRect(micX - Scale(6), micTop, Scale(12), totalHeight);
                Gdiplus::LinearGradientBrush linGrBrush(
                    fullMicRect,
                    Gdiplus::Color(255, 255, 0, 0),
                    Gdiplus::Color(255, 0, 255, 0),
                    Gdiplus::LinearGradientModeVertical);
                    
                Gdiplus::Color colors[] = {
                    Gdiplus::Color(255, 230, 20, 20),    // Red
                    Gdiplus::Color(255, 255, 140, 0),    // Orange
                    Gdiplus::Color(255, 240, 220, 0),    // Yellow
                    Gdiplus::Color(255, 20, 200, 20)     // Green
                };
                Gdiplus::REAL positions[] = { 0.0f, 0.3f, 0.6f, 1.0f };
                linGrBrush.SetInterpolationColors(colors, positions, 4);
                
                Gdiplus::Pen gradientPen(&linGrBrush, 1.5f);
                Gdiplus::Pen blackPen(Gdiplus::Color(255, 40, 40, 40), 1.5f);
                Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 40, 40, 40));
                
                auto DrawMic = [&](Gdiplus::Pen* pen, Gdiplus::Brush* brush) {
                    graphics.FillRectangle(brush, micX - Scale(2), micY - Scale(4), Scale(4), Scale(6));
                    graphics.FillEllipse(brush, micX - Scale(2), micY - Scale(6), Scale(4), Scale(4));
                    graphics.FillEllipse(brush, micX - Scale(2), micY, Scale(4), Scale(4));
                    graphics.DrawArc(pen, micX - Scale(5), micY - Scale(2), Scale(10), Scale(8), 0.0f, 180.0f);
                    graphics.DrawLine(pen, micX, micY + Scale(6), micX, micY + Scale(8));
                    graphics.DrawLine(pen, micX - Scale(3), micY + Scale(8), micX + Scale(3), micY + Scale(8));
                };
                
                int blackHeight = totalHeight - fillHeight;
                if (blackHeight > 0) {
                    Gdiplus::Rect blackClip(micX - Scale(10), micTop, Scale(20), blackHeight);
                    graphics.SetClip(blackClip);
                    DrawMic(&blackPen, &blackBrush);
                    graphics.ResetClip();
                }
                
                if (fillHeight > 0) {
                    Gdiplus::Rect gradientClip(micX - Scale(10), micBottom - fillHeight, Scale(20), fillHeight);
                    graphics.SetClip(gradientClip);
                    DrawMic(&gradientPen, &linGrBrush);
                    graphics.ResetClip();
                }
            }
            } // End GDI+ scope
            
            HFONT hListFont = (HFONT)SendMessage(lpdis->hwndItem, WM_GETFONT, 0, 0);
            HFONT hOldMemFont = (HFONT)SelectObject(hdc, hListFont);
            
            HFONT hOldFont = NULL;
            if (user == g_username) {
                hOldFont = (HFONT)SelectObject(hdc, g_hFontBold);
            }
            
            std::wstring displayUser = user;
            size_t hashPos = displayUser.find(L'#');
            if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);
            
            DrawTextW(hdc, displayUser.c_str(), -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            if (hOldFont) {
                SelectObject(hdc, hOldFont);
            }
            
            SelectObject(hdc, hOldMemFont);
            
            BitBlt(hdcOriginal, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdc, rc.left, rc.top, SRCCOPY);
            SelectObject(hdc, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hdc);
            
            return TRUE;
        }
        break;
    }

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        int inputHeight = Scale(40);
        int btnWidth = Scale(60);
        int listWidth = Scale(150);
        int margin = Scale(5);
        int toolHeight = Scale(24);
        int roomBtnHeight = Scale(30);

        int topHeight = height - inputHeight - toolHeight - margin * 4;
        int logWidth = width - listWidth - margin * 3;
        
        MoveWindow(g_hLog, margin, margin, logWidth, topHeight, TRUE);

        int listHeight = topHeight - roomBtnHeight - margin;
        MoveWindow(g_hUserList, margin * 2 + logWidth, margin, listWidth, listHeight, TRUE);

        int halfList = (listWidth - margin) / 2;
        int roomBtnY = margin * 2 + listHeight;
        MoveWindow(g_hBtnMoveRoom, margin * 2 + logWidth, roomBtnY, listWidth, roomBtnHeight, TRUE);

        int toolY = margin * 2 + topHeight;
        MoveWindow(g_hBtnB, margin, toolY, Scale(24), toolHeight, TRUE);
        MoveWindow(g_hBtnI, margin + Scale(28), toolY, Scale(24), toolHeight, TRUE);
        MoveWindow(g_hBtnU, margin + Scale(56), toolY, Scale(24), toolHeight, TRUE);
        MoveWindow(g_hComboSize, margin + Scale(84), toolY, Scale(60), Scale(400), TRUE);
        RECT rcCombo;
        GetWindowRect(g_hComboSize, &rcCombo);
        int currentHeight = rcCombo.bottom - rcCombo.top;
        int currentItemHeight = SendMessage(g_hComboSize, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
        int nonItemHeight = currentHeight - currentItemHeight;
        SendMessage(g_hComboSize, CB_SETITEMHEIGHT, (WPARAM)-1, toolHeight - nonItemHeight);
        MoveWindow(g_hBtnColor, margin + Scale(148), toolY, Scale(32), toolHeight, TRUE);
        MoveWindow(g_hBtnVoice, margin + Scale(186), toolY, Scale(70), toolHeight, TRUE);
        MoveWindow(g_hVolumeIndicator, margin + Scale(261), toolY, Scale(70), toolHeight, TRUE);
        MoveWindow(g_hSpeakerIcon, margin + Scale(336), toolY, Scale(20), toolHeight, TRUE);
        MoveWindow(g_hVolumeSlider, margin + Scale(356), toolY, Scale(100), toolHeight, TRUE);
        // MoveWindow(g_hBtnVideo, margin + Scale(466), toolY, Scale(80), toolHeight, TRUE); // Hidden until video is restored

        int bottomY = margin * 3 + topHeight + toolHeight;
        MoveWindow(g_hInput, margin, bottomY, width - btnWidth - margin * 3, inputHeight, TRUE);
        MoveWindow(g_hSendBtn, width - btnWidth - margin, bottomY, btnWidth, inputHeight, TRUE);

        return 0;
    }

    case WM_HSCROLL: {
        if ((HWND)lParam == g_hVolumeSlider) {
            if (g_pSimpleAudioVolume) {
                int pos = SendMessage(g_hVolumeSlider, TBM_GETPOS, 0, 0);
                float volume = pos / 100.0f;
                g_pSimpleAudioVolume->SetMasterVolume(volume, NULL);
            }
        }
        return 0;
    }

    case WM_APP_VOLUME_CHANGED: {
        if (g_hVolumeSlider) {
            SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, wParam);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        
        if (id == 1001 && code == BN_CLICKED) {
            if (!g_inputText.empty()) {
                std::wstring msg = g_inputText;
                SendChatMessage(msg);
                g_inputText.clear();
                g_caretPos = 0;
                g_anchorPos = 0;
                UpdateInputFormat();
            }
            SetFocus(g_hInput);
        }
        else if (id == 1002 && code == BN_CLICKED) {
            CHOOSECOLOR cc;
            static COLORREF acrCustClr[16]; 
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hWnd;
            cc.lpCustColors = (LPDWORD)acrCustClr;
            cc.rgbResult = g_currentColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&cc)) {
                g_currentColor = cc.rgbResult;
                UpdateInputFormat();
                SetFocus(g_hInput);
            }
        }
        else if ((id >= 1003 && id <= 1005 && code == BN_CLICKED) || (id == 1006 && code == CBN_SELCHANGE)) {
            UpdateInputFormat();
            SetFocus(g_hInput);
        }

        else if (id == 1009 && code == BN_CLICKED) {
            g_isVoiceActive = !g_isVoiceActive;
            if (g_isVoiceActive) {
                waveInStart(g_hWaveIn);
            } else {
                waveInStop(g_hWaveIn);
                g_currentMicVolume = 0;
                InvalidateRect(g_hVolumeIndicator, NULL, FALSE);
            }
            InvalidateRect(g_hBtnVoice, NULL, FALSE);
            
            // ユーザリスト上の自分のマイクアイコンを再描画
            if (g_hUserList) {
                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_username.c_str());
                if (idx != LB_ERR) {
                    RECT rc;
                    SendMessage(g_hUserList, LB_GETITEMRECT, idx, (LPARAM)&rc);
                    InvalidateRect(g_hUserList, &rc, FALSE);
                }
            }
        }
        else if (id == 1010 && code == BN_CLICKED) {
            // Video toggle (TODO)
        }
        else if (id == 1008 && code == BN_CLICKED) {
            std::wstring newRoom;
            bool isPrivate = false;
            if (ShowMoveRoomDialog(hWnd, newRoom, isPrivate)) { 
                SendLeaveRequest(g_currentRoom);
                
                // Close video windows and stop webcam on room change
                g_isWebcamActive = false;
                for (auto& pair : g_videoWindows) {
                    if (pair.second && IsWindow(pair.second)) {
                        SendMessage(pair.second, WM_CLOSE, 0, 0);
                    }
                }
                g_videoWindows.clear();

                SendJoinRequest(newRoom);
                g_currentRoom = newRoom;
                g_isPrivateRoom = isPrivate;
                if (!isPrivate) {
                    g_publicRooms[newRoom] = (std::max)(1, g_publicRooms[newRoom]);
                }
                ClearLogAndUsers();
                SetWindowTitle(hWnd);
                MsgData sysMsg;
                sysMsg.user = L"";
                WCHAR moveBuf[512];
                swprintf_s(moveBuf, GetString(IDS_MOVED_TO).c_str(), g_currentRoom.c_str());
                sysMsg.msg = moveBuf;
                sysMsg.b = false; sysMsg.i = false; sysMsg.u = false;
                sysMsg.size = 9; sysMsg.color = RGB(128,128,128);
                AppendLog(sysMsg);
            }
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == 1) {
            // Keep UDP NAT mapping alive with heartbeat
            if (g_sessionId != 0 && g_sock != INVALID_SOCKET) {
                MediaHeader hdr = {0};
                hdr.sessionId = g_sessionId;
                hdr.payloadType = 255;
                sockaddr_in serverUdp = {0};
                serverUdp.sin_family = AF_INET;
                serverUdp.sin_port = htons(4001);
                if (g_isServerUdpAddrResolved) {
                    serverUdp.sin_addr = g_serverUdpAddr;
                    sendto(g_sock, (char*)&hdr, sizeof(hdr), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                }
            }
            
            bool listChanged = false;
            DWORD now = GetTickCount();
            {
                std::lock_guard<std::mutex> lock(g_streamerMutex);
                for (auto it = g_videoStreamers.begin(); it != g_videoStreamers.end(); ) {
                    if (now - it->second > 5000) {
                        listChanged = true;
                        it = g_videoStreamers.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            if (listChanged) {
                InvalidateRect(g_hUserList, NULL, FALSE);
            }
        }
        else if (wParam == 2) {
            bool listChanged = false;
            DWORD now = GetTickCount();
            {
                std::lock_guard<std::mutex> lock(g_voiceMutex);
                for (auto it = g_voiceUsers.begin(); it != g_voiceUsers.end(); ) {
                    if (now - it->second > 1000) {
                        listChanged = true;
                        g_voiceVolumes.erase(it->first);
                        it = g_voiceUsers.erase(it);
                    } else {
                        if (g_hUserList) {
                            int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)it->first.c_str());
                            if (idx != LB_ERR) {
                                RECT rc;
                                SendMessage(g_hUserList, LB_GETITEMRECT, idx, (LPARAM)&rc);
                                InvalidateRect(g_hUserList, &rc, FALSE);
                            }
                        }
                        ++it;
                    }
                }
            }
            if (listChanged && g_hUserList) {
                InvalidateRect(g_hUserList, NULL, FALSE);
            }
            if (g_isVoiceActive && g_hUserList) {
                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_username.c_str());
                if (idx != LB_ERR) {
                    RECT rc;
                    SendMessage(g_hUserList, LB_GETITEMRECT, idx, (LPARAM)&rc);
                    InvalidateRect(g_hUserList, &rc, FALSE);
                }
            }
        }
        return 0;
    }

    case WM_USER_MSG: {
        MsgData* pData = (MsgData*)lParam;
        if (pData) {
            AppendLog(*pData);
            delete pData;
        }
        return 0;
    }

    case WM_NEW_ROOM: {
        std::wstring* pRm = (std::wstring*)lParam;
        if (pRm) {
            g_publicRooms[*pRm] = (std::max)(1, g_publicRooms[*pRm]);
            delete pRm;
        }
        return 0;
    }

    case WM_SETFOCUS:
        SetFocus(g_hInput);
        return 0;

    case WM_REVERT_ROOM: {
        SendLeaveRequest(g_currentRoom);
        g_currentRoom = GetString(IDS_LOBBY);
        SendJoinRequest(g_currentRoom);
        ClearLogAndUsers();
        SetWindowTitle(hWnd);
        MsgData sysMsg;
        sysMsg.user = L"";
        sysMsg.msg = GetString(IDS_RETURNED_TO_LOBBY);
        sysMsg.b = false; sysMsg.i = false; sysMsg.u = false;
        sysMsg.size = 9; sysMsg.color = RGB(255,0,0);
        AppendLog(sysMsg);
        return 0;
    }

    case WM_DESTROY:
        SendLeaveRequest(g_currentRoom);
        closesocket(g_sock);
        WSACleanup();
        DeleteObject(g_hFont);
        DeleteObject(g_hFontBold);
        DeleteObject(g_hFontI_btn);
        DeleteObject(g_hFontU_btn);
        if (g_pAudioSessionControl && g_pAudioEvents) {
            g_pAudioSessionControl->UnregisterAudioSessionNotification(g_pAudioEvents);
            g_pAudioEvents->Release();
            g_pAudioEvents = nullptr;
        }
        if (g_pAudioSessionControl) { g_pAudioSessionControl->Release(); g_pAudioSessionControl = nullptr; }
        if (g_pSimpleAudioVolume) { g_pSimpleAudioVolume->Release(); g_pSimpleAudioVolume = nullptr; }
        CoUninitialize();

        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#include "resource.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    SetThreadUILanguage(0);
    g_currentRoom = GetString(IDS_LOBBY);
    
    HDC hdcDpi = GetDC(NULL);
    g_dpi = GetDeviceCaps(hdcDpi, LOGPIXELSX);
    ReleaseDC(NULL, hdcDpi);

    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);
    
    MSG msg;
    WNDCLASS wc = {
        CS_HREDRAW | CS_VREDRAW,
        WndProc, 0, 0, hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)), LoadCursor(0, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE + 1), 0, L"Retro Chat"
    };
    RegisterClass(&wc);
    
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HWND hWnd = CreateWindow(
        L"Retro Chat", GetString(IDS_WINDOW_TITLE).c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, Scale(500), Scale(400),
        0, 0, hInstance, 0);
        
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
