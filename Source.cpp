// Visual styles removed for classic look

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <fstream>
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
#include <cmath>8
#include <algorithm>
#include "H264Helper.h"
bool g_useH264 = true;
H264Encoder* g_pH264Encoder = nullptr;
H264Decoder* g_pH264Decoder = nullptr;
#include "json11.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#pragma comment(lib, "ole32.lib")


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "imm32.lib")

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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

void LogToFile(const std::string& msg) {
    FILE* fp = NULL;
    fopen_s(&fp, "chat.log", "a");
    if (fp) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(fp, "[%02d:%02d:%02d.%03d] %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg.c_str());
        fclose(fp);
    }
}
#define WM_NEW_ROOM (WM_APP + 3)
#define WM_REVERT_ROOM (WM_APP + 4)

HWND g_hLog, g_hInput, g_hSendBtn, g_hUserList, g_hUserListHeader;
HWND g_hBtnB, g_hBtnI, g_hBtnU, g_hComboSize, g_hBtnColor;
HWND g_hBtnMoveRoom;
HWND g_hBtnVoice, g_hBtnVideo, g_hVolumeIndicator, g_hSpeakerIcon, g_hVolumeSlider;
IAudioClient* g_pAudioClient = nullptr;
ISimpleAudioVolume* g_pSimpleAudioVolume = nullptr;
IAudioSessionControl* g_pAudioSessionControl = nullptr;

int g_currentMicVolume = 0;
COLORREF g_currentColor = RGB(0, 0, 0);
COLORREF g_nameColor = 0xFFFFFFFF;
std::map<std::wstring, COLORREF> g_userColors;
std::map<std::wstring, std::wstring> g_userNames;
bool g_isNameChanging = false;
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
        PostMessage(GetParent(g_hVolumeSlider), WM_APP_VOLUME_CHANGED, (WPARAM)(NewVolume * 100), (LPARAM)NewMute);
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
std::wstring g_secretId = L"";
std::wstring g_userId = L"";
std::wstring g_username;
int g_dpi = 96;
inline int Scale(int x) { return MulDiv(x, g_dpi, 96); }

HFONT g_hFont;
HFONT g_hFontBold;
HFONT g_hFontI_btn;
HFONT g_hFontU_btn;
HFONT g_hFontB_btn;
HFONT g_hFontCombo;

std::map<std::wstring, int> g_publicRooms;
std::wstring g_currentRoom = L"";
bool g_isPrivateRoom = false;

#define WM_USER_ROOM_LIST_UPDATED (WM_USER + 2)
HWND g_hMoveRoomDlg = NULL;

ID2D1Factory* g_pD2DFactory = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;

#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <mutex>
#include <map>
#include <thread>
#include <wincrypt.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

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

std::wstring GetIgnoredUsersFilePath() {
    wchar_t appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\chat_app";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\ignored.txt";
    }
    return L"ignored.txt";
}

void LoadIgnoredUsers() {
    std::lock_guard<std::mutex> lock(g_ignoredMutex);
    g_ignoredUsers.clear();
    std::wifstream in(GetIgnoredUsersFilePath());
    if (in.is_open()) {
        std::wstring line;
        while (std::getline(in, line)) {
            if (!line.empty()) g_ignoredUsers.insert(line);
        }
    }
}

void SaveIgnoredUsers() {
    std::lock_guard<std::mutex> lock(g_ignoredMutex);
    std::wofstream out(GetIgnoredUsersFilePath(), std::ios::trunc);
    if (out.is_open()) {
        for (const auto& u : g_ignoredUsers) {
            out << u << L"\n";
        }
    }
}

void SortUserList() {
    extern HWND g_hUserList;
    if (!g_hUserList) return;
    int count = (int)SendMessage(g_hUserList, LB_GETCOUNT, 0, 0);
    if (count <= 1) return;

    std::vector<std::wstring> users;
    for (int i = 0; i < count; ++i) {
        int len = (int)SendMessage(g_hUserList, LB_GETTEXTLEN, i, 0);
        if (len > 0) {
            std::vector<WCHAR> buf(len + 1, 0);
            SendMessage(g_hUserList, LB_GETTEXT, i, (LPARAM)buf.data());
            users.push_back(std::wstring(buf.data()));
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_ignoredMutex);
        std::sort(users.begin(), users.end(), [](const std::wstring& a, const std::wstring& b) {
            bool a_ignored = (g_ignoredUsers.count(a) > 0);
            bool b_ignored = (g_ignoredUsers.count(b) > 0);
            if (a_ignored != b_ignored) {
                return !a_ignored; // false (not ignored) goes before true (ignored)
            }
            std::wstring name_a = a;
            if (g_userNames.count(a)) name_a = g_userNames[a];
            std::wstring name_b = b;
            if (g_userNames.count(b)) name_b = g_userNames[b];
            return _wcsicmp(name_a.c_str(), name_b.c_str()) < 0;
        });
    }

    SendMessage(g_hUserList, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
    for (const auto& u : users) {
        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)u.c_str());
    }
    SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hUserList, NULL, TRUE);
}

std::wstring FilterProfanity(std::wstring text) {
    std::wstring ng_words = GetString(IDS_NG_WORDS);
    size_t start = 0;
    size_t end = ng_words.find(L',');
    while (end != std::wstring::npos) {
        std::wstring word = ng_words.substr(start, end - start);
        if (!word.empty()) {
            size_t pos = 0;
            while ((pos = text.find(word, pos)) != std::wstring::npos) {
                text.replace(pos, word.length(), L"***");
                pos += 3;
            }
        }
        start = end + 1;
        end = ng_words.find(L',', start);
    }
    if (start < ng_words.length()) {
        std::wstring word = ng_words.substr(start);
        if (!word.empty()) {
            size_t pos = 0;
            while ((pos = text.find(word, pos)) != std::wstring::npos) {
                text.replace(pos, word.length(), L"***");
                pos += 3;
            }
        }
    }
    return text;
}

void DecodeJPEG(const std::vector<uint8_t>& jpegData, const std::wstring& username) {
    if (jpegData.empty()) {
        LogToFile("DecodeJPEG: jpegData is empty");
        return;
    }
    IStream* pStream = NULL;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, jpegData.size());
    void* pData = GlobalLock(hGlobal);
    memcpy(pData, jpegData.data(), jpegData.size());
    GlobalUnlock(hGlobal);
    CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
    
    IWICBitmapDecoder* pDecoder = NULL;
    HRESULT hrDec = g_pWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hrDec)) {
        IWICBitmapFrameDecode* pFrame = NULL;
        HRESULT hrFrame = pDecoder->GetFrame(0, &pFrame);
        if (SUCCEEDED(hrFrame)) {
            UINT width, height;
            pFrame->GetSize(&width, &height);
            
            IWICFormatConverter* pConverter = NULL;
            g_pWICFactory->CreateFormatConverter(&pConverter);
            HRESULT hrConv = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hrConv)) {
                std::vector<uint8_t> pixels(width * height * 4);
                HRESULT hrCopy = pConverter->CopyPixels(NULL, width * 4, pixels.size(), pixels.data());
                if (SUCCEEDED(hrCopy)) {
                    std::lock_guard<std::mutex> lock(g_videoMutex);
                    g_videoSizeMap[username] = {width, height};
                    g_videoPixelsMap[username] = std::move(pixels);
                    static DWORD lastDecLog = 0;
                    if (GetTickCount() - lastDecLog > 1000) {
                        lastDecLog = GetTickCount();
                        char buf[256];
                        std::string unameA(username.begin(), username.end());
                        sprintf_s(buf, "DecodeJPEG SUCCESS: %dx%d for %s", width, height, unameA.c_str());
                        LogToFile(buf);
                    }
                } else {
                    char buf[256]; sprintf_s(buf, "DecodeJPEG FAILED: CopyPixels hr=0x%X", hrCopy); LogToFile(buf);
                }
            } else {
                char buf[256]; sprintf_s(buf, "DecodeJPEG FAILED: Initialize FormatConverter hr=0x%X", hrConv); LogToFile(buf);
            }
            pConverter->Release();
            pFrame->Release();
        } else {
            char buf[256]; sprintf_s(buf, "DecodeJPEG FAILED: GetFrame hr=0x%X", hrFrame); LogToFile(buf);
        }
        pDecoder->Release();
    } else {
        char buf[256]; sprintf_s(buf, "DecodeJPEG FAILED: CreateDecoderFromStream hr=0x%X", hrDec); LogToFile(buf);
    }
    pStream->Release();
    HWND hView = NULL;
    auto it = g_videoWindows.find(username);
    if (it != g_videoWindows.end()) {
        hView = it->second;
    }

    if (hView && IsWindow(hView)) {
        InvalidateRect(hView, NULL, FALSE);
    } else {
        std::string unameA(username.begin(), username.end());
        LogToFile("DecodeJPEG: Target video window not found for " + unameA);
    }
}

// --- Audio Globals ---
HWAVEIN g_hWaveIn = NULL;
HWAVEOUT g_hWaveOut = NULL;
bool g_isVoiceActive = false;
bool g_suspectMicDead = false;

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
        DWORD dwNameColor = 0;
        cbData = sizeof(dwNameColor);
        type = REG_DWORD;
        if (RegQueryValueExW(hKey, L"NameColor", NULL, &type, (LPBYTE)&dwNameColor, &cbData) == ERROR_SUCCESS) {
            g_nameColor = dwNameColor;
        }
        if (RegQueryValueExW(hKey, L"SecretId", NULL, &type, (LPBYTE)buf, &cbData) == ERROR_SUCCESS) {
            if (wcslen(buf) > 0) g_secretId = buf;
        }
        RegCloseKey(hKey);
    }
    
    if (g_secretId.empty()) {
        UUID uuid;
        UuidCreate(&uuid);
        RPC_WSTR str;
        if (UuidToStringW(&uuid, &str) == RPC_S_OK) {
            g_secretId = (WCHAR*)str;
            RpcStringFreeW(&str);
        } else {
            g_secretId = L"fallback-uuid";
        }
    }

    if (g_nameColor == 0xFFFFFFFF) {
        srand(GetTickCount());
        COLORREF palette[] = {
            RGB(200, 50, 50), RGB(50, 150, 50), RGB(50, 50, 200),
            RGB(180, 180, 50), RGB(180, 50, 180), RGB(50, 180, 180),
            RGB(255, 128, 0), RGB(128, 0, 255), RGB(0, 128, 255),
            RGB(128, 200, 0), RGB(255, 0, 128), RGB(0, 200, 128)
        };
        g_nameColor = palette[rand() % 12];
    }
}

void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\RetroChat", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Username", 0, REG_SZ, (const BYTE*)g_username.c_str(), (DWORD)((g_username.length() + 1) * sizeof(WCHAR)));
        RegSetValueExW(hKey, L"SecretId", 0, REG_SZ, (const BYTE*)g_secretId.c_str(), (DWORD)((g_secretId.length() + 1) * sizeof(WCHAR)));
        RegSetValueExW(hKey, L"NameColor", 0, REG_DWORD, (const BYTE*)&g_nameColor, sizeof(DWORD));
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
    wchar_t userId[40];
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
    IWICFormatConverter* pConverter = NULL;
    if (SUCCEEDED(g_pWICFactory->CreateFormatConverter(&pConverter))) {
        pConverter->Initialize(pWicBitmap, GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
        pFrame->WriteSource(pConverter, NULL);
        pConverter->Release();
    }
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
            static DWORD lastStreamStatusTime = 0;
            if (GetTickCount() - lastStreamStatusTime > 2000) {
                lastStreamStatusTime = GetTickCount();
                char udpPkt[sizeof(MediaHeader)] = {0};
                MediaHeader* mhdr = (MediaHeader*)udpPkt;
                mhdr->sessionId = g_sessionId;
                mhdr->payloadType = 4; // StreamStatus
                mhdr->timestamp = GetTickCount();
                wcsncpy_s(mhdr->userId, 40, g_userId.c_str(), _TRUNCATE);
                sockaddr_in serverUdp = {0};
                serverUdp.sin_family = AF_INET;
                serverUdp.sin_port = htons(4001);
                if (!g_isServerUdpAddrResolved) {
                    g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                }
                serverUdp.sin_addr = g_serverUdpAddr;
                sendto(g_sock, udpPkt, sizeof(MediaHeader), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
            }

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

            if (g_useH264) {
                if (!g_pH264Encoder) {
                    g_pH264Encoder = new H264Encoder();
                    g_pH264Encoder->Initialize(width, height);
                }
                std::vector<uint8_t> h264;
                if (g_pH264Encoder->EncodeFrame(pData, cbData, h264)) {
                    std::cout << "[Webcam] Encoded H.264 frame, size: " << h264.size() << " time: " << GetTickCount() << std::endl;
                    const int MAX_PAYLOAD = 1000;
                    int totalFragments = (h264.size() + MAX_PAYLOAD - 1) / MAX_PAYLOAD;
                    g_videoSeq++;
                    LARGE_INTEGER freq;
                    QueryPerformanceFrequency(&freq);
                    long long delay_counts = freq.QuadPart / 5000; // 0.2 ms pacing delay
                    
                    for (int i = 0; i < totalFragments; i++) {
                        int offset = i * MAX_PAYLOAD;
                        int len = min(MAX_PAYLOAD, (int)h264.size() - offset);
                        
                        char udpPkt[2048];
                        MediaHeader* mhdr = (MediaHeader*)udpPkt;
                        mhdr->sessionId = g_sessionId;
                        mhdr->seqNum = g_videoSeq;
                        mhdr->payloadType = 5; 
                        mhdr->flags = (i == 0 ? 1 : 0) | (i == totalFragments - 1 ? 2 : 0);
                        mhdr->timestamp = i; 
                        wcsncpy_s(mhdr->userId, 40, g_userId.c_str(), _TRUNCATE);
                        
                        memcpy(udpPkt + sizeof(MediaHeader), h264.data() + offset, len);
                        
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
                        std::vector<uint8_t> dummyJpeg;
                        if (EncodeJPEG(width, height, pData, cbData, dummyJpeg)) {
                            DecodeJPEG(dummyJpeg, g_userId);
                        }
                }
            } else {
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
                        wcsncpy_s(mhdr->userId, 40, g_userId.c_str(), _TRUNCATE);
                        
                        memcpy(udpPkt + sizeof(MediaHeader), jpegData.data() + offset, len);
                        
                        sockaddr_in serverUdp = {0};
                        serverUdp.sin_family = AF_INET;
                        serverUdp.sin_port = htons(4001);
                        if (!g_isServerUdpAddrResolved) {
                            g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                        }
                        serverUdp.sin_addr = g_serverUdpAddr;
                        int sent = sendto(g_sock, udpPkt, sizeof(MediaHeader) + len, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                        LogToFile("UDP Sending Video Fragment, len=" + std::to_string(len) + ", sent=" + std::to_string(sent));

                        LARGE_INTEGER start, current;
                        QueryPerformanceCounter(&start);
                        do {
                            QueryPerformanceCounter(&current);
                        } while (current.QuadPart - start.QuadPart < delay_counts);
                    }
                    DecodeJPEG(jpegData, g_userId);
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
            wcsncpy_s(mhdr->userId, 40, g_userId.c_str(), _TRUNCATE);
            
            memcpy(udpPkt + sizeof(MediaHeader), pHdr->lpData, pHdr->dwBytesRecorded);
            
            sockaddr_in serverUdp = {0};
            serverUdp.sin_family = AF_INET;
            serverUdp.sin_port = htons(4001);
            if (!g_isServerUdpAddrResolved) {
                g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
            }
            serverUdp.sin_addr = g_serverUdpAddr;
            
            int sent = sendto(g_sock, udpPkt, sizeof(MediaHeader) + pHdr->dwBytesRecorded, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
            LogToFile("UDP Sending Audio Packet, len=" + std::to_string(pHdr->dwBytesRecorded) + ", sent=" + std::to_string(sent));
        }
        if (g_isVoiceActive) {
            // If the buffer was aborted (e.g. unplugged), do not re-add it.
            // Calling waveInAddBuffer on a dead device causes a permanent deadlock in winmm.
            if (pHdr->dwBytesRecorded < AUDIO_BUF_SIZE) {
                g_suspectMicDead = true;
                g_hWaveIn = NULL;
            } else {
                if (waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
                    g_suspectMicDead = true;
                    g_hWaveIn = NULL;
                }
            }
        }
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

bool InitAudioIn() {
    if (g_hWaveIn) return true;

    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 16000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveInOpen(&g_hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        g_hWaveIn = NULL;
        return false;
    }

    for (int i = 0; i < AUDIO_BUF_COUNT; i++) {
        g_waveInHdr[i].lpData = g_waveInBuf[i];
        g_waveInHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
        g_waveInHdr[i].dwFlags = 0;
        waveInPrepareHeader(g_hWaveIn, &g_waveInHdr[i], sizeof(WAVEHDR));
        waveInAddBuffer(g_hWaveIn, &g_waveInHdr[i], sizeof(WAVEHDR));
    }
    return true;
}

void CleanupAudioIn() {
    if (g_hWaveIn) {
        MMTIME mmt;
        mmt.wType = TIME_BYTES;
        bool isDead = false;
        if (waveInGetPosition(g_hWaveIn, &mmt, sizeof(mmt)) != MMSYSERR_NOERROR) isDead = true;

        if (isDead || g_suspectMicDead) {
            // Avoid winmm deadlock on disconnected devices
            g_hWaveIn = NULL;
            g_suspectMicDead = false;
            return;
        }

        waveInStop(g_hWaveIn);
        waveInReset(g_hWaveIn);
        for (int i = 0; i < AUDIO_BUF_COUNT; i++) {
            waveInUnprepareHeader(g_hWaveIn, &g_waveInHdr[i], sizeof(WAVEHDR));
        }
        waveInClose(g_hWaveIn);
        g_hWaveIn = NULL;
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

    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

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
    COLORREF nameColor = 0xFFFFFFFF;
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

bool g_isRoomJoined = false;
void UpdateSendBtnState() {
    if (g_hSendBtn && IsWindow(g_hSendBtn)) {
        bool shouldEnable = g_isRoomJoined && (g_inputText.length() > 0);
        bool isEnabled = (IsWindowEnabled(g_hSendBtn) != FALSE);
        if (isEnabled != shouldEnable) {
            EnableWindow(g_hSendBtn, shouldEnable);
        }
    }
}
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
                outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
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
            outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
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

bool g_logIsDraggingScrollbar = false;

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
                        L"MS UI Gothic", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                        9.0f * 1.5f, L"ja-JP", &pFormat); // MS UI Gothic
                        
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
                                COLORREF uCol = (entry.data.nameColor != 0xFFFFFFFF) ? entry.data.nameColor : palette[hash % 12];
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
            si.fMask = SIF_RANGE | SIF_PAGE | (g_logIsDraggingScrollbar ? 0 : SIF_POS);
            si.nMin = 0;
            si.nMax = (int)g_logTotalHeight;
            si.nPage = (int)viewH;
            si.nPos = (int)g_logScrollY;
            SetScrollInfo(hWnd, SB_VERT, &si, FALSE);
            SendMessage(hWnd, WM_NCPAINT, 1, 0);

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
                                    DWORD hlColor = RGB(0, 0, 128);
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

                                    DWORD hlText = RGB(255, 255, 255);
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
                            COLORREF uCol = (entry.data.nameColor != 0xFFFFFFFF) ? entry.data.nameColor : palette[hash % 12];
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
        else if (action == SB_THUMBTRACK || action == SB_THUMBPOSITION) {
            SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
            GetScrollInfo(hWnd, SB_VERT, &si);
            g_logScrollY = si.nTrackPos;
        }
        extern bool g_logIsDraggingScrollbar;
        if (action == SB_THUMBTRACK) g_logIsDraggingScrollbar = true;
        if (action == SB_ENDSCROLL) g_logIsDraggingScrollbar = false;
        
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
    int tsize = 10;
    if (sizeIndex != CB_ERR) {
        WCHAR szText[32];
        SendMessage(g_hComboSize, CB_GETLBTEXT, sizeIndex, (LPARAM)szText);
        tsize = _wtoi(szText);
    }
    
    IDWriteTextFormat* pFormat = nullptr;
    DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE style = i ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    
    HRESULT hrFormat = g_pDWriteFactory->CreateTextFormat(
        L"MS UI Gothic", NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
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

void UpdateImeWindowPos(HWND hWnd) {
    HIMC hImc = ImmGetContext(hWnd);
    if (!hImc) return;

    IDWriteTextLayout* pLayout = CreateInputLayout(hWnd);
    if (pLayout) {
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
            hm.height = tsize * 1.5f;
        } else if (g_caretPos >= g_inputText.length()) {
            pLayout->HitTestTextPosition((UINT32)g_inputText.length() - 1, TRUE, &cx, &cy, &hm);
            cx += 5; cy += 5;
        } else {
            pLayout->HitTestTextPosition((UINT32)g_caretPos, FALSE, &cx, &cy, &hm);
            cx += 5; cy += 5;
        }
        
        COMPOSITIONFORM cf = { 0 };
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = Scale((LONG)cx);
        cf.ptCurrentPos.y = Scale((LONG)cy);
        ImmSetCompositionWindow(hImc, &cf);

        LOGFONTW lf = { 0 };
        lf.lfHeight = -Scale((LONG)(hm.height));
        wcscpy_s(lf.lfFaceName, L"MS UI Gothic");
        ImmSetCompositionFontW(hImc, &lf);

        pLayout->Release();
    }
    ImmReleaseContext(hWnd, hImc);
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
    case WM_IME_COMPOSITION: {
        if (lParam & GCS_RESULTSTR) {
            HIMC hImc = ImmGetContext(hWnd);
            if (hImc) {
                LONG size = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, NULL, 0);
                if (size > 0) {
                    WCHAR* buf = new WCHAR[size / 2 + 1];
                    ImmGetCompositionStringW(hImc, GCS_RESULTSTR, buf, size);
                    buf[size / 2] = 0;
                    
                    if (g_caretPos != g_anchorPos) {
                        int start = min(g_caretPos, g_anchorPos);
                        int end = max(g_caretPos, g_anchorPos);
                        g_inputText.erase(start, end - start);
                        g_caretPos = start;
                        g_anchorPos = start;
                    }
                    if (g_inputText.length() + wcslen(buf) < 1024) {
                        g_inputText.insert(g_caretPos, buf);
                        g_caretPos += (int)wcslen(buf);
                        g_anchorPos = g_caretPos;
                        g_caretVisible = true;
                    } else {
                        MessageBeep(MB_OK);
                    }
                    delete[] buf;
                    
                    UpdateImeWindowPos(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                ImmReleaseContext(hWnd, hImc);
            }
            lParam &= ~GCS_RESULTSTR;
        }
        if (lParam == 0) return 0;
        break;
    }
    case WM_IME_STARTCOMPOSITION:
        g_isIMEComposing = true;
        UpdateImeWindowPos(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_IME_ENDCOMPOSITION:
        g_isIMEComposing = false;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_PAINT: {
        UpdateSendBtnState();
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
                        
                        DWORD hlColor = RGB(0, 0, 128);
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

                        DWORD hlText = RGB(255, 255, 255);
                        float tr = GetRValue(hlText) / 255.0f;
                        float tg = GetGValue(hlText) / 255.0f;
                        float tb = GetBValue(hlText) / 255.0f;
                        ID2D1SolidColorBrush* pTextBrush = nullptr;
                        if (SUCCEEDED(pRT->CreateSolidColorBrush(D2D1::ColorF(tr, tg, tb, 1.0f), &pTextBrush)) && pTextBrush) {
                            DWRITE_TEXT_RANGE selRange = { (UINT32)selStart, (UINT32)(selLen) };
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
                            bool isItalic = SendMessage(g_hBtnI, BM_GETCHECK, 0, 0) == BST_CHECKED;
                            if (!isItalic) pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                            float slant = isItalic ? (hm.height * 0.25f) : 0.0f;
                            pRT->DrawLine(D2D1::Point2F(cx + slant, cy), D2D1::Point2F(cx, cy + hm.height), pCaretBrush, 2.0f * 96.0f / g_dpi);
                            pRT->SetAntialiasMode(oldMode);
                            pCaretBrush->Release();
                        }
                        
                        UpdateImeWindowPos(hWnd);
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
            if (IsWindowEnabled(g_hSendBtn)) {
                SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(1001, BN_CLICKED), 0);
            }
        } else if (wParam == 'V' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_PASTE, 0, 0);
        } else if (wParam == 'C' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_COPY, 0, 0);
        } else if (wParam == 'X' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, WM_CUT, 0, 0);
        } else if (wParam == 'A' && GetKeyState(VK_CONTROL) < 0) {
            g_anchorPos = 0;
            g_caretPos = (int)g_inputText.length();
        } else if (wParam == 'B' && GetKeyState(VK_CONTROL) < 0) {
            bool checked = SendMessage(g_hBtnB, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SendMessage(g_hBtnB, BM_SETCHECK, checked ? BST_UNCHECKED : BST_CHECKED, 0);
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(1003, BN_CLICKED), (LPARAM)g_hBtnB);
        } else if (wParam == 'I' && GetKeyState(VK_CONTROL) < 0) {
            bool checked = SendMessage(g_hBtnI, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SendMessage(g_hBtnI, BM_SETCHECK, checked ? BST_UNCHECKED : BST_CHECKED, 0);
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(1004, BN_CLICKED), (LPARAM)g_hBtnI);
        } else if (wParam == 'U' && GetKeyState(VK_CONTROL) < 0) {
            bool checked = SendMessage(g_hBtnU, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SendMessage(g_hBtnU, BM_SETCHECK, checked ? BST_UNCHECKED : BST_CHECKED, 0);
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(1005, BN_CLICKED), (LPARAM)g_hBtnU);
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
#ifndef WM_GETTITLEBARINFOEX
#define WM_GETTITLEBARINFOEX 0x033F
#endif

HWND g_capturedWnd = NULL;
int g_pressedBtn = 0;
int g_hoverBtn = 0;

void ApplyClassicStyle(HWND hWnd) {
    SetWindowTheme(hWnd, L" ", L" ");
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    RECT rc;
    GetWindowRect(hWnd, &rc);
    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
    int dpiY = 96;
    HDC hdc = GetDC(hWnd);
    if (hdc) {
        dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hWnd, hdc);
    }
    int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
    int bw3 = bw1 * 3;

    int cxCrop = -rcFrame.left - bw3;
    int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - bw3;
    int cyCropBottom = rcFrame.bottom - bw3;
    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;
    HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, (rc.right - rc.left) - cxCrop, (rc.bottom - rc.top) - cyCropBottom);
    SetWindowRgn(hWnd, hRgn, FALSE);
}

void DrawScaledClientEdge(HDC hdc, RECT rc) {
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
    int bw2 = bw1 * 2;

    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH hDarkGray = CreateSolidBrush(RGB(128, 128, 128));
    HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH hLightGray = CreateSolidBrush(RGB(223, 223, 223));

    RECT rcTop = { rc.left, rc.top, rc.right, rc.top + bw1 }; FillRect(hdc, &rcTop, hDarkGray);
    RECT rcLeft = { rc.left, rc.top, rc.left + bw1, rc.bottom }; FillRect(hdc, &rcLeft, hDarkGray);
    RECT rcBottom = { rc.left, rc.bottom - bw1, rc.right, rc.bottom }; FillRect(hdc, &rcBottom, hWhite);
    RECT rcRight = { rc.right - bw1, rc.top, rc.right, rc.bottom }; FillRect(hdc, &rcRight, hWhite);

    RECT rcTop2 = { rc.left + bw1, rc.top + bw1, rc.right - bw1, rc.top + bw2 }; FillRect(hdc, &rcTop2, hBlack);
    RECT rcLeft2 = { rc.left + bw1, rc.top + bw1, rc.left + bw2, rc.bottom - bw1 }; FillRect(hdc, &rcLeft2, hBlack);
    RECT rcBottom2 = { rc.left + bw1, rc.bottom - bw2, rc.right - bw1, rc.bottom - bw1 }; FillRect(hdc, &rcBottom2, hLightGray);
    RECT rcRight2 = { rc.right - bw2, rc.top + bw1, rc.right - bw1, rc.bottom - bw1 }; FillRect(hdc, &rcRight2, hLightGray);

    DeleteObject(hWhite); DeleteObject(hDarkGray); DeleteObject(hBlack); DeleteObject(hLightGray);
}

void DrawScaledButtonBorder(HDC hdc, RECT rc, UINT edge) {
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
    int bw2 = bw1 * 2;

    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH hDarkGray = CreateSolidBrush(RGB(128, 128, 128));
    HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH hLightGray = CreateSolidBrush(RGB(223, 223, 223));

    RECT r = rc;
    if (edge == EDGE_RAISED) {
        RECT rcTop = { r.left, r.top, r.right, r.top + bw1 }; FillRect(hdc, &rcTop, hWhite);
        RECT rcLeft = { r.left, r.top, r.left + bw1, r.bottom }; FillRect(hdc, &rcLeft, hWhite);
        RECT rcBottom = { r.left, r.bottom - bw1, r.right, r.bottom }; FillRect(hdc, &rcBottom, hBlack);
        RECT rcRight = { r.right - bw1, r.top, r.right, r.bottom }; FillRect(hdc, &rcRight, hBlack);
        
        RECT rcTop2 = { r.left + bw1, r.top + bw1, r.right - bw1, r.top + bw2 }; FillRect(hdc, &rcTop2, hLightGray);
        RECT rcLeft2 = { r.left + bw1, r.top + bw1, r.left + bw2, r.bottom - bw1 }; FillRect(hdc, &rcLeft2, hLightGray);
        RECT rcBottom2 = { r.left + bw1, r.bottom - bw2, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcBottom2, hDarkGray);
        RECT rcRight2 = { r.right - bw2, r.top + bw1, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcRight2, hDarkGray);
    } else {
        RECT rcTop = { r.left, r.top, r.right, r.top + bw1 }; FillRect(hdc, &rcTop, hBlack);
        RECT rcLeft = { r.left, r.top, r.left + bw1, r.bottom }; FillRect(hdc, &rcLeft, hBlack);
        RECT rcBottom = { r.left, r.bottom - bw1, r.right, r.bottom }; FillRect(hdc, &rcBottom, hWhite);
        RECT rcRight = { r.right - bw1, r.top, r.right, r.bottom }; FillRect(hdc, &rcRight, hWhite);

        RECT rcTop2 = { r.left + bw1, r.top + bw1, r.right - bw1, r.top + bw2 }; FillRect(hdc, &rcTop2, hDarkGray);
        RECT rcLeft2 = { r.left + bw1, r.top + bw1, r.left + bw2, r.bottom - bw1 }; FillRect(hdc, &rcLeft2, hDarkGray);
        RECT rcBottom2 = { r.left + bw1, r.bottom - bw2, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcBottom2, hLightGray);
        RECT rcRight2 = { r.right - bw2, r.top + bw1, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcRight2, hLightGray);
    }
    DeleteObject(hWhite); DeleteObject(hDarkGray); DeleteObject(hBlack); DeleteObject(hLightGray);
}

LRESULT CALLBACK ClientEdgeSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_NCCALCSIZE) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        if (wParam == TRUE && (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)) {
            NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
            HDC hdc = GetWindowDC(hWnd);
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hWnd, hdc);
            int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
            int offset = bw - GetSystemMetrics(SM_CXEDGE);
            if (offset > 0) {
                params->rgrc[0].left += offset;
                params->rgrc[0].top += offset;
                params->rgrc[0].right -= offset;
                params->rgrc[0].bottom -= offset;
            }
        }
        return res;
    } else if (msg == WM_NCPAINT) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        if (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) {
            HDC hdc = GetWindowDC(hWnd);
            RECT rcWin; GetWindowRect(hWnd, &rcWin);
            RECT rc = {0, 0, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top};
            DrawScaledClientEdge(hdc, rc);
            ReleaseDC(hWnd, hdc);
        }
        return res;
    } else if (msg == WM_PAINT && uIdSubclass == 999) { // 999 means it's a combobox or similar without NC area
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        HDC hdc = GetWindowDC(hWnd);
        RECT rcWin; GetWindowRect(hWnd, &rcWin);
        RECT rc = {0, 0, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top};
        DrawScaledClientEdge(hdc, rc);
        ReleaseDC(hWnd, hdc);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

bool HandleClassicStyle(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
extern LRESULT CALLBACK ClassicButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

BOOL CALLBACK EnumChildProcColorDlg(HWND hwndChild, LPARAM lParam) {
    WCHAR className[256];
    GetClassNameW(hwndChild, className, 256);
    if (_wcsicmp(className, L"Button") == 0) {
        DWORD style = GetWindowLong(hwndChild, GWL_STYLE) & BS_TYPEMASK;
        if (style == BS_PUSHBUTTON || style == BS_DEFPUSHBUTTON) {
            SetWindowSubclass(hwndChild, ClassicButtonProc, 0, 0);
        }
    }
    return TRUE;
}

LRESULT CALLBACK ColorDlgSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    LRESULT classicResult = 0;
    if (HandleClassicStyle(hWnd, msg, wParam, lParam, &classicResult)) {
        return classicResult;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

UINT_PTR CALLBACK ColorHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    if (uiMsg == WM_INITDIALOG) {
        SetWindowSubclass(hdlg, ColorDlgSubclassProc, 0, 0);
        ApplyClassicStyle(hdlg);
        EnumChildWindows(hdlg, EnumChildProcColorDlg, 0);
        return TRUE;
    }
    return 0;
}

bool HandleClassicStyle(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
    switch (msg) {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdcStatic = (HDC)wParam;
        int id = GetDlgCtrlID((HWND)lParam);
        if (id == 1054) {
            SetBkColor(hdcStatic, RGB(192, 192, 192));
            static HBRUSH hbrHeader = CreateSolidBrush(RGB(192, 192, 192));
            *pResult = (LRESULT)hbrHeader;
            return true;
        }
        SetBkColor(hdcStatic, RGB(192, 192, 192));
        static HBRUSH hbrBkgnd = CreateSolidBrush(RGB(192, 192, 192));
        *pResult = (LRESULT)hbrBkgnd;
        return true;
    }
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* pwp = (WINDOWPOS*)lParam;
        int primaryBtn = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
        if ((GetAsyncKeyState(primaryBtn) >= 0) && !(pwp->flags & SWP_NOSIZE) && !(pwp->flags & SWP_NOMOVE)) {
            RECT rcNew = { pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy };
            HMONITOR hMonitor = MonitorFromRect(&rcNew, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            if (GetMonitorInfo(hMonitor, &mi)) {
                bool isLeftEdge = (pwp->x == mi.rcWork.left);
                bool isRightEdge = (pwp->x + pwp->cx == mi.rcWork.right);
                bool isTopEdge = (pwp->y == mi.rcWork.top);
                bool isBottomEdge = (pwp->y + pwp->cy == mi.rcWork.bottom);

                if (isLeftEdge || isRightEdge || isTopEdge || isBottomEdge) {
                    RECT rcFrame = { 0, 0, 0, 0 };
                    AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
                    int cxCrop = -rcFrame.left - 3;
                    int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - 3;
                    int cyCropBottom = rcFrame.bottom - 3;
                    if (cxCrop < 0) cxCrop = 0;
                    if (cyCropTop < 0) cyCropTop = 0;
                    if (cyCropBottom < 0) cyCropBottom = 0;

                    pwp->x -= cxCrop;
                    pwp->y -= cyCropTop;
                    pwp->cx += cxCrop * 2;
                    pwp->cy += cyCropTop + cyCropBottom;
                }
            }
        }
        return false;
    }
    case WM_NCLBUTTONDOWN: {
        int hit = (int)wParam;
        if (hit == HTCLOSE || hit == HTMINBUTTON || hit == HTMAXBUTTON) {
            extern HWND g_capturedWnd;
            extern int g_pressedBtn;
            extern int g_hoverBtn;
            
            SetCapture(hWnd);
            g_capturedWnd = hWnd;
            g_pressedBtn = hit;
            g_hoverBtn = hit;
            SendMessage(hWnd, WM_NCPAINT, 1, 0);
            *pResult = 0;
            return true; // Handled
        }
        return false;
    }
    case WM_MOUSEMOVE: {
        extern HWND g_capturedWnd;
        extern int g_pressedBtn;
        extern int g_hoverBtn;
        if (g_capturedWnd == hWnd && g_pressedBtn != 0) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(hWnd, &pt);
            LRESULT hit = SendMessage(hWnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
            int newHover = (hit == g_pressedBtn) ? g_pressedBtn : 0;
            if (newHover != g_hoverBtn) {
                g_hoverBtn = newHover;
                SendMessage(hWnd, WM_NCPAINT, 1, 0);
            }
        }
        return false;
    }
    case WM_LBUTTONUP: {
        extern HWND g_capturedWnd;
        extern int g_pressedBtn;
        extern int g_hoverBtn;
        if (g_capturedWnd == hWnd && g_pressedBtn != 0) {
            ReleaseCapture();
            int btn = g_pressedBtn;
            int hover = g_hoverBtn;
            g_capturedWnd = NULL;
            g_pressedBtn = 0;
            g_hoverBtn = 0;
            SendMessage(hWnd, WM_NCPAINT, 1, 0);
            
            if (hover == btn) {
                if (btn == HTCLOSE) PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                if (btn == HTMINBUTTON) PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                if (btn == HTMAXBUTTON) PostMessage(hWnd, WM_SYSCOMMAND, IsZoomed(hWnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
            }
        }
        return false;
    }
    case WM_NCMOUSEMOVE:
    case WM_NCMOUSELEAVE:
        return true; // Prevent hover redraw from DefWindowProc
    case WM_WINDOWPOSCHANGED: {
        *pResult = DefWindowProc(hWnd, msg, wParam, lParam);
        WINDOWPOS* pwp = (WINDOWPOS*)lParam;
        if (!(pwp->flags & SWP_NOSIZE)) {
            int dpiY = 96;
            HDC hdc = GetDC(hWnd);
            if (hdc) {
                dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(hWnd, hdc);
            }
            int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
            int bw3 = bw1 * 3;

            RECT rcFrame = { 0, 0, 0, 0 };
            AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
            int cxCrop = -rcFrame.left - bw3;
            int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - bw3;
            int cyCropBottom = rcFrame.bottom - bw3;
            if (cxCrop < 0) cxCrop = 0;
            if (cyCropTop < 0) cyCropTop = 0;
            if (cyCropBottom < 0) cyCropBottom = 0;
            
            HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, pwp->cx - cxCrop, pwp->cy - cyCropBottom);
            SetWindowRgn(hWnd, hRgn, TRUE);
        }
        return true;
    }
    case WM_NCHITTEST: {
        POINT pt;
        pt.x = (int)(short)LOWORD(lParam);
        pt.y = (int)(short)HIWORD(lParam);
        RECT rc;
        GetWindowRect(hWnd, &rc);
        
        int grab = 4;
        
        bool bLeft = (pt.x >= rc.left && pt.x < rc.left + grab);
        bool bRight = (pt.x < rc.right && pt.x >= rc.right - grab);
        bool bTop = (pt.y >= rc.top && pt.y < rc.top + grab);
        bool bBottom = (pt.y < rc.bottom && pt.y >= rc.bottom - grab);
        
        if (bTop && bLeft) { *pResult = HTTOPLEFT; return true; }
        if (bTop && bRight) { *pResult = HTTOPRIGHT; return true; }
        if (bBottom && bLeft) { *pResult = HTBOTTOMLEFT; return true; }
        if (bBottom && bRight) { *pResult = HTBOTTOMRIGHT; return true; }
        if (bLeft) { *pResult = HTLEFT; return true; }
        if (bRight) { *pResult = HTRIGHT; return true; }
        if (bTop) { *pResult = HTTOP; return true; }
        if (bBottom) { *pResult = HTBOTTOM; return true; }

        int dpiY = 96;
        HDC hdc = GetDC(hWnd);
        if (hdc) {
            dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hWnd, hdc);
        }
        int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
        int bw3 = bw1 * 3;

        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
        int cxCrop = -rcFrame.left - bw3;
        int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - bw3;
        if (cxCrop < 0) cxCrop = 0;
        if (cyCropTop < 0) cyCropTop = 0;

        int visLeft = cxCrop;
        int visRight = (rc.right - rc.left) - cxCrop;
        int visTop = cyCropTop;
        int captionHeight = GetSystemMetrics(SM_CYCAPTION);
        RECT rcCaption = { visLeft + 3, visTop + 3, visRight - 3, visTop + 3 + captionHeight };
        if (rcCaption.bottom > -rcFrame.top) rcCaption.bottom = -rcFrame.top;

        POINT wpt = { pt.x - rc.left, pt.y - rc.top };
        if (PtInRect(&rcCaption, wpt)) {
            int margin2 = Scale(2);
            int margin4 = Scale(4);
            int btnHeight = GetSystemMetrics(SM_CYSIZE) - margin4;
            if (btnHeight > captionHeight - margin4) btnHeight = captionHeight - margin4;
            int btnWidth = btnHeight + margin2;
            
            int btnTop = rcCaption.top + margin2;
            int btnRight = rcCaption.right - margin2;
            
            RECT rcClose = { btnRight - btnWidth, btnTop, btnRight, btnTop + btnHeight };
            RECT rcMax = { rcClose.left - margin2 - btnWidth, btnTop, rcClose.left - margin2, btnTop + btnHeight };
            RECT rcMin = { rcMax.left - btnWidth, btnTop, rcMax.left, btnTop + btnHeight };
            
            if (PtInRect(&rcClose, wpt)) { *pResult = HTCLOSE; return true; }
            if (GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX) {
                if (PtInRect(&rcMax, wpt)) { *pResult = HTMAXBUTTON; return true; }
                if (PtInRect(&rcMin, wpt)) { *pResult = HTMINBUTTON; return true; }
            }
            
            HICON hIcon = NULL;
            if (!(GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)) {
                hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
                if (!hIcon) hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
                if (!hIcon && (GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU)) hIcon = LoadIcon(NULL, IDI_APPLICATION);
            }
            if (hIcon) {
                int iconWidth = GetSystemMetrics(SM_CXSMICON);
                RECT rcIcon = { rcCaption.left, rcCaption.top, rcCaption.left + 2 + iconWidth + 4, rcCaption.bottom };
                if (PtInRect(&rcIcon, wpt)) { *pResult = HTSYSMENU; return true; }
            }

            *pResult = HTCAPTION;
            return true;
        }

        return false;
    }
    case WM_SETTEXT: {
        DWORD style = GetWindowLong(hWnd, GWL_STYLE);
        if (style & WS_VISIBLE) {
            SetWindowLong(hWnd, GWL_STYLE, style & ~WS_VISIBLE);
            *pResult = DefWindowProc(hWnd, msg, wParam, lParam);
            SetWindowLong(hWnd, GWL_STYLE, style);
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        } else {
            *pResult = DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return true;
    }
    case WM_NCPAINT:
    case WM_NCACTIVATE: {
        DWORD style = GetWindowLong(hWnd, GWL_STYLE);
        if (style & WS_VISIBLE) SetWindowLong(hWnd, GWL_STYLE, style & ~WS_VISIBLE);
        *pResult = DefWindowProc(hWnd, msg, wParam, lParam);
        if (style & WS_VISIBLE) SetWindowLong(hWnd, GWL_STYLE, style);
        
        if (!IsIconic(hWnd)) {
            HDC hdc = GetWindowDC(hWnd);
            if (hdc) {
                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                int width = rcWindow.right - rcWindow.left;
                int height = rcWindow.bottom - rcWindow.top;

                RECT rcFrame = { 0, 0, 0, 0 };
                AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));

                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
                int bw2 = bw1 * 2;
                int bw3 = bw1 * 3;

                int cxCrop = -rcFrame.left - bw3;
                int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - bw3;
                int cyCropBottom = rcFrame.bottom - bw3;
                if (cxCrop < 0) cxCrop = 0;
                if (cyCropTop < 0) cyCropTop = 0;
                if (cyCropBottom < 0) cyCropBottom = 0;

                int visLeft = cxCrop;
                int visTop = cyCropTop;
                int visRight = width - cxCrop;
                int visBottom = height - cyCropBottom;

                HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
                HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
                HBRUSH hLightGray = CreateSolidBrush(RGB(223, 223, 223));
                HBRUSH hDarkGray = CreateSolidBrush(RGB(128, 128, 128));
                HBRUSH hFaceGray = CreateSolidBrush(RGB(192, 199, 200));

                RECT rcWhiteTop = { visLeft, visTop, visRight - bw1, visTop + bw1 };
                FillRect(hdc, &rcWhiteTop, hWhite);
                RECT rcWhiteLeft = { visLeft, visTop, visLeft + bw1, visBottom - bw1 };
                FillRect(hdc, &rcWhiteLeft, hWhite);

                RECT rcBlackBottom = { visLeft, visBottom - bw1, visRight, visBottom };
                FillRect(hdc, &rcBlackBottom, hBlack);
                RECT rcBlackRight = { visRight - bw1, visTop, visRight, visBottom };
                FillRect(hdc, &rcBlackRight, hBlack);

                RECT rcLightTop = { visLeft + bw1, visTop + bw1, visRight - bw2, visTop + bw2 };
                FillRect(hdc, &rcLightTop, hLightGray);
                RECT rcLightLeft = { visLeft + bw1, visTop + bw1, visLeft + bw2, visBottom - bw2 };
                FillRect(hdc, &rcLightLeft, hLightGray);

                RECT rcDarkBottom = { visLeft + bw1, visBottom - bw2, visRight - bw1, visBottom - bw1 };
                FillRect(hdc, &rcDarkBottom, hDarkGray);
                RECT rcDarkRight = { visRight - bw2, visTop + bw1, visRight - bw1, visBottom - bw1 };
                FillRect(hdc, &rcDarkRight, hDarkGray);

                RECT rcFaceTop = { visLeft + bw2, visTop + bw2, visRight - bw2, visTop + bw3 };
                FillRect(hdc, &rcFaceTop, hFaceGray);
                RECT rcFaceBottom = { visLeft + bw2, visBottom - bw3, visRight - bw2, visBottom - bw2 };
                FillRect(hdc, &rcFaceBottom, hFaceGray);
                RECT rcFaceLeft = { visLeft + bw2, visTop + bw2, visLeft + bw3, visBottom - bw2 };
                FillRect(hdc, &rcFaceLeft, hFaceGray);
                RECT rcFaceRight = { visRight - bw3, visTop + bw2, visRight - bw2, visBottom - bw2 };
                FillRect(hdc, &rcFaceRight, hFaceGray);

                DeleteObject(hBlack);
                DeleteObject(hWhite);
                DeleteObject(hLightGray);
                DeleteObject(hDarkGray);
                DeleteObject(hFaceGray);

                int captionHeight = GetSystemMetrics(SM_CYCAPTION);
                RECT rcCaption = { visLeft + bw3, visTop + bw3, visRight - bw3, visTop + bw3 + captionHeight };
                if (rcCaption.bottom > -rcFrame.top) rcCaption.bottom = -rcFrame.top;
                
                TITLEBARINFOEX tbiex = { 0 };
                tbiex.cbSize = sizeof(TITLEBARINFOEX);
                SendMessage(hWnd, WM_GETTITLEBARINFOEX, 0, (LPARAM)&tbiex);

                RECT rcWin;
                GetWindowRect(hWnd, &rcWin);

                // We no longer exclude the clip rect because we will draw the buttons manually

                bool bActive;
                if (msg == WM_NCACTIVATE) {
                    bActive = (wParam != FALSE);
                } else {
                    bActive = (GetActiveWindow() == hWnd);
                }
                
                HBRUSH hCaptionBrush = CreateSolidBrush(bActive ? RGB(0, 0, 128) : RGB(128, 128, 128));
                FillRect(hdc, &rcCaption, hCaptionBrush);
                DeleteObject(hCaptionBrush);

                int margin2 = Scale(2);
                int margin4 = Scale(4);
                int btnHeight = GetSystemMetrics(SM_CYSIZE) - margin4;
                captionHeight = rcCaption.bottom - rcCaption.top;
                if (btnHeight > captionHeight - margin4) btnHeight = captionHeight - margin4;
                int btnWidth = btnHeight + margin2;

                int btnTop = rcCaption.top + margin2;
                int btnRight = rcCaption.right - margin2;

                RECT rcClose = { btnRight - btnWidth, btnTop, btnRight, btnTop + btnHeight };
                RECT rcMax = { rcClose.left - margin2 - btnWidth, btnTop, rcClose.left - margin2, btnTop + btnHeight };
                RECT rcMin = { rcMax.left - btnWidth, btnTop, rcMax.left, btnTop + btnHeight };

                extern HWND g_capturedWnd;
                extern int g_hoverBtn;

                UINT closeState = DFCS_CAPTIONCLOSE;
                UINT maxState = IsZoomed(hWnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX;
                UINT minState = DFCS_CAPTIONMIN;

                if (g_capturedWnd == hWnd && g_hoverBtn == HTCLOSE) closeState |= DFCS_PUSHED;
                if (g_capturedWnd == hWnd && g_hoverBtn == HTMAXBUTTON) maxState |= DFCS_PUSHED;
                if (g_capturedWnd == hWnd && g_hoverBtn == HTMINBUTTON) minState |= DFCS_PUSHED;

                auto drawCaptionBtn = [&](RECT r, int type, bool pushed) {
                    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                    int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
                    int bw2 = bw1 * 2;

                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &r, hBg);
                    DeleteObject(hBg);

                    // Manually draw button edge
                    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
                    HBRUSH hDarkGray = CreateSolidBrush(RGB(128, 128, 128));
                    HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
                    HBRUSH hLightGray = CreateSolidBrush(RGB(223, 223, 223));

                    if (!pushed) {
                        RECT rcTop = { r.left, r.top, r.right, r.top + bw1 }; FillRect(hdc, &rcTop, hWhite);
                        RECT rcLeft = { r.left, r.top, r.left + bw1, r.bottom }; FillRect(hdc, &rcLeft, hWhite);
                        RECT rcBottom = { r.left, r.bottom - bw1, r.right, r.bottom }; FillRect(hdc, &rcBottom, hBlack);
                        RECT rcRight = { r.right - bw1, r.top, r.right, r.bottom }; FillRect(hdc, &rcRight, hBlack);
                        
                        RECT rcTop2 = { r.left + bw1, r.top + bw1, r.right - bw1, r.top + bw2 }; FillRect(hdc, &rcTop2, hLightGray);
                        RECT rcLeft2 = { r.left + bw1, r.top + bw1, r.left + bw2, r.bottom - bw1 }; FillRect(hdc, &rcLeft2, hLightGray);
                        RECT rcBottom2 = { r.left + bw1, r.bottom - bw2, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcBottom2, hDarkGray);
                        RECT rcRight2 = { r.right - bw2, r.top + bw1, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcRight2, hDarkGray);
                    } else {
                        RECT rcTop = { r.left, r.top, r.right, r.top + bw1 }; FillRect(hdc, &rcTop, hBlack);
                        RECT rcLeft = { r.left, r.top, r.left + bw1, r.bottom }; FillRect(hdc, &rcLeft, hBlack);
                        RECT rcBottom = { r.left, r.bottom - bw1, r.right, r.bottom }; FillRect(hdc, &rcBottom, hWhite);
                        RECT rcRight = { r.right - bw1, r.top, r.right, r.bottom }; FillRect(hdc, &rcRight, hWhite);

                        RECT rcTop2 = { r.left + bw1, r.top + bw1, r.right - bw1, r.top + bw2 }; FillRect(hdc, &rcTop2, hDarkGray);
                        RECT rcLeft2 = { r.left + bw1, r.top + bw1, r.left + bw2, r.bottom - bw1 }; FillRect(hdc, &rcLeft2, hDarkGray);
                        RECT rcBottom2 = { r.left + bw1, r.bottom - bw2, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcBottom2, hLightGray);
                        RECT rcRight2 = { r.right - bw2, r.top + bw1, r.right - bw1, r.bottom - bw1 }; FillRect(hdc, &rcRight2, hLightGray);
                    }

                    DeleteObject(hWhite);
                    DeleteObject(hDarkGray);
                    DeleteObject(hBlack);
                    DeleteObject(hLightGray);
                    
                    int cx = r.left + (r.right - r.left) / 2;
                    int cy = r.top + (r.bottom - r.top) / 2;
                    if (pushed) { cx += bw1; cy += bw1; }
                    
                    int s = bw1;
                    HPEN hBlackPen = CreatePen(PS_SOLID, s, RGB(0,0,0));
                    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0,0,0));
                    HPEN hOldP = (HPEN)SelectObject(hdc, hBlackPen);
                    HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBlackBrush);
                    
                    if (type == 0) { // close
                        int h = 3 * s;
                        int x0 = cx - 4 * s;
                        int y0 = cy - h;
                        int x1 = x0 + 6 * s;
                        int y1 = cy + h;

                        HPEN hThinPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
                        HPEN hOldP2 = (HPEN)SelectObject(hdc, hThinPen);

                        int thick = 2 * s;
                        for (int i = 0; i < thick; i++) {
                            MoveToEx(hdc, x0 + i, y0, NULL); LineTo(hdc, x1 + i + 1, y1 + 1);
                            MoveToEx(hdc, x1 + i, y0, NULL); LineTo(hdc, x0 + i - 1, y1 + 1);
                        }

                        SelectObject(hdc, hOldP2);
                        DeleteObject(hThinPen);
                    } else if (type == 1) { // max
                        SelectObject(hdc, GetStockObject(NULL_BRUSH));
                        Rectangle(hdc, cx - 4*s, cy - 4*s, cx + 5*s, cy + 5*s);
                        MoveToEx(hdc, cx - 4*s, cy - 3*s, NULL); LineTo(hdc, cx + 5*s, cy - 3*s);
                    } else if (type == 2) { // min
                        SelectObject(hdc, GetStockObject(BLACK_BRUSH));
                        Rectangle(hdc, cx - 3*s, cy + 3*s, cx + 3*s, cy + 5*s);
                    } else if (type == 3) { // restore
                        SelectObject(hdc, GetStockObject(NULL_BRUSH));
                        Rectangle(hdc, cx - 1*s, cy - 4*s, cx + 5*s, cy + 2*s);
                        MoveToEx(hdc, cx - 1*s, cy - 3*s, NULL); LineTo(hdc, cx + 5*s, cy - 3*s);
                        HBRUSH hFace = CreateSolidBrush(RGB(192, 192, 192));
                        RECT rcFace = {cx - 4*s, cy - 1*s, cx + 3*s, cy + 5*s};
                        FillRect(hdc, &rcFace, hFace);
                        DeleteObject(hFace);
                        Rectangle(hdc, cx - 4*s, cy - 1*s, cx + 2*s, cy + 5*s);
                        MoveToEx(hdc, cx - 4*s, cy, NULL); LineTo(hdc, cx + 2*s, cy);
                    }
                    
                    SelectObject(hdc, hOldP);
                    SelectObject(hdc, hOldB);
                    DeleteObject(hBlackPen);
                    DeleteObject(hBlackBrush);
                };

                drawCaptionBtn(rcClose, 0, (closeState & DFCS_PUSHED) != 0);
                if (GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX) {
                    drawCaptionBtn(rcMax, IsZoomed(hWnd) ? 3 : 1, (maxState & DFCS_PUSHED) != 0);
                    drawCaptionBtn(rcMin, 2, (minState & DFCS_PUSHED) != 0);
                }

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, bActive ? RGB(255, 255, 255) : RGB(192, 192, 192));
                
                int fontSize = -MulDiv(12, dpiY, 96);
                HFONT hFont = CreateFont(
                    fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Gothic"
                );
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                WCHAR szTitle[256];
                GetWindowText(hWnd, szTitle, 256);
                
                int iconWidth = GetSystemMetrics(SM_CXSMICON);
                RECT rcText = rcCaption;
                
                HICON hIcon = NULL;
                if (!(GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)) {
                    hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
                    if (!hIcon) hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
                    if (!hIcon && (GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU)) hIcon = LoadIcon(NULL, IDI_APPLICATION);
                }
                
                if (hIcon) {
                    rcText.left += iconWidth + 6;
                    int iconY = rcCaption.top + (rcCaption.bottom - rcCaption.top - GetSystemMetrics(SM_CYSMICON)) / 2;
                    DrawIconEx(hdc, rcCaption.left + 2, iconY, hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
                } else {
                    rcText.left += 2;
                }
                
                if (GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX) {
                    rcText.right = rcMin.left - 4;
                } else {
                    rcText.right = rcClose.left - 4;
                }
                
                DrawText(hdc, szTitle, lstrlen(szTitle), &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
                ReleaseDC(hWnd, hdc);
            }
        }
        return true;
    }
    }
    return false;
}

// Dialogs

LRESULT CALLBACK EditSelectionColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

std::wstring g_changeNameResult;
bool g_changeNameOK = false;

void DrawColorButton(LPDRAWITEMSTRUCT lpdis, COLORREF colorToDraw) {
    HDC hdc = lpdis->hDC;
    RECT rc = lpdis->rcItem;
    HBRUSH hFaceBrush = CreateSolidBrush(RGB(192, 192, 192));
    FillRect(hdc, &rc, hFaceBrush);
    DeleteObject(hFaceBrush);
    UINT edge = (lpdis->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_RAISED;
    DrawScaledButtonBorder(hdc, rc, edge);
    if (lpdis->itemState & ODS_FOCUS) {
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
        RECT rcFocus = rc; InflateRect(&rcFocus, -bw*2, -bw*2); DrawFocusRect(hdc, &rcFocus);
    }
    
    GdiFlush(); // Ensure GDI background is drawn before GDI+ draws on top

    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        
        int cx = rc.left + (rc.right - rc.left) / 2 - Scale(1);
        int cy = rc.top + (rc.bottom - rc.top) / 2 - Scale(1);
        
        int size = Scale(13);
        
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
        
        Gdiplus::SolidBrush colorBrush(Gdiplus::Color(255, GetRValue(colorToDraw), GetGValue(colorToDraw), GetBValue(colorToDraw)));
        Gdiplus::Pen blackPen(Gdiplus::Color(255, 0, 0, 0), (Gdiplus::REAL)(Scale(1)));

        graphics.FillRectangle(&colorBrush, cx - size / 2, cy - size / 2, size, size);
        graphics.DrawRectangle(&blackPen, cx - size / 2, cy - size / 2, size, size);
    }
}

LRESULT CALLBACK ChangeNameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT classicResult = 0;
    if (HandleClassicStyle(hWnd, msg, wParam, lParam, &classicResult)) return classicResult;

    static HWND hEdit, hBtnOK, hBtnCancel;
    switch (msg) {
    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260);
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            margin, y, totalWidth - margin * 2, inputHeight, hWnd, 0, GetModuleHandle(0), 0);
        
        y += inputHeight + margin;
        
        int cancelX = totalWidth - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;
        
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            okX, y, btnWidth, btnHeight, hWnd, (HMENU)1, GetModuleHandle(0), 0);
        hBtnCancel = CreateWindow(L"BUTTON", L"キャンセル", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            cancelX, y, btnWidth, btnHeight, hWnd, (HMENU)2, GetModuleHandle(0), 0);
            
        static HWND hBtnColorLocal;
        hBtnColorLocal = CreateWindow(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            margin, y, Scale(32), btnHeight, hWnd, (HMENU)1002, GetModuleHandle(0), 0);
            
        SetWindowSubclass(hBtnOK, ClassicButtonProc, 0, 0);
        SetWindowSubclass(hBtnCancel, ClassicButtonProc, 0, 0);
        SetWindowSubclass(hEdit, EditSelectionColorProc, 0, 0);
        SetWindowSubclass(hEdit, ClientEdgeSubclassProc, 0, 0);
        SetWindowTheme(hEdit, L" ", L" ");
        SetWindowPos(hEdit, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        
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
            std::wstring oldBaseName = g_username;
            size_t hashPos = oldBaseName.find(L'#');
            std::wstring oldTag = L"";
            if (hashPos != std::wstring::npos) {
                oldTag = oldBaseName.substr(hashPos);
                oldBaseName = oldBaseName.substr(0, hashPos);
            }
            if (newName == oldBaseName) {
                newName += oldTag;
            } else if (!newName.empty() && newName.find(L'#') == std::wstring::npos) {
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
        } else if (LOWORD(wParam) == 1002 && HIWORD(wParam) == BN_CLICKED) {
            CHOOSECOLOR cc;
            static COLORREF acrCustClr[16]; 
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hWnd;
            cc.lpCustColors = (LPDWORD)acrCustClr;
            cc.rgbResult = g_nameColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
            cc.lpfnHook = ColorHookProc;
            if (ChooseColor(&cc)) {
                g_nameColor = cc.rgbResult;
                InvalidateRect(GetDlgItem(hWnd, 1002), NULL, FALSE);
                if (g_hLog) InvalidateRect(g_hLog, NULL, FALSE);
                if (g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                SaveSettings();
            }
        }
        return 0;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        if (lpdis->CtlID == 1002) {
            DrawColorButton(lpdis, g_nameColor);
            return TRUE;
        }
        break;
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
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(192, 192, 192));
    RegisterClass(&wc);

    g_changeNameOK = false;
    g_changeNameResult = L"";

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);

    int margin = Scale(5);
    int totalWidth = margin * 2 + Scale(260);
    int totalHeight = margin * 3 + Scale(24) + Scale(24);
    
    RECT rcDlg = { 0, 0, totalWidth, totalHeight };
    AdjustWindowRectEx(&rcDlg, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int winW = rcDlg.right - rcDlg.left;
    int winH = rcDlg.bottom - rcDlg.top;

    HWND hWnd = CreateWindowEx(WS_EX_DLGMODALFRAME, L"ChangeNameClass", GetString(IDS_CHANGE_NAME_TITLE).c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcParent.left + (rcParent.right - rcParent.left - winW) / 2, 
        rcParent.top + (rcParent.bottom - rcParent.top - winH) / 2, 
        winW, winH, hParent, 0, GetModuleHandle(0), 0);
    
    ApplyClassicStyle(hWnd);
    
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

static HWND g_sbPressedWnd = NULL;
static int g_sbPressedHit = 0;

LRESULT CALLBACK ScrollbarColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_NCPAINT) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        if ((GetWindowLong(hWnd, GWL_STYLE) & WS_VSCROLL) == 0) return res;
        
        HDC hdc = GetWindowDC(hWnd);
        RECT rcWin;
        GetWindowRect(hWnd, &rcWin);
        int width = rcWin.right - rcWin.left;
        int height = rcWin.bottom - rcWin.top;
        
        int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
        int border = GetSystemMetrics(SM_CXEDGE);
        if (width <= cxVScroll) {
            ReleaseDC(hWnd, hdc);
            return res;
        }

        SCROLLBARINFO sbi = { sizeof(sbi) };
        if (GetScrollBarInfo(hWnd, OBJID_VSCROLL, &sbi)) {
            int sbWidth = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
            int sbHeight = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
            
            RECT rcWin; GetWindowRect(hWnd, &rcWin);
            int offsetX = sbi.rcScrollBar.left - rcWin.left;
            int offsetY = sbi.rcScrollBar.top - rcWin.top;

            HBRUSH hFace = CreateSolidBrush(RGB(192, 192, 192));
            HPEN hBlack = (HPEN)GetStockObject(BLACK_PEN);

            WORD pat[8] = { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555 };
            HBITMAP hBmp = CreateBitmap(8, 8, 1, 1, pat);
            HBRUSH hTrackBrush = CreatePatternBrush(hBmp);

            auto drawArrow = [&](RECT rc, bool isUp, bool isPressed) {
                FillRect(hdc, &rc, hFace);
                DrawScaledButtonBorder(hdc, rc, isPressed ? EDGE_SUNKEN : EDGE_RAISED);
                
                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;

                int cx = rc.left + (rc.right - rc.left) / 2;
                int cy = rc.top + (rc.bottom - rc.top) / 2;
                if (isPressed) { cx += s; cy += s; }
                SelectObject(hdc, hBlack);
                HBRUSH hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBlackBrush);
                if (isUp) {
                    POINT pts[3] = { {cx, cy - 2*s}, {cx - 3*s, cy + 1*s}, {cx + 3*s, cy + 1*s} };
                    Polygon(hdc, pts, 3);
                } else {
                    POINT pts[3] = { {cx - 3*s, cy - 1*s}, {cx + 3*s, cy - 1*s}, {cx, cy + 2*s} };
                    Polygon(hdc, pts, 3);
                }
                SelectObject(hdc, hOldB);
            };

            RECT rcTop = { offsetX, offsetY, offsetX + sbWidth, offsetY + sbi.dxyLineButton };
            bool isTopPressed = (sbi.rgstate[1] & STATE_SYSTEM_PRESSED) != 0 || (g_sbPressedWnd == hWnd && g_sbPressedHit == 1);
            drawArrow(rcTop, true, isTopPressed);

            RECT rcBot = { offsetX, offsetY + sbHeight - sbi.dxyLineButton, offsetX + sbWidth, offsetY + sbHeight };
            bool isBotPressed = (sbi.rgstate[5] & STATE_SYSTEM_PRESSED) != 0 || (g_sbPressedWnd == hWnd && g_sbPressedHit == 2);
            drawArrow(rcBot, false, isBotPressed);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(192, 192, 192));

            if (sbi.xyThumbBottom > sbi.xyThumbTop && !(sbi.rgstate[0] & STATE_SYSTEM_UNAVAILABLE)) {
                RECT rcPgUp = { offsetX, rcTop.bottom, offsetX + sbWidth, offsetY + sbi.xyThumbTop };
                FillRect(hdc, &rcPgUp, hTrackBrush);

                RECT rcPgDn = { offsetX, offsetY + sbi.xyThumbBottom, offsetX + sbWidth, rcBot.top };
                FillRect(hdc, &rcPgDn, hTrackBrush);

                RECT rcThumb = { offsetX, offsetY + sbi.xyThumbTop, offsetX + sbWidth, offsetY + sbi.xyThumbBottom };
                FillRect(hdc, &rcThumb, hFace);
                DrawScaledButtonBorder(hdc, rcThumb, EDGE_RAISED);
            } else {
                RECT rcTrack = { offsetX, rcTop.bottom, offsetX + sbWidth, rcBot.top };
                FillRect(hdc, &rcTrack, hTrackBrush);
            }

            DeleteObject(hFace);
            DeleteObject(hTrackBrush);
            DeleteObject(hBmp);
            ReleaseDC(hWnd, hdc);
            return res;
        }

        ReleaseDC(hWnd, hdc);
        
        return res;
    }
    else if (msg == WM_NCMOUSEMOVE || msg == WM_NCMOUSELEAVE) {
        if (wParam == HTVSCROLL || wParam == HTHSCROLL) return 0;
    }
    else if (msg == WM_NCLBUTTONDBLCLK || msg == WM_NCLBUTTONDOWN) {
        if (wParam == HTVSCROLL) {
            SCROLLBARINFO sbi = { sizeof(sbi) };
            if (GetScrollBarInfo(hWnd, OBJID_VSCROLL, &sbi)) {
                POINT pt; GetCursorPos(&pt);
                RECT rcTop = { sbi.rcScrollBar.left, sbi.rcScrollBar.top, sbi.rcScrollBar.right, sbi.rcScrollBar.top + sbi.dxyLineButton };
                RECT rcBot = { sbi.rcScrollBar.left, sbi.rcScrollBar.bottom - sbi.dxyLineButton, sbi.rcScrollBar.right, sbi.rcScrollBar.bottom };
                RECT rcThumb = { sbi.rcScrollBar.left, sbi.rcScrollBar.top + sbi.xyThumbTop, sbi.rcScrollBar.right, sbi.rcScrollBar.top + sbi.xyThumbBottom };
                RECT rcPgUp = { sbi.rcScrollBar.left, rcTop.bottom, sbi.rcScrollBar.right, rcThumb.top };
                RECT rcPgDn = { sbi.rcScrollBar.left, rcThumb.bottom, sbi.rcScrollBar.right, rcBot.top };

                int hit = 0;
                if (PtInRect(&rcTop, pt)) hit = 1;
                else if (PtInRect(&rcBot, pt)) hit = 2;
                else if (PtInRect(&rcThumb, pt)) hit = 3;
                else if (PtInRect(&rcPgUp, pt)) hit = 4;
                else if (PtInRect(&rcPgDn, pt)) hit = 5;

                if (hit != 0) {
                    SetCapture(hWnd);
                    int startY = pt.y;
                    SCROLLINFO si = { sizeof(si), SIF_ALL };
                    GetScrollInfo(hWnd, SB_VERT, &si);
                    int startPos = si.nPos;

                    auto sendScroll = [&](int action) { SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(action, 0), 0); };
                    if (hit == 1) sendScroll(SB_LINEUP);
                    if (hit == 2) sendScroll(SB_LINEDOWN);
                    if (hit == 4) sendScroll(SB_PAGEUP);
                    if (hit == 5) sendScroll(SB_PAGEDOWN);

                    if (hit != 3) SetTimer(hWnd, 9998, 250, NULL);
                    
                    g_sbPressedWnd = hWnd;
                    g_sbPressedHit = hit;
                    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);

                    MSG msgLoop;
                    while (GetCapture() == hWnd && GetMessage(&msgLoop, NULL, 0, 0)) {
                        if (msgLoop.message == WM_LBUTTONUP) {
                            ReleaseCapture();
                            break;
                        } else if (msgLoop.message == WM_MOUSEMOVE) {
                            if (hit == 3) {
                                POINT scrPt; GetCursorPos(&scrPt);
                                int trackLen = (sbi.rcScrollBar.bottom - sbi.dxyLineButton) - (sbi.rcScrollBar.top + sbi.dxyLineButton);
                                int thumbLen = sbi.xyThumbBottom - sbi.xyThumbTop;
                                int scrollMax = si.nMax - max((int)si.nPage - 1, 0);
                                if (trackLen > thumbLen && scrollMax > 0) {
                                    int dy = scrPt.y - startY;
                                    int newPos = startPos + dy * scrollMax / (trackLen - thumbLen);
                                    if (newPos < si.nMin) newPos = si.nMin;
                                    if (newPos > scrollMax) newPos = scrollMax;
                                    SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, newPos), 0);
                                    SetScrollPos(hWnd, SB_VERT, newPos, TRUE);
                                    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                                }
                            } else if (hit == 1 || hit == 2) {
                                POINT scrPt; GetCursorPos(&scrPt);
                                bool isInside = (hit == 1 && PtInRect(&rcTop, scrPt)) || (hit == 2 && PtInRect(&rcBot, scrPt));
                                int newHit = isInside ? hit : 0;
                                if (g_sbPressedHit != newHit) {
                                    g_sbPressedHit = newHit;
                                    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                                }
                            }
                        } else if (msgLoop.message == WM_TIMER && msgLoop.wParam == 9998) {
                            SetTimer(hWnd, 9998, 50, NULL);
                            POINT scrPt; GetCursorPos(&scrPt);
                            if (hit == 1 && PtInRect(&rcTop, scrPt)) sendScroll(SB_LINEUP);
                            if (hit == 2 && PtInRect(&rcBot, scrPt)) sendScroll(SB_LINEDOWN);
                            if (hit == 4 && PtInRect(&rcPgUp, scrPt)) sendScroll(SB_PAGEUP);
                            if (hit == 5 && PtInRect(&rcPgDn, scrPt)) sendScroll(SB_PAGEDOWN);
                        } else {
                            TranslateMessage(&msgLoop);
                            DispatchMessage(&msgLoop);
                        }
                    }
                    g_sbPressedWnd = NULL;
                    g_sbPressedHit = 0;
                    if (hit != 3) KillTimer(hWnd, 9998);
                    sendScroll(SB_ENDSCROLL);
                    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                    return 0;
                }
            }
        }
    }
    else if (msg == WM_MOUSEWHEEL || msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_VSCROLL) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

std::wstring g_moveBoxResult;
bool g_moveBoxPrivate = false;
bool g_moveBoxOK = false;
bool g_ignoreEditChange = false;

LRESULT CALLBACK EditSelectionColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBitmap);
        
        SendMessage(hWnd, WM_PRINTCLIENT, (WPARAM)hMemDC, PRF_CLIENT | PRF_ERASEBKGND);
        
        COLORREF sysHighlight = GetSysColor(COLOR_HIGHLIGHT);
        COLORREF targetHighlight = RGB(0, 0, 128);
        if (sysHighlight != targetHighlight) {
            for (int y = 0; y < rc.bottom; y++) {
                for (int x = 0; x < rc.right; x++) {
                    if (GetPixel(hMemDC, x, y) == sysHighlight) {
                        SetPixel(hMemDC, x, y, targetHighlight);
                    }
                }
            }
        }
        
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);
        
        SelectObject(hMemDC, hOld);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        EndPaint(hWnd, &ps);
        return 0;
    }
    else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK || 
             msg == WM_MOUSEMOVE || msg == WM_KEYDOWN || msg == WM_KEYUP) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK MoveRoomProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT classicResult = 0;
    if (HandleClassicStyle(hWnd, msg, wParam, lParam, &classicResult)) return classicResult;

    static HWND hEdit, hList, hChk, hBtnOK, hBtnCancel;
    static std::vector<std::wstring> s_roomNames;
    switch (msg) {
    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260); // 270
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            margin, y, totalWidth - margin * 2, inputHeight, hWnd, (HMENU)101, GetModuleHandle(0), 0);
        y += inputHeight + margin;
        
        hList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED,
            margin, y, totalWidth - margin * 2, listHeight, hWnd, (HMENU)102, GetModuleHandle(0), 0);
        y += listHeight + margin;

        int cancelX = totalWidth - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;
        
        hChk = CreateWindow(L"BUTTON", GetString(IDS_PRIVATE).c_str(), 
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            margin, y, chkWidth, btnHeight, hWnd, (HMENU)103, GetModuleHandle(0), 0);
            
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            okX, y, btnWidth, btnHeight, hWnd, (HMENU)1, GetModuleHandle(0), 0);
            
        hBtnCancel = CreateWindow(L"BUTTON", L"キャンセル", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            cancelX, y, btnWidth, btnHeight, hWnd, (HMENU)2, GetModuleHandle(0), 0);
            
        SetWindowSubclass(hBtnOK, ClassicButtonProc, 0, 0);
        SetWindowSubclass(hBtnCancel, ClassicButtonProc, 0, 0);
        SetWindowSubclass(hEdit, EditSelectionColorProc, 0, 0);
        SetWindowSubclass(hEdit, ClientEdgeSubclassProc, 0, 0);
        SetWindowSubclass(hList, ScrollbarColorProc, 0, 0);
        SetWindowSubclass(hList, ClientEdgeSubclassProc, 0, 0);
        SetWindowTheme(hEdit, L" ", L" ");
        SetWindowTheme(hList, L" ", L" ");
        SetWindowPos(hEdit, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(hList, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        SendMessage(hList, LB_SETITEMHEIGHT, 0, Scale(16));
        
        s_roomNames.clear();
        for (const auto& r : g_publicRooms) {
            if (r.second <= 0) continue;
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

        g_hMoveRoomDlg = hWnd;
        json11::Json getRoomsMsg = json11::Json::object { {"type", "get_rooms"} };
        std::string str = getRoomsMsg.dump() + "\n";
        send(g_tcpSocket, str.c_str(), (int)str.length(), 0);

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
        } else if (id == 102 && code == LBN_DBLCLK) {
            SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(1, BN_CLICKED), (LPARAM)hBtnOK);
        } else if (id == 101 && code == EN_CHANGE) {
            if (g_ignoreEditChange) break;
            int len = GetWindowTextLength(hEdit);
            std::vector<WCHAR> buf(len + 1);
            GetWindowText(hEdit, buf.data(), len + 1);
            std::wstring search = buf.data();
            
            SendMessage(hList, LB_RESETCONTENT, 0, 0);
            s_roomNames.clear();
            for (const auto& r : g_publicRooms) {
                if (r.second <= 0) continue;
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
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
        if (pdis->CtlID == 102 && pdis->itemID != -1) {
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;
            bool isSelected = (pdis->itemState & ODS_SELECTED);
            
            if (isSelected) {
                HBRUSH hSel = CreateSolidBrush(RGB(0, 0, 128));
                FillRect(hdc, &rc, hSel);
                DeleteObject(hSel);
                SetTextColor(hdc, RGB(255, 255, 255));
            } else {
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
                SetTextColor(hdc, RGB(0, 0, 0));
            }
            SetBkMode(hdc, TRANSPARENT);
            
            WCHAR text[256];
            SendMessage(pdis->hwndItem, LB_GETTEXT, pdis->itemID, (LPARAM)text);
            rc.left += Scale(2);
            DrawTextW(hdc, text, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            
            if (pdis->itemState & ODS_FOCUS) {
                SetTextColor(hdc, isSelected ? RGB(255, 255, 255) : RGB(0, 0, 0));
                SetBkColor(hdc, isSelected ? RGB(0, 0, 128) : RGB(255, 255, 255));
                WORD pat[8] = { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555 };
                HBITMAP hBmp = CreateBitmap(8, 8, 1, 1, pat);
                HBRUSH hBrush = CreatePatternBrush(hBmp);
                FrameRect(hdc, &rc, hBrush);
                DeleteObject(hBrush);
                DeleteObject(hBmp);
            }
            return TRUE;
        }
        break;
    }
    case WM_CLOSE:
        g_moveBoxOK = false;
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY: {
        if (g_hMoveRoomDlg == hWnd) {
            g_hMoveRoomDlg = NULL;
            if (g_tcpSocket != INVALID_SOCKET) {
                json11::Json closeRoomsMsg = json11::Json::object { {"type", "close_rooms"} };
                std::string str = closeRoomsMsg.dump() + "\n";
                send(g_tcpSocket, str.c_str(), (int)str.length(), 0);
            }
        }
        return 0;
    }
    case WM_USER_ROOM_LIST_UPDATED: {
        HWND hEdit = GetDlgItem(hWnd, 101);
        HWND hList = GetDlgItem(hWnd, 102);
        int len = GetWindowTextLength(hEdit);
        std::vector<WCHAR> buf(len + 1);
        GetWindowText(hEdit, buf.data(), len + 1);
        std::wstring search = buf.data();
        
        SendMessage(hList, LB_RESETCONTENT, 0, 0);
        s_roomNames.clear();
        for (const auto& r : g_publicRooms) {
            if (r.second <= 0) continue;
            if (search.empty() || r.first.find(search) != std::wstring::npos) {
                std::wstring display = r.first + L" (" + std::to_wstring(r.second) + L")";
                int idx = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                s_roomNames.push_back(r.first);
                SendMessage(hList, LB_SETITEMDATA, idx, (LPARAM)(s_roomNames.size() - 1));
            }
        }
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ShowMoveRoomDialog(HWND hParent, std::wstring& outResult, bool& outPrivate) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MoveRoomProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = L"MoveRoomClass";
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(192, 192, 192));
    RegisterClass(&wc);

    g_moveBoxOK = false;
    g_moveBoxResult = L"";
    g_moveBoxPrivate = false;

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);

    int clientW = Scale(270);
    int clientH = Scale(168);
    RECT rcDlg = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&rcDlg, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int winW = rcDlg.right - rcDlg.left;
    int winH = rcDlg.bottom - rcDlg.top;

    HWND hWnd = CreateWindowEx(WS_EX_DLGMODALFRAME, L"MoveRoomClass", GetString(IDS_MOVE_ROOM_TITLE).c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcParent.left + (rcParent.right - rcParent.left - winW) / 2, 
        rcParent.top + (rcParent.bottom - rcParent.top - winH) / 2, 
        winW, winH, hParent, 0, GetModuleHandle(0), 0);
    
    ApplyClassicStyle(hWnd);
    
    EnableWindow(hParent, FALSE);
    
    MSG msg;
    while (IsWindow(hWnd) && GetMessage(&msg, 0, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_DOWN && msg.hwnd == GetDlgItem(hWnd, 101)) {
                HWND hList = GetDlgItem(hWnd, 102);
                SetFocus(hList);
                if (SendMessage(hList, LB_GETCURSEL, 0, 0) == LB_ERR) {
                    SendMessage(hList, LB_SETCURSEL, 0, 0);
                }
                continue;
            } else if (msg.wParam == VK_UP && msg.hwnd == GetDlgItem(hWnd, 102)) {
                int sel = SendMessage(msg.hwnd, LB_GETCURSEL, 0, 0);
                if (sel <= 0) {
                    SetFocus(GetDlgItem(hWnd, 101));
                    continue;
                }
            }
        }
        if (!IsDialogMessage(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
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

std::string HashRoomName(const std::wstring& roomName) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result = "";
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            LPSTR utf8Str = w2a(roomName.c_str());
            if (utf8Str) {
                if (CryptHashData(hHash, (const BYTE*)utf8Str, (DWORD)strlen(utf8Str), 0)) {
                    DWORD hashLen = 0;
                    DWORD cbHashLen = sizeof(DWORD);
                    if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &cbHashLen, 0)) {
                        std::vector<BYTE> buffer(hashLen);
                        if (CryptGetHashParam(hHash, HP_HASHVAL, buffer.data(), &hashLen, 0)) {
                            char hex[3];
                            for (DWORD i = 0; i < hashLen; i++) {
                                sprintf_s(hex, "%02x", buffer[i]);
                                result += hex;
                            }
                        }
                    }
                }
                GlobalFree(utf8Str);
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return "private_" + result;
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
    if (!data.user.empty() && !data.isSystemEvent) {
        {
            std::lock_guard<std::mutex> lock(g_ignoredMutex);
            if (g_ignoredUsers.count(data.user) > 0) return;
        }
    }
    MsgData filteredData = data;
    filteredData.msg = FilterProfanity(filteredData.msg);
    LogEntry entry;
    entry.data = filteredData;
    g_logEntries.push_back(entry);
    
    const size_t MAX_LOG_ENTRIES = 5000;
    if (g_logEntries.size() > MAX_LOG_ENTRIES + 100) {
        size_t eraseCount = g_logEntries.size() - MAX_LOG_ENTRIES;
        float deletedHeight = 0.0f;
        for (size_t i = 0; i < eraseCount; i++) {
            deletedHeight += g_logEntries[i].height;
            if (g_logEntries[i].pLayout) {
                g_logEntries[i].pLayout->Release();
            }
        }
        g_logEntries.erase(g_logEntries.begin(), g_logEntries.begin() + eraseCount);
        if (!g_autoScroll) {
            g_logScrollY -= deletedHeight;
            if (g_logScrollY < 0.0f) g_logScrollY = 0.0f;
        }
    }
    
    InvalidateRect(g_hLog, NULL, FALSE);
}

struct DecodeTaskData {
    std::vector<uint8_t> mediaData;
    std::wstring username;
    bool isH264;
};

DWORD WINAPI DecodeTaskProc(LPVOID lpParam) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    DecodeTaskData* data = (DecodeTaskData*)lpParam;
    if (data->isH264) {
        if (!g_pH264Decoder) {
            g_pH264Decoder = new H264Decoder();
            g_pH264Decoder->Initialize(640, 480);
        }
        std::vector<uint8_t> rgbOut;
        if (g_pH264Decoder->DecodeFrame(data->mediaData, rgbOut)) {
            std::lock_guard<std::mutex> lock(g_videoMutex);
            g_videoPixelsMap[data->username] = rgbOut;
            g_videoSizeMap[data->username] = {640, 480};
        HWND hView = NULL;
        auto it = g_videoWindows.find(data->username);
        if (it != g_videoWindows.end()) hView = it->second;
        if (hView && IsWindow(hView)) InvalidateRect(hView, NULL, FALSE);
        }
    } else {
        DecodeJPEG(data->mediaData, data->username);
    }
    delete data;
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return 0;
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
                                if (json["sessionId"].is_number()) {
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
                                
                                std::string newId = json["userId"].string_value();
                                if (!newId.empty()) {
                                    g_userId = a2w(newId.c_str());
                                }
                            }
                            else if (type == "chat") {
                                MsgData* pData = new MsgData();
                                std::wstring wuid = a2w(json["userId"].string_value().c_str());
                                pData->user = a2w(json["userName"].string_value().c_str());
                                pData->msg = a2w(json["msg"].string_value().c_str());
                                pData->b = json["b"].bool_value();
                                pData->i = json["i"].bool_value();
                                pData->u = json["u"].bool_value();
                                pData->size = json["sz"].is_number() ? json["sz"].int_value() : 10;
                                pData->color = json["clr"].is_number() ? json["clr"].int_value() : RGB(0,0,0);
                                pData->nameColor = json["nclr"].is_number() ? json["nclr"].int_value() : 0xFFFFFFFF;
                                if (pData->nameColor != 0xFFFFFFFF) {
                                    g_userColors[wuid] = pData->nameColor;
                                    g_userNames[wuid] = pData->user;
                                    if (g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                                }
                                pData->isSystemEvent = false;
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
                                    std::wstring wuid = a2w(item["userId"].string_value().c_str());
                                    std::wstring wname = a2w(item["userName"].string_value().c_str());
                                    g_userNames[wuid] = wname;
                                    g_userColors[wuid] = item["nclr"].int_value();
                                    int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wuid.c_str());
                                    if (idx == LB_ERR) {
                                        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)wuid.c_str());
                                    }
                                }
                                // Ensure local user is always in the list
                                g_userNames[g_userId] = g_username;
                                g_userColors[g_userId] = g_nameColor;
                                int localIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_userId.c_str());
                                if (localIdx == LB_ERR) {
                                    SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)g_userId.c_str());
                                }
                                SortUserList();
                                
                                g_isRoomJoined = true;
                                UpdateSendBtnState();
                                
                                if (g_isNameChanging) {
                                    g_isNameChanging = false;
                                } else {
                                    MsgData* pData = new MsgData();
                                    pData->user = g_username;
                                    pData->msg = GetString(IDS_ENTERED);
                                    pData->b = false; pData->i = false; pData->u = false;
                                    pData->size = 9;
                                    pData->color = RGB(128, 128, 128);
                                    pData->nameColor = g_nameColor;
                                    pData->isSystemEvent = true;
                                    PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
                                }
                            }
                            else if (type == "rename") {
                                std::string uid = json["userId"].string_value();
                                std::wstring wuid = a2w(uid.c_str());
                                std::wstring wname = a2w(json["userName"].string_value().c_str());
                                int nclr = json["nclr"].int_value();
                                g_userNames[wuid] = wname;
                                g_userColors[wuid] = nclr;
                                if (g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                                SortUserList();
                                if (g_videoWindows[wuid] && IsWindow(g_videoWindows[wuid])) {
                                    std::wstring titleName = wname.substr(0, wname.find(L'#'));
                                    SetWindowText(g_videoWindows[wuid], titleName.c_str());
                                }
                            }
                            else if (type == "join") {
                                std::string uid = json["userId"].string_value();
                                std::wstring wuid = a2w(uid.c_str());
                                std::wstring wname = a2w(json["userName"].string_value().c_str());
                                COLORREF joinNameColor = json["nclr"].is_number() ? json["nclr"].int_value() : 0xFFFFFFFF;
                                
                                g_userNames[wuid] = wname;
                                if (joinNameColor != 0xFFFFFFFF) {
                                    g_userColors[wuid] = joinNameColor;
                                }
                                if (g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                                
                                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wuid.c_str());
                                if (idx == LB_ERR) {
                                    SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)wuid.c_str());
                                    SortUserList();
                                    if (wuid != g_userId) {
                                        MsgData* pData = new MsgData();
                                        pData->user = wname;
                                        pData->msg = GetString(IDS_ENTERED);
                                        pData->b = false; pData->i = false; pData->u = false;
                                        pData->size = 9;
                                        pData->color = RGB(128, 128, 128);
                                        pData->nameColor = joinNameColor;
                                        pData->isSystemEvent = true;
                                        PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
                                    }
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
                                    bool ok = ResolveIP(newIp, serverAddr.sin_addr);
                                    LogToFile("Redirect resolving " + newIp + " result: " + (ok ? "OK" : "FAILED"));
                                    g_serverIp = newIp; // Update global server IP
                                    g_isServerUdpAddrResolved = false; // Force re-resolve for UDP
    
                                    if (connect(g_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
                                        LogToFile("Redirect TCP connect OK");
                                        std::string finalRoomName;
                                        if (g_isPrivateRoom) {
                                            finalRoomName = HashRoomName(g_currentRoom);
                                        } else {
                                            LPSTR roomA_ptr = w2a(g_currentRoom.c_str());
                                            finalRoomName = roomA_ptr;
                                            GlobalFree(roomA_ptr);
                                        }

                                        json11::Json joinMsg = json11::Json::object {
                                            {"type", "join"},
                                            {"secretId", w2a(g_secretId.c_str())},
                                            {"userName", w2a(g_username.c_str())},
                                            {"roomName", finalRoomName},
                                            {"pub", !g_isPrivateRoom},
                                            {"nclr", (int)g_nameColor}
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
                                    if (wuid != g_userId) {
                                        MsgData* pData = new MsgData();
                                        pData->user = g_userNames[wuid];
                                        pData->msg = GetString(IDS_LEFT);
                                        pData->b = false; pData->i = false; pData->u = false;
                                        pData->size = 9;
                                        pData->color = RGB(128, 128, 128);
                                        pData->nameColor = g_userColors[wuid];
                                        pData->isSystemEvent = true;
                                        PostMessage(hWndParent, WM_USER_MSG, 0, (LPARAM)pData);
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
                                if (g_hMoveRoomDlg && IsWindow(g_hMoveRoomDlg)) {
                                    PostMessage(g_hMoveRoomDlg, WM_USER_ROOM_LIST_UPDATED, 0, 0);
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
                while (true) {
                    sockaddr_in from;
                    int fromlen = sizeof(from);
                    int res = recvfrom(g_sock, udpBuf, sizeof(udpBuf), 0, (sockaddr*)&from, &fromlen);
                    if (res == SOCKET_ERROR) {
                        int err = WSAGetLastError();
                        if (err == WSAEWOULDBLOCK) break;
                        continue;
                    }
                    if (res > 0 && res >= sizeof(MediaHeader)) {
                        MediaHeader* mhdr = (MediaHeader*)udpBuf;
                        
                        bool isIgnored = false;
                        {
                            std::wstring fromUser = mhdr->userId;
                            std::lock_guard<std::mutex> lock(g_ignoredMutex);
                            isIgnored = (g_ignoredUsers.count(fromUser) > 0);
                        }
                        if (isIgnored) continue;

                        if (mhdr->payloadType == 1) { // Audio
                            PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - sizeof(MediaHeader));
                            std::wstring wname = mhdr->userId;
                            
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
                        else if (mhdr->payloadType == 4) {
                            std::wstring wname = mhdr->userId;
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
                        }
                        else if (mhdr->payloadType == 2 || mhdr->payloadType == 5) {
                            std::wstring wname = mhdr->userId;
                            if (wname == g_userId) continue;
                            uint16_t seq = mhdr->seqNum;
                            int fragIdx = mhdr->timestamp;
                            int len = res - sizeof(MediaHeader);
                            bool isH264 = (mhdr->payloadType == 5);
                            
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
                                DecodeTaskData* taskData = new DecodeTaskData();
                                taskData->username = wname;
                                taskData->isH264 = isH264;
                                for (int i = 0; i < vbuf.totalFragments; i++) {
                                    taskData->mediaData.insert(taskData->mediaData.end(), vbuf.fragments[i].begin(), vbuf.fragments[i].end());
                                }
                                QueueUserWorkItem(DecodeTaskProc, taskData, WT_EXECUTEDEFAULT);
                                g_videoBuffers[wname].erase(seq);
                            }
                            
                            // Cleanup old incomplete frames (throttled to 1s)
                            static DWORD lastCleanup = 0;
                            DWORD now = GetTickCount();
                            if (now - lastCleanup > 1000) {
                                lastCleanup = now;
                                for (auto& pair : g_videoBuffers) {
                                    for (auto it = pair.second.begin(); it != pair.second.end(); ) {
                                        if (now - it->second.lastUpdate > 2000) {
                                            it = pair.second.erase(it);
                                        } else {
                                            ++it;
                                        }
                                    }
                                }
                            }
                        }
                        else if (mhdr->payloadType == 3) {
                            // Watching packet
                            std::wstring viewer = mhdr->userId;
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
                    } else if (res <= 0) {
                        break;
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
                    bool ok = ResolveIP(g_serverIp, serverAddr.sin_addr);
                    LogToFile("Initial resolving " + g_serverIp + " result: " + (ok ? "OK" : "FAILED"));
                    
                    if (connect(g_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
                        g_isServerUdpAddrResolved = false;
                        std::string finalRoomName;
                        if (g_isPrivateRoom) {
                            finalRoomName = HashRoomName(g_currentRoom);
                        } else {
                            LPSTR roomA_ptr = w2a(g_currentRoom.c_str());
                            finalRoomName = roomA_ptr;
                            GlobalFree(roomA_ptr);
                        }

                        json11::Json joinMsg = json11::Json::object {
                            {"type", "join"},
                            {"secretId", w2a(g_secretId.c_str())},
                            {"userName", w2a(g_username.c_str())},
                            {"roomName", finalRoomName},
                            {"pub", !g_isPrivateRoom},
                            {"nclr", (int)g_nameColor}
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
    std::string finalRoomName;
    if (g_isPrivateRoom) {
        finalRoomName = HashRoomName(roomName);
    } else {
        LPSTR roomA_ptr = w2a(roomName.c_str());
        finalRoomName = roomA_ptr;
        GlobalFree(roomA_ptr);
    }
    
    json11::Json leaveMsg = json11::Json::object {
        {"type", "leave"},
        {"userId", std::string(userA_ptr)},
        {"roomName", finalRoomName}
    };
    GlobalFree(userA_ptr);
    
    std::string leaveStr = leaveMsg.dump() + "\n";
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, leaveStr.c_str(), (int)leaveStr.length(), 0);
    }
}

void SendJoinRequest(const std::wstring& roomName, bool isPrivate = false) {
    LPSTR userA_ptr = w2a(g_username.c_str());
    std::string finalRoomName;
    if (isPrivate) {
        finalRoomName = HashRoomName(roomName);
    } else {
        LPSTR roomA_ptr = w2a(roomName.c_str());
        finalRoomName = roomA_ptr;
        GlobalFree(roomA_ptr);
    }
    
    json11::Json joinMsg = json11::Json::object {
        {"type", "join"},
        {"secretId", w2a(g_secretId.c_str())},
        {"userName", w2a(g_username.c_str())},
        {"roomName", finalRoomName},
        {"pub", !isPrivate},
        {"nclr", (int)g_nameColor}
    };
    
    std::string joinStr = joinMsg.dump() + "\n";
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, joinStr.c_str(), (int)joinStr.length(), 0);
    }
}

void SendRenameRequest() {
    json11::Json renameMsg = json11::Json::object {
        {"type", "rename"},
        {"userName", w2a(g_username.c_str())},
        {"nclr", (int)g_nameColor}
    };
    std::string renameStr = renameMsg.dump() + "\n";
    if (g_tcpSocket != INVALID_SOCKET) {
        send(g_tcpSocket, renameStr.c_str(), (int)renameStr.length(), 0);
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
        {"clr", (int)g_currentColor},
        {"nclr", (int)g_nameColor}
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
    pData->nameColor = g_nameColor;
    PostMessage(GetParent(g_hInput), WM_USER_MSG, 0, (LPARAM)pData);
    
    GlobalFree(userA_ptr);
    GlobalFree(msgA_ptr);
}

void SnapWindow(HWND hMovingWnd, LPRECT prc);

std::map<HWND, ID2D1HwndRenderTarget*> g_videoRts;

LRESULT CALLBACK VideoViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT classicResult = 0;
    if (HandleClassicStyle(hwnd, message, wParam, lParam, &classicResult)) return classicResult;

    std::wstring* pUser = (std::wstring*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_ENTERSIZEMOVE: {
        extern bool g_isNewDrag;
        g_isNewDrag = true;
        break;
    }
    case WM_MOVING: {
        LPRECT prc = (LPRECT)lParam;
        SnapWindow(hwnd, prc);
        return TRUE;
    }
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
            if (*pUser == g_userId) {
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
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, pUser->c_str(), pUser->length(), NULL, 0, NULL, NULL);
            if (size_needed > 0) {
                targetA.resize(size_needed);
                WideCharToMultiByte(CP_UTF8, 0, pUser->c_str(), pUser->length(), &targetA[0], size_needed, NULL, NULL);
            }
            
            MediaHeader hdr = {0};
            hdr.sessionId = g_sessionId;
            hdr.payloadType = 3;
            hdr.timestamp = GetTickCount();
            wcsncpy_s(hdr.userId, 40, g_userId.c_str(), _TRUNCATE);
            
            char pkt[1024];
            memcpy(pkt, &hdr, sizeof(hdr));
            memcpy(pkt + sizeof(hdr), targetA.c_str(), targetA.length());
            
            sockaddr_in serverUdp = {0};
            serverUdp.sin_family = AF_INET;
            serverUdp.sin_port = htons(4001);
            if (!g_isServerUdpAddrResolved) {
                g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                LogToFile("Resolve UDP " + g_serverIp + ": " + (g_isServerUdpAddrResolved ? "OK" : "FAILED"));
            }
            serverUdp.sin_addr = g_serverUdpAddr;
            int sent = sendto(g_sock, pkt, sizeof(hdr) + (int)targetA.length(), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
            LogToFile("UDP Sending watch packet to " + targetA + ", sent=" + std::to_string(sent));
            
            g_videoViewers[*pUser][g_userId] = GetTickCount();
            
            int count = 0;
            DWORD now = GetTickCount();
            std::wstring viewerText = L"Viewers: ";
            for (auto& pair : g_videoViewers[*pUser]) {
                if (now - pair.second < 5000) {
                    if (pair.first == *pUser) continue;
                    
                    count++;
                    std::wstring vname = pair.first;
                    if (g_userNames.count(pair.first)) vname = g_userNames[pair.first];
                    viewerText += vname + L", ";
                }
            }
            if (count > 0) viewerText = viewerText.substr(0, viewerText.length() - 2);
            
            WCHAR viewBuf[256];
            swprintf_s(viewBuf, GetString(IDS_VIEWING_COUNT).c_str(), count);
            std::wstring dispName = *pUser;
            if (g_userNames.count(*pUser)) dispName = g_userNames[*pUser];
            dispName = dispName.substr(0, dispName.find(L'#'));
            std::wstring title = dispName + L" " + viewBuf;
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
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
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
                    HRESULT hrBmp = pRT->CreateBitmap(D2D1::SizeU(sz.first, sz.second), pixels.data(), sz.first * 4, 
                        D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBitmap);
                    if (SUCCEEDED(hrBmp) && pBitmap) {
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
                    } else {
                        char buf[256]; sprintf_s(buf, "VideoViewProc FAILED: CreateBitmap hr=0x%X, sz=%dx%d", hrBmp, sz.first, sz.second); LogToFile(buf);
                    }
                } else {
                    static DWORD lastEmptyLog = 0;
                    if (GetTickCount() - lastEmptyLog > 1000) {
                        lastEmptyLog = GetTickCount();
                        std::string unameA(pUser->begin(), pUser->end());
                        LogToFile("VideoViewProc FAILED: pixels.empty() is true for " + unameA);
                    }
                }
            }
            pRT->EndDraw();
        } else {
            LogToFile("VideoViewProc FAILED: pRT or pUser is NULL");
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, message, wParam, lParam);
        if (hit == HTCLIENT) return HTCAPTION;
        return hit;
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

RECT GetWindowMargin(HWND hwnd) {
    if (!IsWindow(hwnd)) return {0, 0, 0, 0};
    
    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&rcFrame, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
    
    HDC hdc = GetDC(hwnd);
    int dpiY = 96;
    if (hdc) {
        dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hwnd, hdc);
    }
    int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
    int bw3 = bw1 * 3;

    int cxCrop = -rcFrame.left - bw3;
    int cyCropTop = -rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - bw3;
    int cyCropBottom = rcFrame.bottom - bw3;
    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;

    return { cxCrop, cyCropTop, cxCrop, cyCropBottom };
}

void GetVisibleRect(HWND hwnd, RECT* prc) {
    GetWindowRect(hwnd, prc);
    RECT margin = GetWindowMargin(hwnd);
    prc->left += margin.left;
    prc->top += margin.top;
    prc->right -= margin.right;
    prc->bottom -= margin.bottom;
}

bool g_isNewDrag = false;

void SnapWindow(HWND hMovingWnd, LPRECT prc) {
    static HWND s_hDragWnd = NULL;
    static RECT s_rcUnsnapped = {0,0,0,0};

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
        s_hDragWnd = NULL;
    }

    if (s_hDragWnd != hMovingWnd || g_isNewDrag) {
        s_hDragWnd = hMovingWnd;
        s_rcUnsnapped = *prc;
        g_isNewDrag = false;
    } else {
        RECT rcCurrent;
        GetWindowRect(hMovingWnd, &rcCurrent);
        int dx = prc->left - rcCurrent.left;
        int dy = prc->top - rcCurrent.top;
        s_rcUnsnapped.left += dx;
        s_rcUnsnapped.top += dy;
        s_rcUnsnapped.right = s_rcUnsnapped.left + (prc->right - prc->left);
        s_rcUnsnapped.bottom = s_rcUnsnapped.top + (prc->bottom - prc->top);
    }

    RECT margin = GetWindowMargin(hMovingWnd);
    RECT rcVis = { s_rcUnsnapped.left + margin.left, s_rcUnsnapped.top + margin.top, s_rcUnsnapped.right - margin.right, s_rcUnsnapped.bottom - margin.bottom };

    std::vector<RECT> targets;
    
    HWND hMainWnd = NULL;
    extern HWND g_hUserList;
    if (g_hUserList) {
        hMainWnd = GetParent(g_hUserList);
    }
    
    if (hMainWnd && hMainWnd != hMovingWnd && IsWindowVisible(hMainWnd)) {
        RECT r; GetVisibleRect(hMainWnd, &r); targets.push_back(r);
    }
    for (auto& pair : g_videoWindows) {
        HWND hVid = pair.second;
        if (hVid && hVid != hMovingWnd && IsWindowVisible(hVid)) {
            RECT r; GetVisibleRect(hVid, &r); targets.push_back(r);
        }
    }
    
    int dpiY = 96;
    HDC hdc = GetDC(hMovingWnd);
    if (hdc) {
        dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hMovingWnd, hdc);
    }
    const int SNAP_EDGE = MulDiv(16, dpiY, 96);
    const int SNAP_CORNER = MulDiv(32, dpiY, 96);

    int dx = 0, dy = 0;
    int minDx = 9999, minDy = 9999;
    
    bool cornerSnapped = false;
    
    HMONITOR hMonitor = MonitorFromWindow(hMovingWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);
    
    int scr_x_cands[] = { mi.rcWork.left - rcVis.left, mi.rcWork.right - rcVis.right };
    int scr_y_cands[] = { mi.rcWork.top - rcVis.top, mi.rcWork.bottom - rcVis.bottom };

    for (auto& t : targets) {
        int x_cands[] = { t.left - rcVis.right, t.right - rcVis.left, t.left - rcVis.left, t.right - rcVis.right };
        int y_cands[] = { t.top - rcVis.bottom, t.bottom - rcVis.top, t.top - rcVis.top, t.bottom - rcVis.bottom };
        
        for (int cx : x_cands) {
            for (int cy : y_cands) {
                if (abs(cx) < SNAP_CORNER && abs(cy) < SNAP_CORNER) {
                    if (abs(cx) + abs(cy) < abs(minDx) + abs(minDy)) {
                        minDx = cx; minDy = cy; cornerSnapped = true;
                    }
                }
            }
        }
    }
    
    for (int cx : scr_x_cands) {
        for (int cy : scr_y_cands) {
            if (abs(cx) < SNAP_CORNER && abs(cy) < SNAP_CORNER) {
                if (abs(cx) + abs(cy) < abs(minDx) + abs(minDy)) {
                    minDx = cx; minDy = cy; cornerSnapped = true;
                }
            }
        }
    }
    
    if (cornerSnapped) {
        dx = minDx;
        dy = minDy;
    } else {
        for (auto& t : targets) {
            if (rcVis.bottom > t.top && rcVis.top < t.bottom) {
                int x_cands[] = { t.left - rcVis.right, t.right - rcVis.left, t.left - rcVis.left, t.right - rcVis.right };
                for (int cx : x_cands) {
                    if (abs(cx) < SNAP_EDGE && abs(cx) < abs(minDx)) {
                        minDx = cx;
                    }
                }
            }
            if (rcVis.right > t.left && rcVis.left < t.right) {
                int y_cands[] = { t.top - rcVis.bottom, t.bottom - rcVis.top, t.top - rcVis.top, t.bottom - rcVis.bottom };
                for (int cy : y_cands) {
                    if (abs(cy) < SNAP_EDGE && abs(cy) < abs(minDy)) {
                        minDy = cy;
                    }
                }
            }
        }
        for (int cx : scr_x_cands) {
            if (abs(cx) < SNAP_EDGE && abs(cx) < abs(minDx)) {
                minDx = cx;
            }
        }
        for (int cy : scr_y_cands) {
            if (abs(cy) < SNAP_EDGE && abs(cy) < abs(minDy)) {
                minDy = cy;
            }
        }
        if (minDx != 9999) dx = minDx;
        if (minDy != 9999) dy = minDy;
    }

    rcVis.left += dx;
    rcVis.top += dy;
    rcVis.right += dx;
    rcVis.bottom += dy;
    
    prc->left = rcVis.left - margin.left;
    prc->top = rcVis.top - margin.top;
    prc->right = rcVis.right + margin.right;
    prc->bottom = rcVis.bottom + margin.bottom;
}

void CalculateVideoWindowPosition(HWND hMainWnd, int width, int height, int& outX, int& outY) {
    RECT margin = GetWindowMargin(hMainWnd);
    RECT rcMain;
    GetVisibleRect(hMainWnd, &rcMain);

    HMONITOR hMonitor = MonitorFromWindow(hMainWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);

    int visWidth = width - margin.left - margin.right;
    int visHeight = height - margin.top - margin.bottom;

    int maxBottom = -1;
    int lastX = -1;
    for (auto& pair : g_videoWindows) {
        HWND hVid = pair.second;
        if (hVid && IsWindow(hVid) && IsWindowVisible(hVid)) {
            RECT rcVid;
            GetVisibleRect(hVid, &rcVid);
            if (rcVid.bottom > maxBottom) {
                maxBottom = rcVid.bottom;
                lastX = rcVid.left;
            }
        }
    }

    int targetVisX, targetVisY;

    if (maxBottom != -1) {
        targetVisX = (lastX != -1) ? lastX : rcMain.right;
        targetVisY = maxBottom;
        if (targetVisX + visWidth > mi.rcWork.right) targetVisX = mi.rcWork.right - visWidth;
        if (targetVisY + visHeight > mi.rcWork.bottom) targetVisY = mi.rcWork.bottom - visHeight;
        if (targetVisX < mi.rcWork.left) targetVisX = mi.rcWork.left;
        if (targetVisY < mi.rcWork.top) targetVisY = mi.rcWork.top;
    } else {
        auto fits = [&](int x, int y) {
            return (x >= mi.rcWork.left && x + visWidth <= mi.rcWork.right &&
                    y >= mi.rcWork.top && y + visHeight <= mi.rcWork.bottom);
        };

        // 1. Right
        targetVisX = rcMain.right; targetVisY = rcMain.top;
        if (!fits(targetVisX, targetVisY)) {
            // 2. Bottom
            targetVisX = rcMain.left; targetVisY = rcMain.bottom;
            if (!fits(targetVisX, targetVisY)) {
                // 3. Top
                targetVisX = rcMain.left; targetVisY = rcMain.top - visHeight;
                if (!fits(targetVisX, targetVisY)) {
                    // 4. Left
                    targetVisX = rcMain.left - visWidth; targetVisY = rcMain.top;
                    if (!fits(targetVisX, targetVisY)) {
                        // 5. Clamped Right
                        targetVisX = rcMain.right; targetVisY = rcMain.top;
                        if (targetVisX + visWidth > mi.rcWork.right) targetVisX = mi.rcWork.right - visWidth;
                        if (targetVisY + visHeight > mi.rcWork.bottom) targetVisY = mi.rcWork.bottom - visHeight;
                        if (targetVisX < mi.rcWork.left) targetVisX = mi.rcWork.left;
                        if (targetVisY < mi.rcWork.top) targetVisY = mi.rcWork.top;
                    }
                }
            }
        }
    }

    outX = targetVisX - margin.left;
    outY = targetVisY - margin.top;
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
                
                if (clickedUser == g_userId) {
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenuW(hMenu, MF_STRING | (g_isWebcamActive ? MF_CHECKED : MF_UNCHECKED), 1, GetString(IDS_VIDEO_STREAM).c_str());
                    AppendMenuW(hMenu, MF_STRING, 3, GetString(IDS_CHANGE_NAME_TITLE).c_str()); 
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(hMenu, g_useH264 ? (MF_STRING | MF_CHECKED) : MF_STRING, 10, L"H.264 ハードウェアエンコード");
                    ClientToScreen(hwnd, &pt);
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);
                    if (cmd == 1) {
                        if (g_isWebcamActive) {
                            g_isWebcamActive = false;
                            if (g_videoWindows[g_userId] && IsWindow(g_videoWindows[g_userId])) {
                                DestroyWindow(g_videoWindows[g_userId]);
                            }
                        } else {
                            if (!g_videoWindows[g_userId] || !IsWindow(g_videoWindows[g_userId])) {
                                std::wstring titleName = g_username.substr(0, g_username.find(L'#'));
                                int vidX = CW_USEDEFAULT, vidY = CW_USEDEFAULT;
                                CalculateVideoWindowPosition(GetParent(hwnd), Scale(320), Scale(240), vidX, vidY);
                                HWND hVid = CreateWindowEx(0, L"D2DVideoViewClass", titleName.c_str(),
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE, vidX, vidY, Scale(320), Scale(240), NULL, NULL, GetModuleHandle(NULL), (LPVOID)g_userId.c_str());
                                ApplyClassicStyle(hVid);
                                g_videoWindows[g_userId] = hVid;
                            } else {
                                SetForegroundWindow(g_videoWindows[g_userId]);
                            }
                            g_isWebcamActive = true;
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                    } else if (cmd == 3) {
                        std::wstring newName;
                        if (ShowChangeNameDialog(GetParent(hwnd), newName) && !newName.empty() && newName != g_username) {
                            g_username = newName;
                            g_userNames[g_userId] = g_username;
                            SaveSettings();
                            SortUserList();
                            InvalidateRect(hwnd, NULL, TRUE);
                            SendRenameRequest();
                        }
                    } else if (cmd == 10) {
                        g_useH264 = !g_useH264;
                        if (!g_useH264 && g_pH264Encoder) {
                            delete g_pH264Encoder;
                            g_pH264Encoder = nullptr;
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
                    AppendMenuW(hMenu, MF_STRING, 5, GetString(IDS_REPORT).c_str());
                    ClientToScreen(hwnd, &pt);
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);
                    if (cmd == 2 && hasStreamer) {
                        if (!g_videoWindows[clickedUser] || !IsWindow(g_videoWindows[clickedUser])) {
                            std::wstring titleName = clickedUser;
                            if (g_userNames.count(clickedUser)) titleName = g_userNames[clickedUser];
                            titleName = titleName.substr(0, titleName.find(L'#'));
                            int vidX = CW_USEDEFAULT, vidY = CW_USEDEFAULT;
                            CalculateVideoWindowPosition(GetParent(hwnd), Scale(320), Scale(240), vidX, vidY);
                            HWND hVid = CreateWindowEx(0, L"D2DVideoViewClass", titleName.c_str(),
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, vidX, vidY, Scale(320), Scale(240), NULL, NULL, GetModuleHandle(NULL), (LPVOID)clickedUser.c_str());
                            ApplyClassicStyle(hVid);
                            g_videoWindows[clickedUser] = hVid;
                        } else {
                            SetForegroundWindow(g_videoWindows[clickedUser]);
                        }
                    } else if (cmd == 4) {
                        {
                            std::lock_guard<std::mutex> lock(g_ignoredMutex);
                            if (g_ignoredUsers.count(clickedUser) > 0) {
                                g_ignoredUsers.erase(clickedUser);
                            } else {
                                g_ignoredUsers.insert(clickedUser);
                                if (g_videoWindows[clickedUser] && IsWindow(g_videoWindows[clickedUser])) {
                                    DestroyWindow(g_videoWindows[clickedUser]);
                                }
                            }
                        }
                        SaveIgnoredUsers();
                        SortUserList();
                        InvalidateRect(hwnd, NULL, FALSE);
                    } else if (cmd == 5) {
                        {
                            std::lock_guard<std::mutex> lock(g_ignoredMutex);
                            g_ignoredUsers.insert(clickedUser);
                            if (g_videoWindows[clickedUser] && IsWindow(g_videoWindows[clickedUser])) {
                                DestroyWindow(g_videoWindows[clickedUser]);
                            }
                        }
                        SaveIgnoredUsers();
                        SortUserList();
                        LPSTR utf8Target = w2a(clickedUser.c_str());
                        json11::Json reportMsg = json11::Json::object {
                            {"type", "report"},
                            {"target", utf8Target}
                        };
                        GlobalFree(utf8Target);
                        std::string msg = reportMsg.dump() + "\n";
                        if (g_tcpSocket != INVALID_SOCKET) {
                            send(g_tcpSocket, msg.c_str(), msg.length(), 0);
                        }
                        MessageBoxW(hwnd, GetString(IDS_REPORTED).c_str(), L"Report", MB_OK);
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            }
        }
        return 0;
    } else if (msg == WM_LBUTTONDBLCLK) {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        LRESULT res = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
        if (HIWORD(res) == 0) {
            int item = LOWORD(res);
            int len = (int)SendMessage(hwnd, LB_GETTEXTLEN, item, 0);
            if (len != LB_ERR && len >= 0) {
                std::vector<WCHAR> buf(len + 1, 0);
                SendMessage(hwnd, LB_GETTEXT, item, (LPARAM)buf.data());
                std::wstring clickedUser(buf.data());
                if (clickedUser == g_userId) {
                    std::wstring newName;
                    if (ShowChangeNameDialog(GetParent(hwnd), newName) && !newName.empty() && newName != g_username) {
                        g_username = newName;
                        g_userNames[g_userId] = g_username;
                        SaveSettings();
                        SortUserList();
                        InvalidateRect(hwnd, NULL, TRUE);
                        SendRenameRequest();
                    }
                }
            }
        }
        // Let the default listbox procedure handle the selection first, then we did our logic.
        // Actually, CallWindowProc will be called below.
    }
    return CallWindowProc(g_oldListProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ClassicButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        HBRUSH hBgBrush = CreateSolidBrush(RGB(192, 192, 192));
        FillRect(hdc, &rc, hBgBrush);
        DeleteObject(hBgBrush);

        bool isPressed = false;
        bool isChecked = false;

        DWORD style = GetWindowLong(hWnd, GWL_STYLE);
        if ((style & BS_TYPEMASK) == BS_AUTOCHECKBOX || (style & BS_TYPEMASK) == BS_CHECKBOX || (style & BS_TYPEMASK) == BS_RADIOBUTTON || (style & BS_TYPEMASK) == BS_AUTORADIOBUTTON) {
            isChecked = (SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
        }
        isPressed = (SendMessage(hWnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;

        UINT edge = (isPressed || isChecked) ? EDGE_SUNKEN : EDGE_RAISED;
        DrawScaledButtonBorder(hdc, rc, edge);

        if (GetFocus() == hWnd) {
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
            RECT rcFocus = rc;
            InflateRect(&rcFocus, -bw * 2, -bw * 2);
            DrawFocusRect(hdc, &rcFocus);
        }

        WCHAR text[256];
        GetWindowText(hWnd, text, 256);
        if (wcslen(text) > 0) {
            SetBkMode(hdc, TRANSPARENT);
            HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
            HFONT hOld = (HFONT)SelectObject(hdc, hFont);

            RECT rcText = rc;
            if (isPressed || isChecked) {
                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
                rcText.left += bw1 * 2;
                rcText.top += bw1 * 2;
            }

            if (!IsWindowEnabled(hWnd)) {
                RECT rcShadow = rcText;
                rcShadow.left += 1; rcShadow.top += 1; rcShadow.right += 1; rcShadow.bottom += 1;
                SetTextColor(hdc, RGB(255, 255, 255));
                DrawTextW(hdc, text, -1, &rcShadow, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SetTextColor(hdc, RGB(128, 128, 128));
                DrawTextW(hdc, text, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                SetTextColor(hdc, RGB(0, 0, 0));
                DrawTextW(hdc, text, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            SelectObject(hdc, hOld);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    else if (msg == WM_ERASEBKGND) {
        return 1;
    }
    else if (msg == BM_SETSTATE || msg == BM_SETCHECK || 
             msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK ||
             msg == WM_MOUSEMOVE || msg == WM_MOUSELEAVE || 
             msg == WM_SETFOCUS || msg == WM_KILLFOCUS || msg == WM_ENABLE) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK ComboBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_PAINT) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) {
            RECT rc;
            GetWindowRect(hWnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
            int bw2 = bw1 * 2;
            
            int btnWidth = GetSystemMetrics(SM_CXVSCROLL);
            RECT rcBtn = { width - btnWidth - bw2, bw2, width - bw2, height - bw2 };
            
            HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
            FillRect(hdc, &rcBtn, hBg);
            DeleteObject(hBg);
            
            bool isPressed = (SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0) != 0);
            
            DrawScaledButtonBorder(hdc, rcBtn, isPressed ? EDGE_SUNKEN : EDGE_RAISED);
            
            int cx = rcBtn.left + (rcBtn.right - rcBtn.left) / 2;
            int cy = rcBtn.top + (rcBtn.bottom - rcBtn.top) / 2;
            if (isPressed) { cx += bw1; cy += bw1; }
            
            int s = bw1;
            HPEN hBlack = CreatePen(PS_SOLID, s, RGB(0,0,0));
            HBRUSH hBlackBrush = CreateSolidBrush(RGB(0,0,0));
            HPEN hOldP = (HPEN)SelectObject(hdc, hBlack);
            HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBlackBrush);
            
            POINT pts[3] = {
                { cx - 3*s, cy - 1*s },
                { cx + 3*s, cy - 1*s },
                { cx, cy + 2*s }
            };
            Polygon(hdc, pts, 3);
            
            SelectObject(hdc, hOldP);
            SelectObject(hdc, hOldB);
            DeleteObject(hBlack);
            DeleteObject(hBlackBrush);
            
            ReleaseDC(hWnd, hdc);
        }
        return res;
    }
    else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK ||
             msg == WM_MOUSEMOVE || msg == WM_MOUSELEAVE || 
             msg == WM_SETFOCUS || msg == WM_KILLFOCUS) {
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT classicResult = 0;
    if (HandleClassicStyle(hWnd, msg, wParam, lParam, &classicResult)) return classicResult;

    switch (msg) {
    case WM_ENTERSIZEMOVE: {
        extern bool g_isNewDrag;
        g_isNewDrag = true;
        break;
    }
    case WM_MOVING: {
        LPRECT prc = (LPRECT)lParam;
        SnapWindow(hWnd, prc);
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
        int minClientWidth = Scale(5) * 2 + Scale(444);
        int minClientHeight = Scale(300);

        RECT rc = { 0, 0, minClientWidth, minClientHeight };
        AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
        pmmi->ptMinTrackSize.x = rc.right - rc.left;
        pmmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }
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
        
        #ifndef SIO_UDP_CONNRESET
        #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
        #endif
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(g_sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

        u_long mode = 1;
        ioctlsocket(g_sock, FIONBIO, &mode);

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

        g_hFont = CreateFontW(Scale(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"MS UI Gothic");
        g_hFontBold = CreateFontW(Scale(12), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"MS UI Gothic");
        g_hFontI_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
            
        g_hFontU_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
            
        g_hFontB_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
        g_hFontCombo = CreateFontW(Scale(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"MS UI Gothic");

        // UI Creation
        g_hLog = CreateWindowEx(WS_EX_CLIENTEDGE, L"D2DLogViewClass", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hUserList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
            0, 0, 0, 0, hWnd, (HMENU)1050, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hUserListHeader = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"ユーザ一覧 (0)",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hWnd, (HMENU)1054, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        SetWindowTheme(g_hLog, L" ", L" ");
        SetWindowTheme(g_hUserList, L" ", L" ");
        SetWindowTheme(g_hUserListHeader, L" ", L" ");
        
        SetWindowSubclass(g_hLog, ClientEdgeSubclassProc, 0, 0);
        SetWindowSubclass(g_hUserList, ClientEdgeSubclassProc, 0, 0);
        SetWindowSubclass(g_hUserListHeader, ClientEdgeSubclassProc, 0, 0);

        SetWindowSubclass(g_hLog, ScrollbarColorProc, 0, 0);
        SetWindowSubclass(g_hUserList, ScrollbarColorProc, 0, 0);



        g_hBtnMoveRoom = CreateWindow(L"BUTTON", L"", 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1008, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        g_hBtnVoice = CreateWindow(L"BUTTON", GetString(IDS_TALK).c_str(), 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1009, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hVolumeIndicator = CreateWindow(L"STATIC", L"", 
            WS_CHILD | SS_OWNERDRAW,
            0, 0, 0, 0, hWnd, (HMENU)1051, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hSpeakerIcon = CreateWindow(L"STATIC", L"", 
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY,
            0, 0, 0, 0, hWnd, (HMENU)1053, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hVolumeSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS,
            0, 0, 0, 0, hWnd, (HMENU)1052, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        SendMessage(g_hVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessage(g_hVolumeSlider, TBM_SETTICFREQ, 10, 0);
        SetWindowTheme(g_hVolumeSlider, L" ", L" ");

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

        const WCHAR* sizes[] = { L"6", L"7", L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"28", L"32" };
        for (auto sz : sizes) SendMessage(g_hComboSize, CB_ADDSTRING, 0, (LPARAM)sz);
        SendMessage(g_hComboSize, CB_SETCURSEL, 4, 0);

        SendMessage(g_hUserList, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessage(g_hUserListHeader, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        SendMessage(g_hSendBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessage(g_hBtnB, WM_SETFONT, (WPARAM)g_hFontB_btn, TRUE);
        SendMessage(g_hBtnI, WM_SETFONT, (WPARAM)g_hFontI_btn, TRUE);
        SendMessage(g_hBtnU, WM_SETFONT, (WPARAM)g_hFontU_btn, TRUE);
        SendMessage(g_hBtnVideo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        
        SetWindowTheme(g_hSendBtn, L" ", L" ");
        SetWindowSubclass(g_hSendBtn, ClassicButtonProc, 0, 0);
        SetWindowTheme(g_hBtnB, L" ", L" ");
        SetWindowSubclass(g_hBtnB, ClassicButtonProc, 0, 0);
        SetWindowTheme(g_hBtnI, L" ", L" ");
        SetWindowSubclass(g_hBtnI, ClassicButtonProc, 0, 0);
        SetWindowTheme(g_hBtnU, L" ", L" ");
        SetWindowSubclass(g_hBtnU, ClassicButtonProc, 0, 0);
        SetWindowTheme(g_hBtnVideo, L" ", L" ");
        SetWindowSubclass(g_hBtnVideo, ClassicButtonProc, 0, 0);
        SendMessage(g_hComboSize, WM_SETFONT, (WPARAM)g_hFontCombo, TRUE);
        SetWindowTheme(g_hComboSize, L" ", L" ");
        SetWindowSubclass(g_hComboSize, ClientEdgeSubclassProc, 999, 0);
        SetWindowSubclass(g_hComboSize, ComboBoxSubclassProc, 0, 0);
        SetWindowPos(g_hComboSize, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        SetWindowSubclass(g_hInput, ClientEdgeSubclassProc, 0, 0);

        UpdateInputFormat();
        SetWindowTitle(hWnd);

        // Set the 16x16 icon for the title bar. 
        // We leave ICON_BIG as NULL so the Windows Shell will extract the best icon (like 24x24) natively from the EXE!
//        HICON hIcon16 = (HICON)LoadImage(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
//        if (hIcon16) {
//            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon16);
//        }


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
        EnableWindow(g_hSendBtn, FALSE);
        SendJoinRequest(g_currentRoom, g_isPrivateRoom);
        CreateThread(0, 0, RecvThread, hWnd, 0, 0);
        SetTimer(hWnd, 1, 3000, NULL);
        SetTimer(hWnd, 2, 100, NULL);
        InitVolumeSync(hWnd);

        return 0;
    }

    case WM_MEASUREITEM: {
        LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
        if (lpmis->CtlID == 1050) {
            lpmis->itemHeight = Scale(28);
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        
        if (lpdis->CtlID == 1009) {
            HDC hdcPaint = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            HDC hdc = CreateCompatibleDC(hdcPaint);
            HBITMAP hBmp = CreateCompatibleBitmap(hdcPaint, rc.right - rc.left, rc.bottom - rc.top);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdc, hBmp);

            if (g_isVoiceActive) {
                SetDCBrushColor(hdc, RGB(100, 255, 100));
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
            } else {
                HBRUSH hFaceBrush = CreateSolidBrush(RGB(192, 192, 192));
                FillRect(hdc, &rc, hFaceBrush);
                DeleteObject(hFaceBrush);
            }
            UINT edge = ((lpdis->itemState & ODS_SELECTED) || g_isVoiceActive) ? EDGE_SUNKEN : EDGE_RAISED;
            DrawScaledButtonBorder(hdc, rc, edge);
            
            if (lpdis->itemState & ODS_FOCUS) {
                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
                RECT rcFocus = rc;
                InflateRect(&rcFocus, -bw*2, -bw*2);
                DrawFocusRect(hdc, &rcFocus);
            }

            {
                Gdiplus::Graphics graphics(hdc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                int cx = rc.left + Scale(14);
                int cy = rc.top + (rc.bottom - rc.top) / 2 - Scale(2);

                Gdiplus::SolidBrush micBrush(Gdiplus::Color(255, 0, 0, 0));
                Gdiplus::Pen micPen(Gdiplus::Color(255, 0, 0, 0), (Gdiplus::REAL)(Scale(1) * 1.5f));
                
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

            BitBlt(hdcPaint, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, hOldBmp);
            DeleteObject(hBmp);
            DeleteDC(hdc);

            return TRUE;
        }
        else if (lpdis->CtlID == 1008) {
            HDC hdcPaint = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            HDC hdc = CreateCompatibleDC(hdcPaint);
            HBITMAP hBmp = CreateCompatibleBitmap(hdcPaint, rc.right - rc.left, rc.bottom - rc.top);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdc, hBmp);

            HBRUSH hFaceBrush = CreateSolidBrush(RGB(192, 192, 192));
            FillRect(hdc, &rc, hFaceBrush);
            DeleteObject(hFaceBrush);
            UINT edge = (lpdis->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_RAISED;
            DrawScaledButtonBorder(hdc, rc, edge);
            if (lpdis->itemState & ODS_FOCUS) {
                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
                RECT rcFocus = rc; InflateRect(&rcFocus, -bw*2, -bw*2); DrawFocusRect(hdc, &rcFocus);
            }

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
                
                Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), (Gdiplus::REAL)(Scale(1) * 1.5f));
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

            BitBlt(hdcPaint, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, hOldBmp);
            DeleteObject(hBmp);
            DeleteDC(hdc);

            return TRUE;
        }
        else if (lpdis->CtlID == 1002) {
            DrawColorButton(lpdis, g_currentColor);
            return TRUE;
        }
        else if (lpdis->CtlID == 1051) {
            HDC hdcPaint = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            HDC hdc = CreateCompatibleDC(hdcPaint);
            HBITMAP hBmp = CreateCompatibleBitmap(hdcPaint, rc.right - rc.left, rc.bottom - rc.top);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdc, hBmp);
            
            HBRUSH hFaceBrush = CreateSolidBrush(RGB(192, 192, 192));
            FillRect(hdc, &rc, hFaceBrush);
            DeleteObject(hFaceBrush);
            
            if (g_isVoiceActive) {
                if (!g_hWaveIn) {
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, RGB(255, 0, 0));
                    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
                    RECT textRc = rc;
                    DrawText(hdc, L"接続なし", -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(hdc, hOldFont);
                } else {
                    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                    int bw = MulDiv(2, dpiY, 96); if (bw < 2) bw = 2;
                    
                    int maxVolWidth = rc.right - rc.left - bw * 2;
                    int volWidth = (maxVolWidth * g_currentMicVolume) / 100;
                    if (volWidth > 0) {
                        Gdiplus::Graphics graphics(hdc);
                        
                        Gdiplus::Color colors[] = {
                            Gdiplus::Color(255, 0, 220, 0),    // Green
                            Gdiplus::Color(255, 255, 220, 0),  // Yellow
                            Gdiplus::Color(255, 255, 0, 0)     // Red
                        };
                        float positions[] = { 0.0f, 0.7f, 1.0f };
                        
                        Gdiplus::LinearGradientBrush linBrush(
                            Gdiplus::Point(rc.left + bw, rc.top + bw),
                            Gdiplus::Point(rc.right - bw, rc.top + bw),
                            colors[0], colors[2]);
                        linBrush.SetInterpolationColors(colors, positions, 3);
                        
                        graphics.FillRectangle(&linBrush, rc.left + bw, rc.top + bw, volWidth, rc.bottom - rc.top - bw * 2);
                    }
                }
            }
            
            DrawScaledButtonBorder(hdc, rc, EDGE_SUNKEN);

            BitBlt(hdcPaint, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, hOldBmp);
            DeleteObject(hBmp);
            DeleteDC(hdc);

            return TRUE;
        }
        else if (lpdis->CtlID == 1053) {
            HDC hdc = lpdis->hDC;
            RECT rc = lpdis->rcItem;
            
            // Draw background
            HBRUSH hBgBrush = CreateSolidBrush(RGB(192, 192, 192));
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 60, 60, 60));
            Gdiplus::Pen blackPen(Gdiplus::Color(255, 60, 60, 60), (Gdiplus::REAL)(Scale(1) * 1.5f));
            BOOL isMuted = FALSE;
            float masterVol = 1.0f;
            if (g_pSimpleAudioVolume) {
                g_pSimpleAudioVolume->GetMute(&isMuted);
                g_pSimpleAudioVolume->GetMasterVolume(&masterVol);
            }
            bool showMute = isMuted || (masterVol <= 0.0f);

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

            if (showMute) {
                int xCenter = cx + Scale(7);
                int yCenter = cy;
                int xSize = Scale(3);
                graphics.DrawLine(&blackPen, xCenter - xSize, yCenter - xSize, xCenter + xSize, yCenter + xSize);
                graphics.DrawLine(&blackPen, xCenter - xSize, yCenter + xSize, xCenter + xSize, yCenter - xSize);
            } else {
                // Sound waves (natural circular arcs, slightly smaller)
                int waveCx = cx + Scale(2);
                int r1 = Scale(5);
                graphics.DrawArc(&blackPen, waveCx - r1, cy - r1, r1 * 2, r1 * 2, -45.0f, 90.0f);
                
                int r2 = Scale(9);
                graphics.DrawArc(&blackPen, waveCx - r2, cy - r2, r2 * 2, r2 * 2, -45.0f, 90.0f);
            }

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
            std::wstring user(szText); // This is the userId

            std::wstring displayUser = user;
            if (g_userNames.count(user)) {
                displayUser = g_userNames[user];
            }

            bool isIgnored = false;
            {
                std::lock_guard<std::mutex> lock(g_ignoredMutex);
                isIgnored = (g_ignoredUsers.count(user) > 0);
            }

            COLORREF userColor = GetSysColor(COLOR_WINDOWTEXT);
            if (!isIgnored) {
                if (user == g_userId && g_nameColor != 0xFFFFFFFF) {
                    userColor = g_nameColor;
                } else if (g_userColors.count(user) > 0) {
                    userColor = g_userColors[user];
                } else {
                    size_t hash = 0;
                    for (wchar_t c : displayUser) hash = hash * 31 + c;
                    COLORREF palette[] = {
                        RGB(200, 50, 50), RGB(50, 150, 50), RGB(50, 50, 200),
                        RGB(180, 180, 50), RGB(180, 50, 180), RGB(50, 180, 180),
                        RGB(255, 128, 0), RGB(128, 0, 255), RGB(0, 128, 255),
                        RGB(128, 200, 0), RGB(255, 0, 128), RGB(0, 200, 128)
                    };
                    userColor = palette[hash % 12];
                }
            }

            if (lpdis->itemState & ODS_SELECTED) {
                HBRUSH hSelBrush = CreateSolidBrush(RGB(0, 0, 128));
                FillRect(hdc, &rc, hSelBrush);
                DeleteObject(hSelBrush);
                SetTextColor(hdc, isIgnored ? RGB(180, 180, 180) : RGB(255, 255, 255));
            } else {
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
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
            if (user == g_userId && g_isWebcamActive) isStreaming = true;
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
                int antBaseX = tvX + (iconSize + Scale(2)) / 2 + Scale(1);
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
            if (user == g_userId && g_isVoiceActive) {
                isVoicing = true;
                currentVolume = g_currentMicVolume;
            }
            if (isIgnored) isVoicing = false;
            
            if (isVoicing) {
                float margin = Scale(2);
                float visTop = rc.top + margin + Scale(1);
                float visBotForU = rc.bottom - margin - Scale(5);
                float visH = visBotForU - visTop;
                float u = visH / 14.0f;
                float mx = std::round(rc.right - 8.0f * u) + Scale(1);
                float my = std::round(visTop + 6.0f * u);
                
                float visBot = visBotForU + Scale(1);
                
                float penWidth = (float)Scale(1);
                if (penWidth < 1.0f) penWidth = 1.0f;
                
                float clipTop = visTop - penWidth;
                float clipBot = visBot + penWidth;
                float clipH = clipBot - clipTop;
                
                float fillHeight = (clipH * currentVolume) / 100.0f;
                if (fillHeight > clipH) fillHeight = clipH;
                
                Gdiplus::RectF fullMicRect(mx - 6*u, clipTop, 12*u, clipH);
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
                
                Gdiplus::Pen gradientPen(&linGrBrush, penWidth);
                
                bool isSelected = (lpdis->itemState & ODS_SELECTED);
                Gdiplus::Color baseColor = isSelected ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 40, 40, 40);
                Gdiplus::Pen basePen(baseColor, penWidth);
                Gdiplus::SolidBrush baseBrush(baseColor);
                
                auto DrawMic = [&](Gdiplus::Pen* pen, Gdiplus::Brush* brush) {
                    graphics.FillRectangle(brush, mx - 2*u, my - 4*u, 4*u, 6*u);
                    graphics.FillEllipse(brush, mx - 2*u, my - 6*u, 4*u, 4*u);
                    graphics.FillEllipse(brush, mx - 2*u, my, 4*u, 4*u);
                    
                    Gdiplus::SmoothingMode oldMode = graphics.GetSmoothingMode();
                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
                    graphics.DrawArc(pen, mx - 4*u, my - 2*u, 8*u, 8*u, 0.0f, 180.0f);
                    graphics.DrawLine(pen, mx, my + 6*u, mx, visBot);
                    graphics.DrawLine(pen, mx - 3*u, visBot, mx + 3*u, visBot);
                    graphics.SetSmoothingMode(oldMode);
                };
                
                float blackHeight = clipH - fillHeight;
                if (blackHeight > 0) {
                    Gdiplus::RectF blackClip(mx - 10*u, clipTop, 20*u, blackHeight);
                    graphics.SetClip(blackClip);
                    DrawMic(&basePen, &baseBrush);
                    graphics.ResetClip();
                }
                
                if (fillHeight > 0) {
                    Gdiplus::RectF gradientClip(mx - 10*u, clipBot - fillHeight, 20*u, fillHeight);
                    graphics.SetClip(gradientClip);
                    DrawMic(&gradientPen, &linGrBrush);
                    graphics.ResetClip();
                }
            }
            } // End GDI+ scope
            
            HFONT hListFont = (HFONT)SendMessage(lpdis->hwndItem, WM_GETFONT, 0, 0);
            HFONT hOldMemFont = (HFONT)SelectObject(hdc, hListFont);
            
            HFONT hOldFont = NULL;
            if (user == g_userId) {
                hOldFont = (HFONT)SelectObject(hdc, g_hFontBold);
            }
            
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
            
        }
        break;
    }

    case WM_NOTIFY: {
        NMHDR* nmhdr = (NMHDR*)lParam;
        if (nmhdr->hwndFrom == g_hVolumeSlider && nmhdr->code == NM_CUSTOMDRAW) {
            NMCUSTOMDRAW* pcd = (NMCUSTOMDRAW*)lParam;
            if (pcd->dwDrawStage == CDDS_PREPAINT) {
                HDC hdc = pcd->hdc;
                RECT rcBg = pcd->rc;
                HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                FillRect(hdc, &rcBg, hBg);
                DeleteObject(hBg);
                
                int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                RECT rcChannel;
                SendMessage(g_hVolumeSlider, TBM_GETCHANNELRECT, 0, (LPARAM)&rcChannel);
                
                int thumbW = 10 * s;
                int xStart = rcChannel.left + thumbW / 2;
                int xEnd = rcChannel.right - thumbW / 2;
                
                int tick1_Y1 = rcChannel.bottom + 3 * s;
                int tick1_Y2 = tick1_Y1 + 2 * s;
                
                int tick2_Y2 = rcChannel.top - 3 * s;
                int tick2_Y1 = tick2_Y2 - 2 * s;
                
                for (int i = 0; i <= 10; i++) {
                    int x = xStart + (xEnd - xStart) * i / 10;
                    MoveToEx(hdc, x, tick1_Y1, NULL);
                    LineTo(hdc, x, tick1_Y2);
                    MoveToEx(hdc, x, tick2_Y1, NULL);
                    LineTo(hdc, x, tick2_Y2);
                }
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                return CDRF_NOTIFYITEMDRAW;
            } else if (pcd->dwDrawStage == CDDS_ITEMPREPAINT) {
                if (pcd->dwItemSpec == TBCD_THUMB) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &rc, hBg);
                    DeleteObject(hBg);
                    DrawScaledButtonBorder(hdc, rc, EDGE_RAISED);
                    return CDRF_SKIPDEFAULT;
                } else if (pcd->dwItemSpec == TBCD_CHANNEL) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                    int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;
                    rc.top += (rc.bottom - rc.top) / 2 - 2*s;
                    rc.bottom = rc.top + 4*s;
                    DrawScaledButtonBorder(hdc, rc, EDGE_SUNKEN);
                    return CDRF_SKIPDEFAULT;
                } else if (pcd->dwItemSpec == TBCD_TICS) {
                    return CDRF_SKIPDEFAULT;
                }
            }
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
        
        HDWP hdwp = BeginDeferWindowPos(15);

        hdwp = DeferWindowPos(hdwp, g_hLog, NULL, margin, margin, logWidth, topHeight, SWP_NOZORDER);

        int headerHeight = Scale(20);
        int listHeight = topHeight - roomBtnHeight - margin - headerHeight;
        hdwp = DeferWindowPos(hdwp, g_hUserListHeader, NULL, margin * 2 + logWidth, margin, listWidth, headerHeight, SWP_NOZORDER);
        hdwp = DeferWindowPos(hdwp, g_hUserList, NULL, margin * 2 + logWidth, margin + headerHeight, listWidth, listHeight, SWP_NOZORDER);

        int roomBtnY = margin + headerHeight + listHeight + margin;
        hdwp = DeferWindowPos(hdwp, g_hBtnMoveRoom, NULL, margin * 2 + logWidth, roomBtnY, listWidth, roomBtnHeight, SWP_NOZORDER);

        int toolY = margin * 2 + topHeight;
        int currentX = margin;
        int gap = Scale(4);

        hdwp = DeferWindowPos(hdwp, g_hBtnB, NULL, currentX, toolY, Scale(24), toolHeight, SWP_NOZORDER);
        currentX += Scale(24) + gap;

        hdwp = DeferWindowPos(hdwp, g_hBtnI, NULL, currentX, toolY, Scale(24), toolHeight, SWP_NOZORDER);
        currentX += Scale(24) + gap;

        hdwp = DeferWindowPos(hdwp, g_hBtnU, NULL, currentX, toolY, Scale(24), toolHeight, SWP_NOZORDER);
        currentX += Scale(24) + gap;

        // g_hComboSize must not be handled by DeferWindowPos, as SetWindowPos on a combobox reverts its custom closed height.
        // We conditionally position it to avoid reverting its height repeatedly during window resize.
        int comboW = Scale(48);
        RECT rcCurrentCombo;
        GetWindowRect(g_hComboSize, &rcCurrentCombo);
        POINT ptCombo = { rcCurrentCombo.left, rcCurrentCombo.top };
        ScreenToClient(hWnd, &ptCombo);

        if (ptCombo.x != currentX || ptCombo.y != toolY || (rcCurrentCombo.right - rcCurrentCombo.left) != comboW) {
            SetWindowPos(g_hComboSize, NULL, currentX, toolY, comboW, Scale(400), SWP_NOZORDER);
        }

        GetWindowRect(g_hComboSize, &rcCurrentCombo);
        int currentComboHeight = rcCurrentCombo.bottom - rcCurrentCombo.top;
        int currentItemHeight = SendMessage(g_hComboSize, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
        int nonItemHeight = currentComboHeight - currentItemHeight;

        static int lastGoodNonItemHeight = Scale(6);
        if (nonItemHeight > 0 && nonItemHeight < Scale(20)) {
            lastGoodNonItemHeight = nonItemHeight;
        }

        int targetItemHeight = toolHeight - lastGoodNonItemHeight;
        if (currentItemHeight != targetItemHeight) {
            SendMessage(g_hComboSize, CB_SETITEMHEIGHT, (WPARAM)-1, targetItemHeight);
        }
        currentX += comboW + gap;

        hdwp = DeferWindowPos(hdwp, g_hBtnColor, NULL, currentX, toolY, Scale(32), toolHeight, SWP_NOZORDER);
        currentX += Scale(32) + gap;

        hdwp = DeferWindowPos(hdwp, g_hBtnVoice, NULL, currentX, toolY, Scale(70), toolHeight, SWP_NOZORDER);
        currentX += Scale(70) + gap;

        hdwp = DeferWindowPos(hdwp, g_hVolumeIndicator, NULL, currentX, toolY, Scale(70), toolHeight, SWP_NOZORDER);
        currentX += Scale(70) + gap;

        hdwp = DeferWindowPos(hdwp, g_hSpeakerIcon, NULL, currentX, toolY, Scale(20), toolHeight, SWP_NOZORDER);
        currentX += Scale(20) + gap;

        hdwp = DeferWindowPos(hdwp, g_hVolumeSlider, NULL, currentX, toolY, Scale(100), toolHeight, SWP_NOZORDER);

        int bottomY = margin * 3 + topHeight + toolHeight;
        hdwp = DeferWindowPos(hdwp, g_hInput, NULL, margin, bottomY, width - btnWidth - margin * 3, inputHeight, SWP_NOZORDER);
        hdwp = DeferWindowPos(hdwp, g_hSendBtn, NULL, width - btnWidth - margin, bottomY, btnWidth, inputHeight, SWP_NOZORDER);

        EndDeferWindowPos(hdwp);

        return 0;
    }

    case WM_HSCROLL: {
        if ((HWND)lParam == g_hVolumeSlider) {
            if (g_pSimpleAudioVolume) {
                int pos = SendMessage(g_hVolumeSlider, TBM_GETPOS, 0, 0);
                float volume = pos / 100.0f;
                BOOL isMuted = FALSE;
                g_pSimpleAudioVolume->GetMute(&isMuted);
                if (isMuted && volume > 0.0f) {
                    g_pSimpleAudioVolume->SetMute(FALSE, NULL);
                }
                g_pSimpleAudioVolume->SetMasterVolume(volume, NULL);
                if (g_hSpeakerIcon) {
                    InvalidateRect(g_hSpeakerIcon, NULL, FALSE);
                }
            }
        }
        return 0;
    }

    case WM_APP_VOLUME_CHANGED: {
        if (g_hVolumeSlider) {
            BOOL isMuted = (BOOL)lParam;
            int displayPos = isMuted ? 0 : (int)wParam;
            SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, displayPos);
            if (g_hSpeakerIcon) {
                InvalidateRect(g_hSpeakerIcon, NULL, FALSE);
            }
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
            cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
            cc.lpfnHook = ColorHookProc;
            if (ChooseColor(&cc)) {
                g_currentColor = cc.rgbResult;
                InvalidateRect(g_hBtnColor, NULL, FALSE);
                UpdateInputFormat();
                SetFocus(g_hInput);
            }
        }
        else if (id == 1053 && code == STN_CLICKED) {
            if (g_pSimpleAudioVolume) {
                BOOL isMuted = FALSE;
                g_pSimpleAudioVolume->GetMute(&isMuted);
                g_pSimpleAudioVolume->SetMute(!isMuted, NULL);
                InvalidateRect(g_hSpeakerIcon, NULL, FALSE);
            }
        }
        else if ((id >= 1003 && id <= 1005 && code == BN_CLICKED) || (id == 1006 && (code == CBN_SELCHANGE || code == CBN_CLOSEUP))) {
            if (id == 1006) {
                InvalidateRect(g_hComboSize, NULL, FALSE);
            }
            if (code != CBN_CLOSEUP) {
                UpdateInputFormat();
                SetFocus(g_hInput);
            }
        }

        else if (id == 1009 && code == BN_CLICKED) {
            if (!g_isVoiceActive) {
                g_isVoiceActive = true;
                if (InitAudioIn()) {
                    waveInStart(g_hWaveIn);
                }
                ShowWindow(g_hVolumeIndicator, SW_SHOW);
            } else {
                g_isVoiceActive = false;
                CleanupAudioIn();
                g_currentMicVolume = 0;
                ShowWindow(g_hVolumeIndicator, SW_HIDE);
            }
            InvalidateRect(g_hVolumeIndicator, NULL, FALSE);
            InvalidateRect(g_hBtnVoice, NULL, FALSE);
            
            // ユーザリスト上の自分のマイクアイコンを再描画
            if (g_hUserList) {
                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_userId.c_str());
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

                SendJoinRequest(newRoom, isPrivate);
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
                        std::wstring deadUser = it->first;
                        listChanged = true;
                        it = g_videoStreamers.erase(it);
                        if (g_videoWindows[deadUser] && IsWindow(g_videoWindows[deadUser])) {
                            DestroyWindow(g_videoWindows[deadUser]);
                        }
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
            if (g_hUserListHeader) {
                static int lastCount = -1;
                int count = (int)SendMessage(g_hUserList, LB_GETCOUNT, 0, 0);
                if (count != lastCount) {
                    lastCount = count;
                    WCHAR buf[256];
                    swprintf_s(buf, L"ユーザ一覧 (%d)", count);
                    SetWindowText(g_hUserListHeader, buf);
                }
            }

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
                int idx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)g_userId.c_str());
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
        g_isPrivateRoom = false;
        SendJoinRequest(g_currentRoom, g_isPrivateRoom);
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
        DeleteObject(g_hFontB_btn);
        DeleteObject(g_hFontCombo);
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
    LoadIgnoredUsers();
    
    HDC hdcDpi = GetDC(NULL);
    g_dpi = GetDeviceCaps(hdcDpi, LOGPIXELSX);
    ReleaseDC(NULL, hdcDpi);

    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);
    
    MSG msg;
    WNDCLASS wc = {
        0,
        WndProc, 0, 0, hInstance,
        LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)), LoadCursor(0, IDC_ARROW), // hIcon is NULL
        (HBRUSH)CreateSolidBrush(RGB(192, 192, 192)), 0, L"Retro Chat"
    };
    RegisterClass(&wc);
    
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int minClientWidth = Scale(5) * 2 + Scale(444);
    int minClientHeight = Scale(300);
    RECT rcMin = { 0, 0, minClientWidth, minClientHeight };
    AdjustWindowRectEx(&rcMin, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE, 0);

    HWND hWnd = CreateWindow(
        L"Retro Chat", GetString(IDS_WINDOW_TITLE).c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, rcMin.right - rcMin.left, rcMin.bottom - rcMin.top,
        0, 0, hInstance, 0);
        
    // SetThemeAppProperties(0);
    SetWindowTheme(hWnd, L" ", L" ");
    SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)FALSE, 0);

    ApplyClassicStyle(hWnd);
        
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
