#include <audioclient.h>
#include <mmdeviceapi.h>

extern bool g_isVoiceActive;
extern SOCKET g_sock;
extern std::string g_userId;
extern long long g_sessionId;
extern OpusEncoder* g_opusEncoder;

std::thread* g_pAudioLoopbackThread = nullptr;
bool g_runAudioLoopback = false;
extern void UpdateInputFormat();
extern bool g_isServerUdpAddrResolved;
extern sockaddr_in g_serverUdpAddr;
extern std::string g_serverIp;
extern bool ResolveIP(const std::string& host, sockaddr_in& addr);
extern int g_currentMicVolume;
extern HWND g_hVolumeIndicator;
extern DWORD g_lastVoiceActiveTime;

void AudioLoopbackThreadFunc() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    IMMDeviceEnumerator* pEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        IMMDevice* pDevice = nullptr;
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (SUCCEEDED(hr)) {
            IAudioClient* pAudioClient = nullptr;
            hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
            if (SUCCEEDED(hr)) {
                WAVEFORMATEX* pwfx = nullptr;
                pAudioClient->GetMixFormat(&pwfx);
                
                hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, pwfx, NULL);
                if (SUCCEEDED(hr)) {
                    IAudioCaptureClient* pCaptureClient = nullptr;
                    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
                    if (SUCCEEDED(hr)) {
                        pAudioClient->Start();
                        
                        bool isFloat = false;
                        if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) isFloat = true;
                        else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                            WAVEFORMATEXTENSIBLE* pEx = (WAVEFORMATEXTENSIBLE*)pwfx;
                            if (IsEqualGUID(pEx->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) isFloat = true;
                        }
                        
                        float ratio = pwfx->nSamplesPerSec / 16000.0f;
                        int inChannels = pwfx->nChannels;
                        
                        short resampleBuf[320];
                        int resampleCount = 0;
                        float phase = 0.0f;
                        
                        UINT32 packetLength = 0;
                        while (g_runAudioLoopback && g_sock != INVALID_SOCKET) {
                            Sleep(10);
                            hr = pCaptureClient->GetNextPacketSize(&packetLength);
                            while (packetLength != 0 && g_runAudioLoopback) {
                                BYTE* pData;
                                UINT32 numFramesAvailable;
                                DWORD flags;
                                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
                                if (SUCCEEDED(hr)) {
                                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                                        // Ignore silent buffers or treat as 0
                                    } else {
                                        float* fSamples = (float*)pData;
                                        short* sSamples = (short*)pData;
                                        
                                        for (UINT32 i = 0; i < numFramesAvailable; i++) {
                                            phase += 1.0f;
                                            if (phase >= ratio) {
                                                phase -= ratio;
                                                
                                                float sum = 0.0f;
                                                if (isFloat) {
                                                    for (int c = 0; c < inChannels; c++) sum += fSamples[i * inChannels + c];
                                                } else {
                                                    for (int c = 0; c < inChannels; c++) sum += (sSamples[i * inChannels + c] / 32768.0f);
                                                }
                                                sum /= inChannels;
                                                
                                                if (sum > 1.0f) sum = 1.0f;
                                                if (sum < -1.0f) sum = -1.0f;
                                                resampleBuf[resampleCount++] = (short)(sum * 32767.0f);
                                                
                                                if (resampleCount >= 320) {
                                                    // Compute volume
                                                    long long sumSquares = 0;
                                                    for (int s = 0; s < 320; s++) {
                                                        sumSquares += resampleBuf[s] * resampleBuf[s];
                                                    }
                                                    int rms = (int)sqrt(sumSquares / 320);
                                                    g_currentMicVolume = (rms * 100) / 32768;
                                                    if (g_currentMicVolume > 100) g_currentMicVolume = 100;
                                                    if (g_currentMicVolume < 0) g_currentMicVolume = 0;
                                                    
                                                    if (rms > 150) g_lastVoiceActiveTime = GetTickCount();
                                                    
                                                    g_currentMicVolume *= 3;
                                                    if (g_currentMicVolume > 100) g_currentMicVolume = 100;
                                                    InvalidateRect(g_hVolumeIndicator, NULL, FALSE);
                                                    
                                                    if (GetTickCount() - g_lastVoiceActiveTime <= 500) {
                                                        if (g_opusEncoder) {
                                                            char udpPkt[2048];
                                                            MediaHeader* mhdr = (MediaHeader*)udpPkt;
                                                            mhdr->sessionId = g_sessionId;
                                                            mhdr->seqNum = 0;
                                                            mhdr->payloadType = 1;
                                                            mhdr->flags = 0;
                                                            mhdr->timestamp = GetTickCount();
                                                            wcsncpy_s(mhdr->userId, 40, g_userId.c_str(), _TRUNCATE);
                                                            
                                                            unsigned char* payloadData = (unsigned char*)(udpPkt + sizeof(MediaHeader));
                                                            int opusBytes = opus_encode(g_opusEncoder, resampleBuf, 320, payloadData, sizeof(udpPkt) - sizeof(MediaHeader));
                                                            if (opusBytes > 0) {
                                                                sockaddr_in serverUdp = {0};
                                                                serverUdp.sin_family = AF_INET;
                                                                serverUdp.sin_port = htons(4001);
                                                                if (!g_isServerUdpAddrResolved) g_isServerUdpAddrResolved = ResolveIP(g_serverIp, g_serverUdpAddr);
                                                                serverUdp.sin_addr = g_serverUdpAddr;
                                                                sendto(g_sock, udpPkt, sizeof(MediaHeader) + opusBytes, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                                                            }
                                                        }
                                                    }
                                                    resampleCount = 0;
                                                }
                                            }
                                        }
                                    }
                                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                                }
                                pCaptureClient->GetNextPacketSize(&packetLength);
                            }
                        }
                        pAudioClient->Stop();
                        pCaptureClient->Release();
                    }
                }
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
            }
            pDevice->Release();
        }
        pEnumerator->Release();
    }
    CoUninitialize();
}

bool StartWASAPICapture() {
    g_runAudioLoopback = true;
    g_pAudioLoopbackThread = new std::thread(AudioLoopbackThreadFunc);
    return true;
}

void StopWASAPICapture() {
    g_runAudioLoopback = false;
    if (g_pAudioLoopbackThread) {
        if (g_pAudioLoopbackThread->joinable()) g_pAudioLoopbackThread->join();
        delete g_pAudioLoopbackThread;
        g_pAudioLoopbackThread = nullptr;
    }
}
