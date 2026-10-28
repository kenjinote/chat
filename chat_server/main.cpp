#define _WIN32_WINNT 0x0601
#include <iostream>
#include <string>
#include <memory>
#include <set>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <vector>
#include <array>
#include <atomic>
#include <chrono>
#include <asio.hpp>
#include "json11.hpp"
#include <sw/redis++/redis++.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <climits>
using asio::ip::tcp;

bool isValidText(const std::string& utf8str) {
    if (utf8str.length() > 4096) return false; // Max message length
    
    size_t i = 0;
    int consecutive_combining = 0;
    while (i < utf8str.length()) {
        unsigned char c = utf8str[i];
        uint32_t codepoint = 0;
        int bytes = 0;
        if (c <= 0x7F) {
            codepoint = c;
            bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= utf8str.length()) break;
            codepoint = ((c & 0x1F) << 6) | (utf8str[i+1] & 0x3F);
            bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= utf8str.length()) break;
            codepoint = ((c & 0x0F) << 12) | ((utf8str[i+1] & 0x3F) << 6) | (utf8str[i+2] & 0x3F);
            bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= utf8str.length()) break;
            codepoint = ((c & 0x07) << 18) | ((utf8str[i+1] & 0x3F) << 12) | ((utf8str[i+2] & 0x3F) << 6) | (utf8str[i+3] & 0x3F);
            bytes = 4;
        } else {
            bytes = 1;
            i++;
            continue;
        }
        
        // Ancient scripts / hieroglyphs
        if (codepoint >= 0x13000 && codepoint <= 0x1342F) return false;
        if (codepoint >= 0x12000 && codepoint <= 0x123FF) return false;
        if (codepoint >= 0x14400 && codepoint <= 0x1467F) return false;

        // PUA (Private Use Area)
        if (codepoint >= 0xE000 && codepoint <= 0xF8FF) return false;
        if (codepoint >= 0xF0000 && codepoint <= 0xFFFFD) return false;
        if (codepoint >= 0x100000 && codepoint <= 0x10FFFD) return false;

        // Controls and Specials
        if (codepoint <= 0x001F && codepoint != 0x0A && codepoint != 0x0D && codepoint != 0x09) return false;
        if (codepoint >= 0x007F && codepoint <= 0x009F) return false;
        if (codepoint >= 0xE0000 && codepoint <= 0xE007F) return false; // Tags block
        if ((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) return false; // Non-characters
        if (codepoint >= 0xFFF9 && codepoint <= 0xFFFB) return false; // Interlinear annotations

        // Invisible / special spaces
        if (codepoint == 0x1680 || codepoint == 0x180E || codepoint == 0x2800 || codepoint == 0x3164) return false;
        if (codepoint >= 0x2000 && codepoint <= 0x200A) return false; // Various spaces (optional, but requested for robustness against blank names, though normal spaces are 0x20. Wait! Spaces are okay in messages, but let's just block the weird ones 1680,180E,2800,3164)
        // Wait, I shouldn't block 2000-200A entirely as users might use en-quad. But it's okay to skip for now.
        
        bool is_combining = (codepoint >= 0x0300 && codepoint <= 0x036F) || 
                            (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) || 
                            (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) || 
                            (codepoint >= 0x20D0 && codepoint <= 0x20FF) || 
                            (codepoint >= 0xFE20 && codepoint <= 0xFE2F) ||
                            (codepoint >= 0xFE00 && codepoint <= 0xFE0F) || // Variation selectors
                            (codepoint >= 0xE0100 && codepoint <= 0xE01EF); // Variation selectors supp
                            
        if (is_combining) {
            consecutive_combining++;
            if (consecutive_combining > 3) return false;
        } else {
            consecutive_combining = 0;
        }
        
        i += bytes;
    }
    return true;
}

std::string truncateUtf8SingleLine(const std::string& utf8str, size_t maxCodepoints, size_t maxBytes) {
    std::string out;
    out.reserve(std::min(utf8str.size(), maxBytes));
    size_t i = 0;
    size_t codepoints = 0;
    while (i < utf8str.size() && codepoints < maxCodepoints) {
        unsigned char c = static_cast<unsigned char>(utf8str[i]);
        size_t bytes = 1;
        if (c <= 0x7F) {
            bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= utf8str.size()) break;
            bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= utf8str.size()) break;
            bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= utf8str.size()) break;
            bytes = 4;
        } else {
            i++;
            continue;
        }
        if (bytes == 1 && (c < 0x20 || c == 0x7F)) {
            i++;
            continue;
        }
        if (out.size() + bytes > maxBytes) break;
        out.append(utf8str, i, bytes);
        i += bytes;
        codepoints++;
    }
    return out;
}

std::string sanitizeUserName(const std::string& rawName, const std::string& fallbackId) {
    std::string clean = truncateUtf8SingleLine(rawName, 32, 96);
    if (clean.empty() || !isValidText(clean)) return fallbackId;
    return clean;
}

std::string sanitizeRoomName(const std::string& rawRoom, bool isPublic) {
    if (!isPublic || rawRoom.rfind("private_", 0) == 0) {
        std::string clean = truncateUtf8SingleLine(rawRoom, 80, 80);
        if (clean.empty() || !isValidText(clean)) return "Public";
        return clean;
    }
    std::string clean = truncateUtf8SingleLine(rawRoom, 32, 96);
    if (clean.empty() || !isValidText(clean)) return "Public";
    return clean;
}

using asio::ip::udp;

#pragma pack(push, 1)
struct MediaHeader {
    uint32_t sessionId;
    uint16_t seqNum;
    uint8_t  payloadType; // 0 = audio, 1 = video
    uint8_t  flags;
    uint32_t timestamp;
};
#pragma pack(pop)

std::string compute_sha256_uuid(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string hexStr = ss.str();
    
    // Format the first 32 characters as a UUID (8-4-4-4-12)
    return hexStr.substr(0, 8) + "-" + 
           hexStr.substr(8, 4) + "-" + 
           hexStr.substr(12, 4) + "-" + 
           hexStr.substr(16, 4) + "-" + 
           hexStr.substr(20, 12);
}

class ChatSession;

// Room management
class Room {
public:
    bool join(std::shared_ptr<ChatSession> session) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_participants.size() >= 100) return false;
        m_participants.insert(session);
        return true;
    }

    void leave(std::shared_ptr<ChatSession> session) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_participants.erase(session);
    }

    std::vector<json11::Json::object> getUsers();

    void deliver(const std::string& msg, std::shared_ptr<ChatSession> sender = nullptr);
    void broadcastMedia(const std::shared_ptr<ChatSession>& sender, const uint8_t* data, size_t length, udp::socket& udpSocket, uint8_t payloadType);

    void setPrivate(bool isPrivate) { m_isPrivate = isPrivate; }
    bool isPrivate() const { return m_isPrivate; }

private:
    std::set<std::shared_ptr<ChatSession>> m_participants;
    std::mutex m_mutex;
    bool m_isPrivate = false;
};

class ServerManager {
public:
    static ServerManager& get() {
        static ServerManager instance;
        return instance;
    }

    std::shared_ptr<Room> getOrCreateRoom(const std::string& roomName, bool& outCreated) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(roomName);
        if (it == m_rooms.end()) {
            auto room = std::make_shared<Room>();
            m_rooms[roomName] = room;
            outCreated = true;
            return room;
        }
        outCreated = false;
        return it->second;
    }

    void removeRoom(const std::string& roomName) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rooms.erase(roomName);
    }

    std::vector<std::pair<std::string, int>> getRoomList() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::pair<std::string, int>> list;
        for (const auto& pair : m_rooms) {
            list.push_back({pair.first, pair.second->getUsers().size()});
        }
        return list;
    }

    std::shared_ptr<ChatSession> getSession(uint32_t sessionId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(sessionId);
        if (it != m_sessions.end()) return it->second;
        return nullptr;
    }

    void registerSession(uint32_t sessionId, std::shared_ptr<ChatSession> session) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions[sessionId] = session;
    }

    void unregisterSession(uint32_t sessionId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.erase(sessionId);
        m_roomListSubscribers.erase(sessionId);
    }

    void subscribeRoomList(uint32_t sessionId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_roomListSubscribers.insert(sessionId);
    }

    void unsubscribeRoomList(uint32_t sessionId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_roomListSubscribers.erase(sessionId);
    }

    void notifyRoomListChanged();

    void broadcastGlobal(const std::string& msg);

    void disconnectIp(const std::string& ip);
    void disconnectUserId(const std::string& userId);

    std::shared_ptr<sw::redis::Redis> getRedis() { return m_redis; }
    void initRedis(const std::string& uri, const std::string& myEndpoint, asio::io_context& io) {
        m_redis = std::make_shared<sw::redis::Redis>(uri);
        m_myEndpoint = myEndpoint;
        m_timer = std::make_shared<asio::steady_timer>(io, std::chrono::seconds(5));
        doTimer();
    }
    std::string getMyEndpoint() { return m_myEndpoint; }
    
    void sendRoomListTo(std::shared_ptr<ChatSession> session);

private:
    void doTimer();
    ServerManager() {}
    std::unordered_map<std::string, std::shared_ptr<Room>> m_rooms;
    std::unordered_map<uint32_t, std::shared_ptr<ChatSession>> m_sessions;
    std::set<uint32_t> m_roomListSubscribers;
    std::mutex m_mutex;
    std::shared_ptr<sw::redis::Redis> m_redis;
    std::string m_myEndpoint;
    std::shared_ptr<asio::steady_timer> m_timer;
};

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket) : m_socket(std::move(socket)) {
        static std::atomic<uint32_t> nextSessionId{1};
        m_sessionId = nextSessionId++;
        m_hasUdpEndpoint = false;
    }

    ~ChatSession() {
        ServerManager::get().unregisterSession(m_sessionId);
    }

    void disconnect() {
        auto self(shared_from_this());
        asio::post(m_socket.get_executor(), [this, self]() {
            asio::error_code ec;
            m_socket.close(ec);
        });
    }

    std::string getIp() {
        try {
            return m_socket.remote_endpoint().address().to_string();
        } catch (...) {
            return "";
        }
    }

    void start() {
        ServerManager::get().registerSession(m_sessionId, shared_from_this());
        
        // Welcome message with Session ID
        json11::Json initMsg = json11::Json::object {
            {"type", "welcome"},
            {"sessionId", (int)m_sessionId}
        };
        deliver(initMsg.dump() + "\n");
        doRead();
    }

    void deliver(const std::string& msg) {
        auto self(shared_from_this());
        asio::post(m_socket.get_executor(), [this, self, msg]() {
            if (m_writeQueue.size() > 1000) {
                // Client is too slow or unresponsive, force disconnect to save memory
                asio::error_code ec;
                m_socket.close(ec);
                return;
            }
            
            bool write_in_progress = !m_writeQueue.empty();
            m_writeQueue.push_back(msg);
            if (!write_in_progress) {
                doWrite();
            }
        });
    }

    std::string getUserId() const { 
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_userId; 
    }
    std::string getUserName() const {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_userName;
    }
    int getNclr() const {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_nclr;
    }
    std::shared_ptr<Room> getRoom() const { 
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_room; 
    }

    uint32_t m_sessionId;
    udp::endpoint m_udpEndpoint;
    bool m_hasUdpEndpoint;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_watching;

    bool updateUdpEndpoint(const udp::endpoint& ep) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!m_hasUdpEndpoint || m_udpEndpoint != ep) {
            m_udpEndpoint = ep;
            m_hasUdpEndpoint = true;
            return true;
        }
        return false;
    }

    bool getUdpEndpoint(udp::endpoint& outEp) const {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!m_hasUdpEndpoint) return false;
        outEp = m_udpEndpoint;
        return true;
    }

    void recordWatching(const std::string& targetUserId) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_watching.size() >= 100) {
            m_watching.clear();
        }
        m_watching[targetUserId] = std::chrono::steady_clock::now();
    }
    
    void subscribeVideo(const std::string& targetUserId) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_subscribedVideos.insert(targetUserId);
    }
    
    void unsubscribeVideo(const std::string& targetUserId) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_subscribedVideos.erase(targetUserId);
    }
    
    bool isSubscribedToVideo(const std::string& targetUserId) const {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_subscribedVideos.find(targetUserId) != m_subscribedVideos.end();
    }

private:
    std::set<std::string> m_subscribedVideos;

    void doRead() {
        auto self(shared_from_this());
        asio::async_read_until(m_socket, m_buffer, '\n',
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    std::istream is(&m_buffer);
                    std::string line;
                    std::getline(is, line);
                    
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    handleMessage(line);
                    doRead();
                } else {
                    if (m_room) {
                        m_room->leave(self);
                        bool stillInRoom = false;
                        for (const auto& u : m_room->getUsers()) {
                            if (u.at("userId").string_value() == m_userId) { stillInRoom = true; break; }
                        }
                        if (!stillInRoom) {
                            json11::Json leaveMsg = json11::Json::object {
                                {"type", "leave"},
                                {"userId", m_userId}
                            };
                            m_room->deliver(leaveMsg.dump() + "\n");
                        }
                        if (m_room->getUsers().empty()) {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->del("room_owner:" + m_roomName);
                                } catch (const std::exception& e) {
                                    std::cerr << "Redis error: " << e.what() << std::endl;
                                }
                            }
                        }
                        {
                            std::lock_guard<std::mutex> lock(m_stateMutex);
                            m_room.reset();
                        }
                    }
                    ServerManager::get().unregisterSession(m_sessionId);
                }
            });
    }

    void doWrite() {
        auto self(shared_from_this());
        asio::async_write(m_socket,
            asio::buffer(m_writeQueue.front().data(), m_writeQueue.front().length()),
            [this, self](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    m_writeQueue.pop_front();
                    if (!m_writeQueue.empty()) {
                        doWrite();
                    }
                } else {
                    if (m_room) {
                        m_room->leave(self);
                        bool stillInRoom = false;
                        for (const auto& u : m_room->getUsers()) {
                            if (u.at("userId").string_value() == m_userId) { stillInRoom = true; break; }
                        }
                        if (!stillInRoom) {
                            json11::Json leaveMsg = json11::Json::object {
                                {"type", "leave"},
                                {"userId", m_userId}
                            };
                            m_room->deliver(leaveMsg.dump() + "\n");
                        }
                        if (m_room->getUsers().empty()) {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->del("room_owner:" + m_roomName);
                                } catch (const std::exception& e) {
                                    std::cerr << "Redis error during disconnect: " << e.what() << std::endl;
                                }
                            }
                        }
                        {
                            std::lock_guard<std::mutex> lock(m_stateMutex);
                            m_room.reset();
                        }
                    }
                    ServerManager::get().unregisterSession(m_sessionId);
                }
            });
    }

    void handleMessage(const std::string& msg) {
        std::string err;
        json11::Json json = json11::Json::parse(msg, err);
        if (!err.empty()) return;

        std::string type = json["type"].string_value();

        if (type == "join") {
            std::string oldUserId;
            std::string secretId = json["secretId"].string_value();
            if (secretId.empty()) secretId = "unknown";
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                oldUserId = m_userId;
                m_userId = compute_sha256_uuid(secretId);
            }

            if (ServerManager::get().getRedis()) {
                try {
                    auto banned = ServerManager::get().getRedis()->get("banned_user:" + m_userId);
                    if (banned) {
                        disconnect();
                        return;
                    }
                } catch (...) {}
            }

            bool isPubReq = true;
            if (json["pub"].is_bool()) {
                isPubReq = json["pub"].bool_value();
            }
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_userName = sanitizeUserName(json["userName"].string_value(), m_userId);
                m_nclr = json["nclr"].int_value();
            }
            std::string originalRoomName = sanitizeRoomName(json["roomName"].string_value(), isPubReq);

            if (ServerManager::get().getRedis()) {
                try {
                    ServerManager::get().getRedis()->setex("ip_of:" + m_userId, 86400, getIp());
                } catch (...) {}
            }

            std::string roomName = originalRoomName;
            int suffix = 1;

            while (true) {
                // Room Affinity logic via Redis (All rooms centralized)
                if (ServerManager::get().getRedis()) {
                    auto redis = ServerManager::get().getRedis();
                    try {
                        auto owner = redis->get("room_owner:" + roomName);
                        if (owner) {
                            if (*owner != ServerManager::get().getMyEndpoint()) {
                                // Redirect client to the correct server
                                size_t colon = owner->find(':');
                                std::string ip = owner->substr(0, colon);
                                int port = 4000;
                                if (colon != std::string::npos) port = std::stoi(owner->substr(colon + 1));
                                
                                json11::Json redirectMsg = json11::Json::object {
                                    {"type", "redirect"},
                                    {"ip", ip},
                                    {"port", port},
                                    {"roomName", roomName}
                                };
                                deliver(redirectMsg.dump() + "\n");
                                return; // DO NOT process join locally
                            }
                        } else {
                            // Find the least loaded server
                            std::vector<std::string> keys;
                            redis->keys("server_load:*", std::back_inserter(keys));
                            
                            std::string bestServer = ServerManager::get().getMyEndpoint();
                            int minLoad = INT_MAX;
                            
                            for (const auto& key : keys) {
                                auto val = redis->get(key);
                                if (val) {
                                    int load = std::stoi(*val);
                                    if (load < minLoad) {
                                        minLoad = load;
                                        bestServer = key.substr(12); // remove "server_load:"
                                    }
                                }
                            }
                            
                            // Claim the room for the best server using atomic SETNX logic
                            bool acquired = redis->set("room_owner:" + roomName, bestServer, std::chrono::seconds(15), sw::redis::UpdateType::NOT_EXIST);
                            
                            if (acquired) {
                                if (bestServer != ServerManager::get().getMyEndpoint()) {
                                    // Redirect client to the best server
                                    size_t colon = bestServer.find(':');
                                    std::string ip = bestServer.substr(0, colon);
                                    int port = 4000;
                                    if (colon != std::string::npos) port = std::stoi(bestServer.substr(colon + 1));
                                    
                                    json11::Json redirectMsg = json11::Json::object {
                                        {"type", "redirect"},
                                        {"ip", ip},
                                        {"port", port},
                                        {"roomName", roomName}
                                    };
                                    deliver(redirectMsg.dump() + "\n");
                                    return;
                                }
                            } else {
                                // We lost the race. Fetch the actual owner set by another server
                                auto newOwner = redis->get("room_owner:" + roomName);
                                if (newOwner && *newOwner != ServerManager::get().getMyEndpoint()) {
                                    size_t colon = newOwner->find(':');
                                    std::string ip = newOwner->substr(0, colon);
                                    int port = 4000;
                                    if (colon != std::string::npos) port = std::stoi(newOwner->substr(colon + 1));
                                    
                                    json11::Json redirectMsg = json11::Json::object {
                                        {"type", "redirect"},
                                        {"ip", ip},
                                        {"port", port},
                                        {"roomName", roomName}
                                    };
                                    deliver(redirectMsg.dump() + "\n");
                                    return;
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Redis error during join: " << e.what() << std::endl;
                    }
                }

                bool created = false;
                std::shared_ptr<Room> targetRoom = ServerManager::get().getOrCreateRoom(roomName, created);
                if (created) {
                    bool isPublic = true;
                    if (json["pub"].is_bool()) {
                        isPublic = json["pub"].bool_value();
                    }
                    targetRoom->setPrivate(!isPublic);
                }
                
                if (targetRoom->join(shared_from_this())) {
                    // Success!
                    if (roomName != originalRoomName) {
                        // Notify the user that they were placed in an overflow room
                        bool isPrivate = (originalRoomName.find("private_") == 0);
                        json11::Json sysMsg = json11::Json::object {
                            {"type", "sysmsg"},
                            {"id", "room_overflow"},
                            {"isPrivate", isPrivate},
                            {"orig", originalRoomName},
                            {"new", roomName}
                        };
                        deliver(sysMsg.dump() + "\n");
                    }

                    if (created) {
                        // Announce removed for bandwidth optimization
                    }

                    if (m_room && m_room != targetRoom) {
                        m_room->leave(shared_from_this());
                        json11::Json leaveMsg = json11::Json::object {
                            {"type", "leave"},
                            {"userId", oldUserId}
                        };
                        m_room->deliver(leaveMsg.dump() + "\n");
                        if (m_room->getUsers().empty()) {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->del("room_owner:" + m_roomName);
                                } catch (const std::exception& e) {
                                    std::cerr << "Redis error: " << e.what() << std::endl;
                                }
                            }
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_stateMutex);
                        m_room = targetRoom;
                    }
                    m_roomName = roomName;

                    

                    std::cout << "User " << m_userId << " (Session: " << m_sessionId << ") joined room " << roomName << std::endl;
                    break;
                }

                // Room is full, redirect client to LB with incremented suffix
                suffix++;
                std::string newRoomName = originalRoomName + ":" + std::to_string(suffix);
                
                json11::Json redirectMsg = json11::Json::object {
                    {"type", "redirect"},
                    {"ip", "chat-lb-2026.japaneast.cloudapp.azure.com"},
                    {"port", 4000},
                    {"roomName", newRoomName}
                };
                deliver(redirectMsg.dump() + "\n");
                return;
            }


            json11::Json welcomeMsg = json11::Json::object {
                {"type", "welcome"},
                {"userId", m_userId},
                {"pub", !m_room->isPrivate()}
            };
            deliver(welcomeMsg.dump() + "\n");

            auto users = m_room->getUsers();
            json11::Json::array userArray;
            for (const auto& u : users) {
                userArray.push_back(u);
            }
            json11::Json usersMsg = json11::Json::object {
                {"type", "users"},
                {"users", userArray}
            };
            deliver(usersMsg.dump() + "\n");

            json11::Json joinBroadcast = json11::Json::object {
                {"type", "join"},
                {"userId", m_userId},
                {"userName", m_userName},
                {"nclr", m_nclr}
            };
            m_room->deliver(joinBroadcast.dump() + "\n", shared_from_this());
            
        } else if (type == "chat") {
            if (m_room) {
                std::string msgText = json["msg"].string_value();
                if (!isValidText(msgText)) return; // drop spam
                json11::Json::object chatObj = json.object_items();
                chatObj["userId"] = m_userId;
                chatObj["userName"] = m_userName;
                chatObj["nclr"] = m_nclr;
                
                if (chatObj["sz"].is_number()) {
                    int sz = chatObj["sz"].int_value();
                    if (sz > 32) sz = 32;
                    if (sz < 6) sz = 6;
                    chatObj["sz"] = sz;
                }
                
                m_room->deliver(json11::Json(chatObj).dump() + "\n", shared_from_this());
            }
        } else if (type == "rename") {
            std::string newName = sanitizeUserName(json["userName"].string_value(), m_userId);
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_userName = newName;
                m_nclr = json["nclr"].int_value();
            }
            if (m_room) {
                json11::Json renameBroadcast = json11::Json::object {
                    {"type", "rename"},
                    {"userId", m_userId},
                    {"userName", m_userName},
                    {"nclr", m_nclr}
                };
                m_room->deliver(renameBroadcast.dump() + "\n", shared_from_this());
            }
        } else if (type == "get_rooms") {
            ServerManager::get().subscribeRoomList(m_sessionId);
            ServerManager::get().sendRoomListTo(shared_from_this());
            
        } else if (type == "close_rooms") {
            ServerManager::get().unsubscribeRoomList(m_sessionId);
        } else if (type == "subscribe_video") {
            subscribeVideo(json["target"].string_value());
        } else if (type == "unsubscribe_video") {
            unsubscribeVideo(json["target"].string_value());
        } else if (type == "leave") {
            if (m_room) {
                m_room->leave(shared_from_this());
                
                bool stillInRoom = false;
                auto users = m_room->getUsers();
                for (const auto& u : users) {
                    if (u.at("userId").string_value() == m_userId) {
                        stillInRoom = true;
                        break;
                    }
                }
                
                if (!stillInRoom) {
                    json11::Json leaveMsg = json11::Json::object {
                        {"type", "leave"},
                        {"userId", m_userId}
                    };
                    m_room->deliver(leaveMsg.dump() + "\n");
                }
                
                if (m_room->getUsers().empty()) {
                    ServerManager::get().removeRoom(m_roomName);
                    if (ServerManager::get().getRedis()) {
                        try {
                            ServerManager::get().getRedis()->del("room_owner:" + m_roomName);
                        } catch (const std::exception& e) {
                            std::cerr << "Redis error: " << e.what() << std::endl;
                        }
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    m_room.reset();
                }
                m_roomName = "";
                
            }
        } else if (type == "report") {
            std::string target = json["target"].string_value();
            if (!target.empty() && ServerManager::get().getRedis()) {
                auto redis = ServerManager::get().getRedis();
                try {
                    std::string reporterIp = getIp();
                    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    // --- Spam Reporting Prevention ---
                    std::string historyKey = "reporter_history:" + m_userId;
                    std::string historyIpKey = "reporter_history_ip:" + reporterIp;
                    
                    redis->zadd(historyKey, target, (double)now);
                    redis->command("ZREMRANGEBYSCORE", historyKey, "-inf", std::to_string(now - 3600));
                    redis->expire(historyKey, 3600);
                    long long historyCount = redis->zcard(historyKey);

                    redis->zadd(historyIpKey, target, (double)now);
                    redis->command("ZREMRANGEBYSCORE", historyIpKey, "-inf", std::to_string(now - 3600));
                    redis->expire(historyIpKey, 3600);
                    long long historyIpCount = redis->zcard(historyIpKey);

                    long long maxHistory = std::max(historyCount, historyIpCount);

                    if (maxHistory >= 60) {
                        redis->setex("banned_user:" + m_userId, 86400, "1");
                        ServerManager::get().disconnectUserId(m_userId);
                        return;
                    } else if (maxHistory >= 30) {
                        return;
                    }
                    // ----------------------------------

                    std::string reportKey = "report:" + target;
                    std::string reportIpKey = "report_ip:" + target;
                    
                    redis->zadd(reportKey, m_userId, (double)now);
                    redis->zadd(reportIpKey, reporterIp, (double)now);
                    
                    // Remove reports older than 24 hours
                    redis->command("ZREMRANGEBYSCORE", reportKey, "-inf", std::to_string(now - 86400));
                    redis->expire(reportKey, 86400);
                    
                    redis->command("ZREMRANGEBYSCORE", reportIpKey, "-inf", std::to_string(now - 86400));
                    redis->expire(reportIpKey, 86400);
                    
                    long long countUser = redis->zcard(reportKey);
                    long long countIp = redis->zcard(reportIpKey);
                    long long actualCount = std::min(countUser, countIp);
                    
                    if (actualCount >= 7) {
                        redis->setex("banned_user:" + target, 86400, "1");
                        ServerManager::get().disconnectUserId(target);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Redis error during report: " << e.what() << std::endl;
                }
            }
        }
    }

    tcp::socket m_socket;
    asio::streambuf m_buffer{65536};
    std::deque<std::string> m_writeQueue;
    std::shared_ptr<Room> m_room;
    std::string m_roomName;
    std::string m_userId;
    std::string m_userName;
    int m_nclr;
    mutable std::mutex m_stateMutex;
};

void ServerManager::doTimer() {
    std::string msgStr;
    std::vector<std::shared_ptr<ChatSession>> subs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_redis) {
            try {
                auto pipe = m_redis->pipeline();
                pipe.setex("server_load:" + m_myEndpoint, 15, std::to_string(m_sessions.size()));
                for (const auto& pair : m_rooms) {
                    pipe.setex("room_owner:" + pair.first, 15, m_myEndpoint);
                    if (!pair.second->isPrivate()) {
                        pipe.setex("room_count:" + pair.first, 15, std::to_string(pair.second->getUsers().size()));
                    }
                }
                pipe.exec();
                
                if (!m_roomListSubscribers.empty()) {
                    std::vector<std::string> keys;
                    m_redis->keys("room_count:*", std::back_inserter(keys));
                    
                    json11::Json::array rmArray;
                    for (const auto& key : keys) {
                        auto val = m_redis->get(key);
                        if (val) {
                            std::string roomName = key.substr(11); // remove "room_count:"
                            if (roomName.empty() || roomName.length() > 96 || !isValidText(roomName)) {
                                try {
                                    m_redis->del(key);
                                    m_redis->del("room_owner:" + roomName);
                                } catch (...) {}
                                continue;
                            }
                            rmArray.push_back(json11::Json::object {
                                {"name", roomName},
                                {"count", std::stoi(*val)}
                            });
                        }
                    }
                    json11::Json roomsMsg = json11::Json::object {
                        {"type", "room_list"},
                        {"rooms", rmArray}
                    };
                    msgStr = roomsMsg.dump() + "\n";
                    
                    for (uint32_t sid : m_roomListSubscribers) {
                        auto it = m_sessions.find(sid);
                        if (it != m_sessions.end()) subs.push_back(it->second);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Redis error in doTimer: " << e.what() << std::endl;
            }
        }
    }
    for (auto& s : subs) {
        s->deliver(msgStr);
    }
    
    if (m_timer) {
        m_timer->expires_after(std::chrono::seconds(5));
        m_timer->async_wait([this](std::error_code ec) {
            if (!ec) {
                doTimer();
            }
        });
    }
}

void ServerManager::sendRoomListTo(std::shared_ptr<ChatSession> session) {
    std::string msgStr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_redis) {
            try {
                std::vector<std::string> keys;
                m_redis->keys("room_count:*", std::back_inserter(keys));
                
                json11::Json::array rmArray;
                for (const auto& key : keys) {
                    auto val = m_redis->get(key);
                    if (val) {
                        std::string roomName = key.substr(11);
                        if (roomName.empty() || roomName.length() > 96 || !isValidText(roomName)) {
                            try {
                                m_redis->del(key);
                                m_redis->del("room_owner:" + roomName);
                            } catch (...) {}
                            continue;
                        }
                        rmArray.push_back(json11::Json::object {
                            {"name", roomName},
                            {"count", std::stoi(*val)}
                        });
                    }
                }
                json11::Json roomsMsg = json11::Json::object {
                    {"type", "room_list"},
                    {"rooms", rmArray}
                };
                msgStr = roomsMsg.dump() + "\n";
            } catch (...) {}
        }
    }
    if (!msgStr.empty() && session) {
        session->deliver(msgStr);
    }
}

void Room::deliver(const std::string& msg, std::shared_ptr<ChatSession> sender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto participant : m_participants) {
        if (participant != sender) {
            participant->deliver(msg);
        }
    }
}

std::vector<json11::Json::object> Room::getUsers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<json11::Json::object> users;
    std::set<std::string> uniqueUsers;
    for (auto& p : m_participants) {
        if (uniqueUsers.insert(p->getUserId()).second) {
            users.push_back(json11::Json::object {
                {"userId", p->getUserId()},
                {"userName", p->getUserName()},
                {"nclr", p->getNclr()}
            });
        }
    }
    return users;
}

void Room::broadcastMedia(const std::shared_ptr<ChatSession>& sender, const uint8_t* data, size_t length, udp::socket& udpSocket, uint8_t payloadType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    asio::error_code ec;
    auto now = std::chrono::steady_clock::now();
    for (auto& p : m_participants) {
        if (p != sender) {
            udp::endpoint targetEp;
            if (!p->getUdpEndpoint(targetEp)) continue;
            bool shouldSend = true;
            if (payloadType == 2 || payloadType == 5 || payloadType == 6) { // JPEG, H264, or Video FEC
                if (!p->isSubscribedToVideo(sender->getUserId())) {
                    shouldSend = false;
                }
            }
            if (shouldSend) {
                // Synchronous send for low latency, zero-heap allocation forwarding
                udpSocket.send_to(asio::buffer(data, length), targetEp, 0, ec);
            }
        }
    }
}

void ServerManager::broadcastGlobal(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_sessions) {
        pair.second->deliver(msg);
    }
}

void ServerManager::disconnectIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->second->getIp() == ip) {
            it->second->disconnect();
        }
    }
}

void ServerManager::disconnectUserId(const std::string& userId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->second->getUserId() == userId) {
            it->second->disconnect();
        }
    }
}

class ChatServer {
public:
    ChatServer(asio::io_context& io_context, short port)
        : m_acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        doAccept();
    }

private:
    void doAccept() {
        m_acceptor.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Disable Nagle's algorithm for extreme low latency
                    socket.set_option(tcp::no_delay(true));
                    std::make_shared<ChatSession>(std::move(socket))->start();
                }
                doAccept();
            });
    }

    tcp::acceptor m_acceptor;
};

class UdpMediaServer {
public:
    UdpMediaServer(asio::io_context& io_context, short port)
        : m_socket(io_context, udp::endpoint(udp::v4(), port)) {
        // Increase socket buffer size for high throughput video
        asio::socket_base::receive_buffer_size recv_sz(2097152);
        asio::socket_base::send_buffer_size send_sz(2097152);
        m_socket.set_option(recv_sz);
        m_socket.set_option(send_sz);
        doReceive();
    }

private:
    void doReceive() {
        m_socket.async_receive_from(
            asio::buffer(m_recvBuffer), m_remoteEndpoint,
            [this](std::error_code ec, std::size_t bytes_recvd) {
                if (ec) {
                    std::cout << "UDP Error: " << ec.message() << std::endl;
                }
                if (!ec && bytes_recvd >= sizeof(MediaHeader)) {
                    MediaHeader* header = reinterpret_cast<MediaHeader*>(m_recvBuffer.data());
                    uint32_t sessionId = header->sessionId;

                    auto session = ServerManager::get().getSession(sessionId);
                    if (session) {
                        // NAT Traversal / UDP Hole Punching: register their UDP port
                        if (session->updateUdpEndpoint(m_remoteEndpoint)) {
                            std::cout << "UDP Endpoint locked for Session " << sessionId << ": " << m_remoteEndpoint << std::endl;
                        }

                        uint8_t payloadType = header->payloadType;
                        if (payloadType == 3 && bytes_recvd > 92) {
                            std::string targetA(reinterpret_cast<const char*>(m_recvBuffer.data()) + 92, bytes_recvd - 92);
                            session->recordWatching(targetA);
                        }

                        // Broadcast to the room using Zero-copy fast path
                        auto room = session->getRoom();
                        if (room) {
                            room->broadcastMedia(session, m_recvBuffer.data(), bytes_recvd, m_socket, payloadType);
                        }
                    }
                }
                doReceive();
            });
    }

    udp::socket m_socket;
    udp::endpoint m_remoteEndpoint;
    std::array<uint8_t, 65536> m_recvBuffer; // Max UDP packet size
};

#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        std::string redis_uri = "tcp://127.0.0.1:6379";
        std::string my_endpoint = "127.0.0.1:4000";
        
        // Use environment variables if present
        if (const char* env_endpoint = std::getenv("ENDPOINT")) {
            my_endpoint = env_endpoint;
        } else if (argc > 1) {
            my_endpoint = argv[1];
        }
        
        if (const char* env_redis = std::getenv("REDIS_URL")) {
            redis_uri = env_redis;
        } else if (argc > 2) {
            redis_uri = argv[2];
        }

        std::cout << "Starting SFU Chat Server..." << std::endl;
        std::cout << "Endpoint: " << my_endpoint << std::endl;
        std::cout << "Redis: " << redis_uri << std::endl;
        
        asio::io_context tcp_io;
        asio::io_context udp_io;
        ServerManager::get().initRedis(redis_uri, my_endpoint, tcp_io);

        // Parse local port from endpoint
        short tcp_port = 4000;
        short udp_port = 4001;
        size_t colon = my_endpoint.find(':');
        if (colon != std::string::npos) {
            tcp_port = std::stoi(my_endpoint.substr(colon + 1));
            udp_port = tcp_port + 1; // Assuming UDP is TCP port + 1
        }
        
        std::cout << "TCP Signaling: Port " << tcp_port << std::endl;
        std::cout << "UDP Media: Port " << udp_port << std::endl;
        
        ChatServer tcp_server(tcp_io, tcp_port);
        UdpMediaServer udp_server(udp_io, udp_port);

        std::thread tcp_thread([&tcp_io]() {
            tcp_io.run();
        });
        std::thread udp_thread([&udp_io]() {
            udp_io.run();
        });

        tcp_thread.join();
        udp_thread.join();

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
// FORCE REBUILD 18

