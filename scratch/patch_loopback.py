import re

with open("Source.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Load loopback.cpp
with open("scratch/loopback.cpp", "r", encoding="utf-8") as f:
    loopback = f.read()

# Remove includes and externs from loopback.cpp
loopback = re.sub(r'#include <audioclient\.h>.*?extern HWND g_hVolumeIndicator;\nextern DWORD g_lastVoiceActiveTime;\n', '', loopback, flags=re.DOTALL)

# Insert the loopback thread before InitAudioIn
content = content.replace("bool InitAudioIn() {", loopback + "\n\nbool InitWinMMCapture() {")

# Rename InitAudioIn and CleanupAudioIn
content = content.replace("void CleanupAudioIn() {", "void StopWinMMCapture() {")

# Update InitWinMMCapture to use device ID
wmm_old = "if (waveInOpen(&g_hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)"
wmm_new = """UINT deviceId = WAVE_MAPPER;
    if (g_selectedAudioDevice >= 0) {
        deviceId = (UINT)g_selectedAudioDevice;
    }
    if (waveInOpen(&g_hWaveIn, deviceId, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)"""
content = content.replace(wmm_old, wmm_new)

# Add InitAudioIn and CleanupAudioIn wrappers
wrappers = """
bool InitAudioIn() {
    if (!g_opusEncoder) {
        int err;
        g_opusEncoder = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &err);
        if (g_opusEncoder) {
            opus_encoder_ctl(g_opusEncoder, OPUS_SET_BITRATE(32000));
        }
    }
    
    if (g_selectedAudioDevice == -2) {
        return StartWASAPICapture();
    } else {
        return InitWinMMCapture();
    }
}

void CleanupAudioIn() {
    if (g_selectedAudioDevice == -2) {
        StopWASAPICapture();
    } else {
        StopWinMMCapture();
    }
}
"""
content = content.replace("bool InitWinMMCapture() {", wrappers + "\n\nbool InitWinMMCapture() {")

with open("Source.cpp", "w", encoding="utf-8") as f:
    f.write(content)
