import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add headers and globals
audio_globals = """#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

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

#pragma pack(push, 1)
struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t payloadType; // 1 = Audio, 2 = Video, 255 = HolePunch
    uint8_t flags;
    uint32_t timestamp;
};
#pragma pack(pop)

void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pHdr = (WAVEHDR*)dwParam1;
        if (pHdr->dwBytesRecorded > 0 && g_isVoiceActive && g_sock != INVALID_SOCKET) {
            char udpPkt[2048];
            MediaHeader* mhdr = (MediaHeader*)udpPkt;
            mhdr->sessionId = g_sessionId;
            mhdr->seqNum = 0;
            mhdr->payloadType = 1;
            mhdr->flags = 0;
            mhdr->timestamp = GetTickCount();
            
            memcpy(udpPkt + sizeof(MediaHeader), pHdr->lpData, pHdr->dwBytesRecorded);
            
            sockaddr_in serverUdp = {0};
            serverUdp.sin_family = AF_INET;
            serverUdp.sin_port = htons(4001);
            inet_pton(AF_INET, g_serverIp.c_str(), &serverUdp.sin_addr);
            
            sendto(g_sock, udpPkt, sizeof(MediaHeader) + pHdr->dwBytesRecorded, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
        }
        waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR));
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
"""

content = content.replace('#include "json11.hpp"', '#include "json11.hpp"\n' + audio_globals)

# 2. Add buttons HWND
content = content.replace("HWND g_hBtnCreateRoom, g_hBtnMoveRoom;", "HWND g_hBtnCreateRoom, g_hBtnMoveRoom;\nHWND g_hBtnVoice, g_hBtnVideo;")

# 3. Remove struct inside RecvThread
recv_hdr_struct = """#pragma pack(push, 1)
                                struct MediaHeader {
                                    uint32_t sessionId;
                                    uint16_t seqNum;
                                    uint8_t payloadType;
                                    uint8_t flags;
                                    uint32_t timestamp;
                                };
                                #pragma pack(pop)
                                
                                MediaHeader hdr = {0};"""
content = content.replace(recv_hdr_struct, "MediaHeader hdr = {0};")

# 4. Handle audio playback in RecvThread UDP section
udp_recv_old = """if (res > 0) {
                    // Media decoding logic goes here in the future
                }"""
udp_recv_new = """if (res >= sizeof(MediaHeader)) {
                    MediaHeader* mhdr = (MediaHeader*)udpBuf;
                    if (mhdr->payloadType == 1) { // Audio
                        PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - sizeof(MediaHeader));
                    }
                }"""
content = content.replace(udp_recv_old, udp_recv_new)

# 5. Add button creation in InitNetwork (WM_CREATE)
btn_create_old = """g_hBtnMoveRoom = CreateWindow(L"BUTTON", L"\\x90E8\\x5C4B\\x79FB\\x52D5", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)1008, ((LPCREATESTRUCT)lParam)->hInstance, 0);"""
btn_create_new = btn_create_old + """
        g_hBtnVoice = CreateWindow(L"BUTTON", L"Voice: OFF", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)1009, ((LPCREATESTRUCT)lParam)->hInstance, 0);

        g_hBtnVideo = CreateWindow(L"BUTTON", L"Video: OFF", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)1010, ((LPCREATESTRUCT)lParam)->hInstance, 0);
            
        InitAudio();"""
content = content.replace(btn_create_old, btn_create_new)

# 6. Button placement in WM_SIZE
size_old = """MoveWindow(g_hBtnMoveRoom, xOffset, 10, btnWidth, 30, TRUE);
            xOffset += btnWidth + 10;"""
size_new = size_old + """
            MoveWindow(g_hBtnVoice, xOffset, 10, btnWidth, 30, TRUE);
            xOffset += btnWidth + 10;
            MoveWindow(g_hBtnVideo, xOffset, 10, btnWidth, 30, TRUE);
            xOffset += btnWidth + 10;"""
content = content.replace(size_old, size_new)

# 7. Button clicks in WM_COMMAND
cmd_old = """else if (id == 1008 && code == BN_CLICKED) {"""
cmd_new = """else if (id == 1009 && code == BN_CLICKED) {
            g_isVoiceActive = !g_isVoiceActive;
            if (g_isVoiceActive) {
                SetWindowText(g_hBtnVoice, L"Voice: ON");
                waveInStart(g_hWaveIn);
            } else {
                SetWindowText(g_hBtnVoice, L"Voice: OFF");
                waveInStop(g_hWaveIn);
            }
        }
        else if (id == 1010 && code == BN_CLICKED) {
            // Video toggle (TODO)
        }
        """ + cmd_old
content = content.replace(cmd_old, cmd_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Patch applied")
