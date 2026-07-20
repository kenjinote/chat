import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Fix missing InvalidateRect
    old_udp = """                                g_voiceUsers[wname] = GetTickCount();
                                g_voiceVolumes[wname] = volume;
                            }
                        }"""
    new_udp = """                                g_voiceUsers[wname] = GetTickCount();
                                g_voiceVolumes[wname] = volume;
                            }
                            if (newlySpeaking && g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);
                        }"""
    content = content.replace(old_udp, new_udp)

    # 2. Fix AudioMixerLoop missing WHDR_DONE buffers
    old_mixer = """    while (g_audioMixerRunning) {
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
                float x = mixed[i] / 32768.0f;
                float abs_x = x < 0 ? -x : x;
                float y;
                if (abs_x < 0.5f) {
                    y = x;
                } else if (abs_x < 1.5f) {
                    float val = abs_x - 0.5f;
                    float shaped = 0.5f + val - 0.5f * val * val;
                    y = (x > 0) ? shaped : -shaped;
                } else {
                    y = (x > 0) ? 1.0f : -1.0f;
                }
                outSamples[i] = (short)(y * 32767.0f);
            }

            g_waveOutHdr[freeIdx].dwBufferLength = AUDIO_BUF_SIZE;
            waveOutWrite(g_hWaveOut, &g_waveOutHdr[freeIdx], sizeof(WAVEHDR));
        }
    }"""
    
    new_mixer = """    while (g_audioMixerRunning) {
        WaitForSingleObject(g_hAudioEvent, INFINITE);
        if (!g_audioMixerRunning) break;

        bool processed = true;
        while (processed) {
            processed = false;
            for (int i = 0; i < AUDIO_OUT_BUF_COUNT; i++) {
                if (g_waveOutHdr[i].dwFlags & WHDR_DONE) {
                    const int sampleCount = AUDIO_BUF_SIZE / 2;
                    std::vector<int> mixed(sampleCount, 0);

                    {
                        std::lock_guard<std::mutex> lock(g_audioMixerMutex);
                        for (auto it = g_jitterBuffers.begin(); it != g_jitterBuffers.end(); ++it) {
                            if (!it->second.empty()) {
                                const JitterFrame& frame = it->second.front();
                                int len = (int)frame.samples.size();
                                if (len > sampleCount) len = sampleCount;
                                
                                for (int k = 0; k < len; k++) {
                                    mixed[k] += frame.samples[k];
                                }
                                it->second.pop();
                            }
                        }
                    }

                    short* outSamples = (short*)g_waveOutBuf[i];
                    for (int k = 0; k < sampleCount; k++) {
                        float x = mixed[k] / 32768.0f;
                        float abs_x = x < 0 ? -x : x;
                        float y;
                        if (abs_x < 0.5f) {
                            y = x;
                        } else if (abs_x < 1.5f) {
                            float val = abs_x - 0.5f;
                            float shaped = 0.5f + val - 0.5f * val * val;
                            y = (x > 0) ? shaped : -shaped;
                        } else {
                            y = (x > 0) ? 1.0f : -1.0f;
                        }
                        outSamples[k] = (short)(y * 32767.0f);
                    }

                    g_waveOutHdr[i].dwBufferLength = AUDIO_BUF_SIZE;
                    waveOutWrite(g_hWaveOut, &g_waveOutHdr[i], sizeof(WAVEHDR));
                    processed = true;
                }
            }
        }
    }"""
    content = content.replace(old_mixer, new_mixer)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched audio loop and invalidate")

if __name__ == '__main__':
    patch()
