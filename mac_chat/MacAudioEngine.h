#include <cstdint>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>

class MacAudioEngine {
public:
    MacAudioEngine();
    ~MacAudioEngine();

    bool Initialize();
    void StartMicrophone();
    void StopMicrophone();
    
    void StartPlayback();
    void StopPlayback();
    void SetPlaybackVolume(float volume); // volume: 0.0 to 1.0

    void ProcessIncomingAudio(const uint8_t* data, size_t size);
    void FillOutputBuffer(void* buffer); // Internal

    using AudioDataCallback = std::function<void(const uint8_t* data, size_t size)>;
    using VolumeCallback = std::function<void(int volume)>;
    void SetAudioCallback(AudioDataCallback callback);
    void SetVolumeCallback(VolumeCallback callback);

    void ProcessInputData(const uint8_t* data, size_t size);
    bool IsRecording() const { return m_isRecording; }

private:
    bool m_isInitialized;
    bool m_isRecording;
    bool m_isPlaying;
    AudioDataCallback m_callback;
    VolumeCallback m_volCallback;
    
    std::mutex m_audioMutex;
    std::queue<std::vector<uint8_t>> m_audioBufferQueue;

    // CoreAudio specific state structures (opaque pointers to avoid header pollution)
    void* m_audioQueueInput;
    void* m_audioQueueOutput;
    void* m_inBuffers[3];
    void* m_outBuffers[3];
};
