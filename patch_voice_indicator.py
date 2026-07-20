import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Restore the voice indicator logic
    old_udp = """                            {
                                std::lock_guard<std::mutex> lock(g_audioMixerMutex);
                                JitterFrame frame;
                                frame.samples.assign(samples, samples + sampleCount);
                                g_jitterBuffers[wname].push(frame);
                                
                                if (g_jitterBuffers[wname].size() > 5) {
                                    g_jitterBuffers[wname].pop();
                                }
                            }"""
    
    new_udp = """                            {
                                std::lock_guard<std::mutex> lock(g_audioMixerMutex);
                                JitterFrame frame;
                                frame.samples.assign(samples, samples + sampleCount);
                                g_jitterBuffers[wname].push(frame);
                                
                                if (g_jitterBuffers[wname].size() > 5) {
                                    g_jitterBuffers[wname].pop();
                                }
                            }
                            
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
                            if (newlySpeaking && g_hUserList) InvalidateRect(g_hUserList, NULL, FALSE);"""
    content = content.replace(old_udp, new_udp)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Restored voice indicator logic")

if __name__ == '__main__':
    patch()
