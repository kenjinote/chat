import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    old_clamp = """            short* outSamples = (short*)g_waveOutBuf[freeIdx];
            for (int i = 0; i < sampleCount; i++) {
                int val = mixed[i];
                if (val > 32767) val = 32767;
                else if (val < -32768) val = -32768;
                outSamples[i] = (short)val;
            }"""

    new_clamp = """            short* outSamples = (short*)g_waveOutBuf[freeIdx];
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
            }"""

    content = content.replace(old_clamp, new_clamp)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched soft clipper")

if __name__ == '__main__':
    patch()
