#include "ChatNetwork.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>

ChatNetwork::ChatNetwork() : m_tcpSocket(-1), m_udpSocket(-1), m_callback(nullptr), m_audioCallback(nullptr), m_videoCallback(nullptr), m_isRunning(false), m_audioSeqNum(0), m_videoSeqNum(0) {
    memset(&m_serverUdpAddr, 0, sizeof(m_serverUdpAddr));
}

ChatNetwork::~ChatNetwork() {
    Disconnect();
}

bool ChatNetwork::Connect(const std::string& host, int tcpPort, int udpPort) {
    if (m_tcpSocket >= 0) Disconnect();

    // TCP Setup
    m_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcpSocket < 0) return false;

    // UDP Setup
    m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udpSocket < 0) return false;

    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) return false;

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    std::copy((char*)server->h_addr, (char*)server->h_addr + server->h_length, (char*)&serv_addr.sin_addr.s_addr);
    serv_addr.sin_port = htons(tcpPort);

    if (connect(m_tcpSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return false;
    }

    // Set UDP address for sending media
    m_serverUdpAddr.sin_family = AF_INET;
    std::copy((char*)server->h_addr, (char*)server->h_addr + server->h_length, (char*)&m_serverUdpAddr.sin_addr.s_addr);
    m_serverUdpAddr.sin_port = htons(udpPort);

    m_isRunning = true;
    m_receiveThread = std::thread(&ChatNetwork::ReceiveLoop, this);
    m_receiveUdpThread = std::thread(&ChatNetwork::ReceiveUdpLoop, this);

    std::cout << "Connected to TCP:" << tcpPort << " and initialized UDP:" << udpPort << std::endl;
    return true;
}

void ChatNetwork::Disconnect() {
    m_isRunning = false;
    if (m_tcpSocket >= 0) {
        close(m_tcpSocket);
        m_tcpSocket = -1;
    }
    if (m_udpSocket >= 0) {
        close(m_udpSocket);
        m_udpSocket = -1;
    }
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
    if (m_receiveUdpThread.joinable()) {
        m_receiveUdpThread.join();
    }
}

bool ChatNetwork::IsConnected() const {
    return m_isRunning && m_tcpSocket >= 0;
}

bool ChatNetwork::SendMessage(const std::string& msg) {
    if (m_tcpSocket < 0) return false;
    ssize_t sent = send(m_tcpSocket, msg.c_str(), msg.length(), 0);
    return sent == msg.length();
}

bool ChatNetwork::SendAudioData(const uint8_t* data, size_t size) {
    if (m_udpSocket < 0) return false; // Not connected yet
    
    // Construct packet
    MediaHeader hdr = {0};
    hdr.sessionId = 0; 
    hdr.seqNum = m_audioSeqNum++;
    hdr.payloadType = 1; // 1 = Audio
    hdr.flags = 0;
    
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    hdr.timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    extern std::string g_userId;
    for (size_t i = 0; i < 16; ++i) {
        if (i < g_userId.length()) hdr.userId[i] = g_userId[i];
        else hdr.userId[i] = 0;
    }
    
    std::vector<uint8_t> packet(sizeof(MediaHeader) + size);
    memcpy(packet.data(), &hdr, sizeof(MediaHeader));
    memcpy(packet.data() + sizeof(MediaHeader), data, size);
    
    ssize_t sent = sendto(m_udpSocket, packet.data(), packet.size(), 0, 
                          (struct sockaddr*)&m_serverUdpAddr, sizeof(m_serverUdpAddr));
                          
    return sent == packet.size();
}

bool ChatNetwork::SendVideoData(const uint8_t* data, size_t size, bool isH264) {
    if (m_udpSocket < 0) return false; 
    
    MediaHeader hdr = {0};
    hdr.sessionId = 0; 
    hdr.seqNum = m_videoSeqNum++;
    hdr.payloadType = isH264 ? 5 : 2; // 5 = H264, 2 = JPEG
    hdr.flags = 0;
    
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    hdr.timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    extern std::string g_userId;
    for (size_t i = 0; i < 16; ++i) {
        if (i < g_userId.length()) hdr.userId[i] = g_userId[i];
        else hdr.userId[i] = 0;
    }
    
    std::vector<uint8_t> packet(sizeof(MediaHeader) + size);
    memcpy(packet.data(), &hdr, sizeof(MediaHeader));
    memcpy(packet.data() + sizeof(MediaHeader), data, size);
    
    ssize_t sent = sendto(m_udpSocket, packet.data(), packet.size(), 0, 
                          (struct sockaddr*)&m_serverUdpAddr, sizeof(m_serverUdpAddr));
                          
    return sent == packet.size();
}

void ChatNetwork::SetMessageCallback(MessageCallback callback) {
    m_callback = callback;
}

void ChatNetwork::SetAudioReceiveCallback(AudioReceiveCallback callback) {
    m_audioCallback = callback;
}

void ChatNetwork::SetVideoReceiveCallback(VideoReceiveCallback callback) {
    m_videoCallback = callback;
}

void ChatNetwork::ReceiveLoop() {
    std::vector<char> buffer(4096);
    std::string tcpAccum;
    while (m_isRunning && m_tcpSocket >= 0) {
        ssize_t bytesRead = recv(m_tcpSocket, buffer.data(), buffer.size() - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            tcpAccum.append(buffer.data(), bytesRead);
            
            size_t pos;
            while ((pos = tcpAccum.find('\n')) != std::string::npos) {
                std::string line = tcpAccum.substr(0, pos);
                tcpAccum.erase(0, pos + 1);
                
                if (m_callback) {
                    m_callback(line + "\n");
                }
            }
        } else if (bytesRead == 0) {
            break;
        } else {
            break;
        }
    }
    m_isRunning = false;
}

void ChatNetwork::ReceiveUdpLoop() {
    std::vector<uint8_t> buffer(65535);
    while (m_isRunning && m_udpSocket >= 0) {
        struct sockaddr_in src_addr;
        socklen_t src_len = sizeof(src_addr);
        ssize_t bytesRead = recvfrom(m_udpSocket, buffer.data(), buffer.size(), 0, (struct sockaddr*)&src_addr, &src_len);
        
        if (bytesRead > 0 && bytesRead >= sizeof(MediaHeader)) {
            MediaHeader* mhdr = (MediaHeader*)buffer.data();
            size_t payloadSize = bytesRead - sizeof(MediaHeader);
            const uint8_t* payload = buffer.data() + sizeof(MediaHeader);
            
            if (mhdr->payloadType == 1 && m_audioCallback) {
                std::u16string userId(mhdr->userId);
                m_audioCallback(userId, payload, payloadSize);
            } else if ((mhdr->payloadType == 2 || mhdr->payloadType == 5) && m_videoCallback) {
                std::u16string userId(mhdr->userId);
                uint16_t seq = mhdr->seqNum;
                int fragIdx = mhdr->timestamp;
                bool isH264 = (mhdr->payloadType == 5);
                
                auto& vbuf = m_videoBuffers[userId][seq];
                vbuf.lastUpdate = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                vbuf.fragments[fragIdx].assign(payload, payload + payloadSize);
                
                if (mhdr->flags & 2) {
                    vbuf.totalFragments = fragIdx + 1;
                }
                
                if (vbuf.totalFragments > 0 && vbuf.fragments.size() == (size_t)vbuf.totalFragments) {
                    std::vector<uint8_t> frameData;
                    for (int i = 0; i < vbuf.totalFragments; i++) {
                        frameData.insert(frameData.end(), vbuf.fragments[i].begin(), vbuf.fragments[i].end());
                    }
                    m_videoCallback(userId, frameData.data(), frameData.size(), isH264);
                    m_videoBuffers[userId].erase(seq);
                }
            }
            
            // Simple cleanup
            static uint32_t lastCleanup = 0;
            uint32_t now = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            if (now - lastCleanup > 1000) {
                lastCleanup = now;
                for (auto& userPair : m_videoBuffers) {
                    for (auto it = userPair.second.begin(); it != userPair.second.end(); ) {
                        if (now - it->second.lastUpdate > 2000) {
                            it = userPair.second.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        } else if (bytesRead <= 0) {
            usleep(10000); 
        }
    }
}
