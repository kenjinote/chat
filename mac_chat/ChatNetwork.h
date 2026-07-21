#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <netinet/in.h>
#include <map>

#pragma pack(push, 1)
struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t payloadType; // 1 = Audio, 2 = Video, 255 = HolePunch
    uint8_t flags;
    uint32_t timestamp;
    char16_t userId[40]; // 16-bit to match Windows wchar_t
};
#pragma pack(pop)

class ChatNetwork {
public:
    ChatNetwork();
    ~ChatNetwork();

    bool Connect(const std::string& host, int tcpPort, int udpPort);
    void Disconnect();
    bool IsConnected() const;
    bool SendMessage(const std::string& msg);
    
    // New function for sending Audio via UDP
    bool SendAudioData(const uint8_t* data, size_t size);
    
    // New function for sending Video via UDP
    bool SendVideoData(const uint8_t* data, size_t size, bool isH264);

    // Callback function type for received messages
    using MessageCallback = std::function<void(const std::string&)>;
    void SetMessageCallback(MessageCallback callback);

    using AudioReceiveCallback = std::function<void(const std::u16string& userId, const uint8_t* data, size_t size)>;
    using VideoReceiveCallback = std::function<void(const std::u16string& userId, const uint8_t* data, size_t size, bool isH264)>;
    void SetAudioReceiveCallback(AudioReceiveCallback callback);
    void SetVideoReceiveCallback(VideoReceiveCallback callback);

private:
    void ReceiveLoop();
    void ReceiveUdpLoop();

    int m_tcpSocket;
    int m_udpSocket;
    
    // Store UDP server address for sendto
    struct sockaddr_in m_serverUdpAddr;

    MessageCallback m_callback;
    AudioReceiveCallback m_audioCallback;
    VideoReceiveCallback m_videoCallback;
    std::thread m_receiveThread;
    std::thread m_receiveUdpThread;
    std::atomic<bool> m_isRunning;
    
    uint16_t m_audioSeqNum;
    uint16_t m_videoSeqNum;

    struct VideoBuffer {
        uint32_t lastUpdate;
        int totalFragments;
        std::map<int, std::vector<uint8_t>> fragments;
        VideoBuffer() : lastUpdate(0), totalFragments(-1) {}
    };
    std::map<std::u16string, std::map<uint16_t, VideoBuffer>> m_videoBuffers;
};
