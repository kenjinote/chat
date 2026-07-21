#include "MacAudioEngine.h"
#import <AudioToolbox/AudioToolbox.h>
#include <iostream>

static void HandleInputBuffer(void *aqData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer, 
                              const AudioTimeStamp *inStartTime, UInt32 inNumPackets, 
                              const AudioStreamPacketDescription *inPacketDesc) {
    MacAudioEngine *engine = (MacAudioEngine *)aqData;
    if (engine->IsRecording() && inNumPackets > 0 && inBuffer->mAudioDataByteSize > 0) {
        engine->ProcessInputData((const uint8_t*)inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
    }
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
}

static void HandleOutputBuffer(void *aqData, AudioQueueRef outAQ, AudioQueueBufferRef outBuffer) {
    MacAudioEngine *engine = (MacAudioEngine *)aqData;
    engine->FillOutputBuffer(outBuffer);
    AudioQueueEnqueueBuffer(outAQ, outBuffer, 0, NULL);
}

MacAudioEngine::MacAudioEngine() 
    : m_isInitialized(false), m_isRecording(false), m_isPlaying(false), m_callback(nullptr),
      m_audioQueueInput(nullptr), m_audioQueueOutput(nullptr) {
    for (int i = 0; i < 3; i++) {
        m_inBuffers[i] = nullptr;
        m_outBuffers[i] = nullptr;
    }
}

MacAudioEngine::~MacAudioEngine() {
    StopMicrophone();
    StopPlayback();
    if (m_audioQueueInput) {
        AudioQueueDispose((AudioQueueRef)m_audioQueueInput, true);
    }
    if (m_audioQueueOutput) {
        AudioQueueDispose((AudioQueueRef)m_audioQueueOutput, true);
    }
}

bool MacAudioEngine::Initialize() {
    AudioStreamBasicDescription asbd = {0};
    asbd.mSampleRate = 16000;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    asbd.mBytesPerPacket = 2;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = 2;
    asbd.mChannelsPerFrame = 1;
    asbd.mBitsPerChannel = 16;
    
    OSStatus status = AudioQueueNewInput(&asbd, HandleInputBuffer, this, NULL, kCFRunLoopCommonModes, 0, (AudioQueueRef*)&m_audioQueueInput);
    if (status != noErr) {
        std::cerr << "Failed to create AudioQueueInput" << std::endl;
        return false;
    }
    
    int bufferByteSize = 16000 * 2 * 0.1; // 100ms buffer = 3200 bytes
    for (int i = 0; i < 3; ++i) {
        AudioQueueAllocateBuffer((AudioQueueRef)m_audioQueueInput, bufferByteSize, (AudioQueueBufferRef*)&m_inBuffers[i]);
        AudioQueueEnqueueBuffer((AudioQueueRef)m_audioQueueInput, (AudioQueueBufferRef)m_inBuffers[i], 0, NULL);
    }
    
    // Output Queue Setup
    status = AudioQueueNewOutput(&asbd, HandleOutputBuffer, this, NULL, kCFRunLoopCommonModes, 0, (AudioQueueRef*)&m_audioQueueOutput);
    if (status != noErr) {
        std::cerr << "Failed to create AudioQueueOutput" << std::endl;
        return false;
    }
    
    for (int i = 0; i < 3; ++i) {
        AudioQueueAllocateBuffer((AudioQueueRef)m_audioQueueOutput, bufferByteSize, (AudioQueueBufferRef*)&m_outBuffers[i]);
        // Don't enqueue output buffers until we start playback
    }
    
    m_isInitialized = true;
    return true;
}

void MacAudioEngine::SetAudioCallback(AudioDataCallback callback) {
    m_callback = callback;
}

void MacAudioEngine::SetVolumeCallback(VolumeCallback callback) {
    m_volCallback = callback;
}

void MacAudioEngine::ProcessInputData(const uint8_t* data, size_t size) {
    if (m_volCallback && size > 0) {
        double sum = 0;
        int16_t* pSamples = (int16_t*)data;
        int numSamples = size / 2;
        for (int i = 0; i < numSamples; i++) {
            sum += pSamples[i] * pSamples[i];
        }
        double rms = sqrt(sum / numSamples);
        int volume = (rms * 100) / 32768;
        volume = volume * 3;
        if (volume > 100) volume = 100;
        if (volume < 0) volume = 0;
        m_volCallback(volume);
    }
    
    if (m_callback) {
        m_callback(data, size);
    }
}

void MacAudioEngine::ProcessIncomingAudio(const uint8_t* data, size_t size) {
    if (!m_isPlaying) return;
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_audioBufferQueue.push(std::vector<uint8_t>(data, data + size));
}

void MacAudioEngine::FillOutputBuffer(void* buffer) {
    AudioQueueBufferRef outBuffer = (AudioQueueBufferRef)buffer;
    std::lock_guard<std::mutex> lock(m_audioMutex);
    
    uint8_t* target = (uint8_t*)outBuffer->mAudioData;
    size_t targetCapacity = outBuffer->mAudioDataBytesCapacity;
    size_t bytesFilled = 0;
    
    while (bytesFilled < targetCapacity && !m_audioBufferQueue.empty()) {
        auto& data = m_audioBufferQueue.front();
        size_t needed = targetCapacity - bytesFilled;
        if (data.size() <= needed) {
            memcpy(target + bytesFilled, data.data(), data.size());
            bytesFilled += data.size();
            m_audioBufferQueue.pop();
        } else {
            memcpy(target + bytesFilled, data.data(), needed);
            data.erase(data.begin(), data.begin() + needed);
            bytesFilled += needed;
        }
    }
    
    if (bytesFilled < targetCapacity) {
        memset(target + bytesFilled, 0, targetCapacity - bytesFilled);
    }
    outBuffer->mAudioDataByteSize = (UInt32)targetCapacity;
}

void MacAudioEngine::StartMicrophone() {
    if (!m_isInitialized) return;
    m_isRecording = true;
    AudioQueueStart((AudioQueueRef)m_audioQueueInput, NULL);
    std::cout << "Microphone capture started (CoreAudio)." << std::endl;
}

void MacAudioEngine::StopMicrophone() {
    if (!m_isRecording) return;
    m_isRecording = false;
    AudioQueueStop((AudioQueueRef)m_audioQueueInput, true);
    std::cout << "Microphone capture stopped." << std::endl;
}

void MacAudioEngine::StartPlayback() {
    if (!m_isInitialized || m_isPlaying) return;
    m_isPlaying = true;
    
    // Prime the buffers with silence
    for (int i = 0; i < 3; ++i) {
        FillOutputBuffer(m_outBuffers[i]);
        AudioQueueEnqueueBuffer((AudioQueueRef)m_audioQueueOutput, (AudioQueueBufferRef)m_outBuffers[i], 0, NULL);
    }
    
    AudioQueueStart((AudioQueueRef)m_audioQueueOutput, NULL);
    std::cout << "Audio playback started (CoreAudio)." << std::endl;
}

void MacAudioEngine::StopPlayback() {
    if (!m_isPlaying) return;
    m_isPlaying = false;
    AudioQueueStop((AudioQueueRef)m_audioQueueOutput, true);
    
    std::lock_guard<std::mutex> lock(m_audioMutex);
    while(!m_audioBufferQueue.empty()) m_audioBufferQueue.pop();
    
    std::cout << "Audio playback stopped." << std::endl;
}

void MacAudioEngine::SetPlaybackVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    if (m_audioQueueOutput) {
        AudioQueueSetParameter((AudioQueueRef)m_audioQueueOutput, kAudioQueueParam_Volume, volume);
    }
}
