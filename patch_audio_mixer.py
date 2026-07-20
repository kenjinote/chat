import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add globals
    globals_code = """#include <queue>
#include <map>

std::mutex g_audioMixerMutex;
HANDLE g_hAudioEvent = NULL;
bool g_audioMixerRunning = false;
std::thread g_audioMixerThread;

struct JitterFrame {
    std::vector<short> samples;
};
std::map<std::wstring, std::queue<JitterFrame>> g_jitterBuffers;

"""
    if "g_audioMixerMutex" not in content:
        content = content.replace("HWAVEOUT g_hWaveOut = NULL;", globals_code + "HWAVEOUT g_hWaveOut = NULL;")

    # 2. Modify InitAudio and add AudioMixerLoop
    old_init_audio = """    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

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
}"""

    new_init_audio = """    g_hAudioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)g_hAudioEvent, 0, CALLBACK_EVENT);

    for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
        g_waveOutHdr[i].lpData = g_waveOutBuf[i];
        g_waveOutHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
        g_waveOutHdr[i].dwFlags = 0;
        waveOutPrepareHeader(g_hWaveOut, &g_waveOutHdr[i], sizeof(WAVEHDR));
    }
}

void AudioMixerLoop() {
    for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
        memset(g_waveOutBuf[i], 0, AUDIO_BUF_SIZE);
        g_waveOutHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
        waveOutWrite(g_hWaveOut, &g_waveOutHdr[i], sizeof(WAVEHDR));
    }

    while (g_audioMixerRunning) {
        WaitForSingleObject(g_hAudioEvent, INFINITE);
        if (!g_audioMixerRunning) break;

        int freeIdx = -1;
        for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
            if (g_waveOutHdr[i].dwFlags & WHDR_DONE) {
                freeIdx = i;
                break;
            }
        }

        if (freeIdx != -1) {
            const int sampleCount = AUDIO_BUF_SIZE / 2;
            std::vector<int> mixed(sampleCount, 0);

            {
                std::lock_guard<std::mutex> lock(g_audioMixerMutex);
                for (auto it = g_jitterBuffers.begin(); it != g_jitterBuffers.end(); ++it) {
                    if (!it->second.empty()) {
                        const JitterFrame& frame = it->second.front();
                        int len = (int)frame.samples.size();
                        if (len > sampleCount) len = sampleCount;
                        
                        for (int i = 0; i < len; i++) {
                            mixed[i] += frame.samples[i];
                        }
                        it->second.pop();
                    }
                }
            }

            short* outSamples = (short*)g_waveOutBuf[freeIdx];
            for (int i = 0; i < sampleCount; i++) {
                int val = mixed[i];
                if (val > 32767) val = 32767;
                else if (val < -32768) val = -32768;
                outSamples[i] = (short)val;
            }

            g_waveOutHdr[freeIdx].dwBufferLength = AUDIO_BUF_SIZE;
            waveOutWrite(g_hWaveOut, &g_waveOutHdr[freeIdx], sizeof(WAVEHDR));
        }
    }
}"""
    content = content.replace(old_init_audio, new_init_audio)

    # 3. Modify UDP Thread
    old_udp = """                        if (mhdr->payloadType == 1) { // Audio
                            PlayAudioFrame(udpBuf + sizeof(MediaHeader), res - (int)sizeof(MediaHeader));
                            std::wstring wname = mhdr->userId;
                            
                            int audioLen = res - (int)sizeof(MediaHeader);
                            short* samples = (short*)(udpBuf + sizeof(MediaHeader));
                            int sampleCount = audioLen / 2;"""
    
    new_udp = """                        if (mhdr->payloadType == 1) { // Audio
                            std::wstring wname = mhdr->userId;
                            int audioLen = res - (int)sizeof(MediaHeader);
                            short* samples = (short*)(udpBuf + sizeof(MediaHeader));
                            int sampleCount = audioLen / 2;
                            
                            {
                                std::lock_guard<std::mutex> lock(g_audioMixerMutex);
                                JitterFrame frame;
                                frame.samples.assign(samples, samples + sampleCount);
                                g_jitterBuffers[wname].push(frame);
                                
                                if (g_jitterBuffers[wname].size() > 5) {
                                    g_jitterBuffers[wname].pop();
                                }
                            }"""
    content = content.replace(old_udp, new_udp)

    # 4. Start and Stop Mixer Thread in main
    old_main_start = """    InitAudio();
    
    std::thread udpThread(UdpReceiveLoop);"""
    
    new_main_start = """    InitAudio();
    g_audioMixerRunning = true;
    g_audioMixerThread = std::thread(AudioMixerLoop);
    
    std::thread udpThread(UdpReceiveLoop);"""
    content = content.replace(old_main_start, new_main_start)

    old_main_stop = """    g_udpRunning = false;
    closesocket(g_sock);
    if (udpThread.joinable()) udpThread.join();"""

    new_main_stop = """    g_udpRunning = false;
    closesocket(g_sock);
    if (udpThread.joinable()) udpThread.join();

    g_audioMixerRunning = false;
    if (g_hAudioEvent) SetEvent(g_hAudioEvent);
    if (g_audioMixerThread.joinable()) g_audioMixerThread.join();"""
    content = content.replace(old_main_stop, new_main_stop)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched audio mixer")

if __name__ == '__main__':
    patch()
