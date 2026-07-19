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
using asio::ip::tcp;
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
        if (m_participants.size() >= 50) return false;
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
    void initRedis(const std::string& uri, const std::string& myEndpoint) {
        m_redis = std::make_shared<sw::redis::Redis>(uri);
        m_myEndpoint = myEndpoint;
    }
    std::string getMyEndpoint() { return m_myEndpoint; }

private:
    ServerManager() {}
    std::unordered_map<std::string, std::shared_ptr<Room>> m_rooms;
    std::unordered_map<uint32_t, std::shared_ptr<ChatSession>> m_sessions;
    std::set<uint32_t> m_roomListSubscribers;
    std::mutex m_mutex;
    std::shared_ptr<sw::redis::Redis> m_redis;
    std::string m_myEndpoint;
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
        asio::error_code ec;
        m_socket.close(ec);
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
    }

    std::string getUserId() const { 
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_userId; 
    }
    std::string getUserName() const { return m_userName; }
    int getNclr() const { return m_nclr; }
    std::shared_ptr<Room> getRoom() const { 
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_room; 
    }

    uint32_t m_sessionId;
    udp::endpoint m_udpEndpoint;
    bool m_hasUdpEndpoint;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_watching;
    
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
                        if (m_room->getUsers().empty() && m_roomName != "Public") {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->hdel("room_owner", m_roomName);
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
                        if (m_room->getUsers().empty() && m_roomName != "Public") {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->hdel("room_owner", m_roomName);
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

            m_userName = json["userName"].string_value();
            if (m_userName.empty()) m_userName = m_userId; // fallback
            m_nclr = json["nclr"].int_value();
            std::string originalRoomName = json["roomName"].string_value();
            if (originalRoomName.empty()) originalRoomName = "Public";

            if (ServerManager::get().getRedis()) {
                try {
                    ServerManager::get().getRedis()->setex("ip_of:" + m_userId, 86400, getIp());
                } catch (...) {}
            }

            std::string roomName = originalRoomName;
            int suffix = 1;

            while (true) {
                // Room Affinity logic via Redis (Bypass for Lobbies to shard across servers)
                bool isLobby = (roomName.find("\xE3\x83\xAD\xE3\x83\x93\xE3\x83\xBC") == 0); // "ロビー"
                if (!isLobby && roomName != "Public" && ServerManager::get().getRedis()) {
                    auto redis = ServerManager::get().getRedis();
                    try {
                        auto owner = redis->hget("room_owner", roomName);
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
                                    {"port", port}
                                };
                                deliver(redirectMsg.dump() + "\n");
                                return; // DO NOT process join locally
                            }
                        } else {
                            // Claim the room
                            redis->hset("room_owner", roomName, ServerManager::get().getMyEndpoint());
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
                        json11::Json sysMsg = json11::Json::object {
                            {"type", "chat"},
                            {"user", ""},
                            {"msg", originalRoomName + " が満室のため、" + roomName + " に入室しました。"},
                            {"clr", 255}, // Red color to make it visible
                            {"b", false}, {"i", false}, {"u", false},
                            {"sz", 12}
                        };
                        deliver(sysMsg.dump() + "\n");
                    }

                    if (created && roomName != "Public") {
                        // Announce removed for bandwidth optimization
                    }

                    if (m_room && m_room != targetRoom) {
                        m_room->leave(shared_from_this());
                        json11::Json leaveMsg = json11::Json::object {
                            {"type", "leave"},
                            {"userId", oldUserId}
                        };
                        m_room->deliver(leaveMsg.dump() + "\n");
                        if (m_room->getUsers().empty() && m_roomName != "Public") {
                            ServerManager::get().removeRoom(m_roomName);
                            if (ServerManager::get().getRedis()) {
                                try {
                                    ServerManager::get().getRedis()->hdel("room_owner", m_roomName);
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

                    ServerManager::get().notifyRoomListChanged();

                    std::cout << "User " << m_userId << " (Session: " << m_sessionId << ") joined room " << roomName << std::endl;
                    break;
                }

                // Room is full, increment suffix and try again
                suffix++;
                roomName = originalRoomName + ":" + std::to_string(suffix);
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
                json11::Json::object chatObj = json.object_items();
                chatObj["userId"] = m_userId;
                chatObj["userName"] = m_userName;
                chatObj["nclr"] = m_nclr;
                m_room->deliver(json11::Json(chatObj).dump() + "\n", shared_from_this());
            }
        } else if (type == "rename") {
            m_userName = json["userName"].string_value();
            m_nclr = json["nclr"].int_value();
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
            ServerManager::get().notifyRoomListChanged();
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
                
                if (m_room->getUsers().empty() && m_roomName != "Public") {
                    ServerManager::get().removeRoom(m_roomName);
                    if (ServerManager::get().getRedis()) {
                        try {
                            ServerManager::get().getRedis()->hdel("room_owner", m_roomName);
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
                ServerManager::get().notifyRoomListChanged();
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

void ServerManager::notifyRoomListChanged() {
    std::vector<std::shared_ptr<ChatSession>> subs;
    std::string msgStr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_roomListSubscribers.empty()) return;
        std::vector<std::pair<std::string, int>> list;
        for (const auto& pair : m_rooms) {
            if (pair.second->isPrivate()) continue;
            list.push_back({pair.first, (int)pair.second->getUsers().size()});
        }
        json11::Json::array rmArray;
        for (const auto& r : list) {
            rmArray.push_back(json11::Json::object {
                {"name", r.first},
                {"count", r.second}
            });
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
    for (auto& s : subs) {
        s->deliver(msgStr);
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
        if (p != sender && p->m_hasUdpEndpoint) {
            bool shouldSend = true;
            if (payloadType == 2 || payloadType == 5) { // JPEG or H264
                if (!p->isSubscribedToVideo(sender->getUserId())) {
                    shouldSend = false;
                }
            }
            if (shouldSend) {
                // Synchronous send for low latency, zero-heap allocation forwarding
                udpSocket.send_to(asio::buffer(data, length), p->m_udpEndpoint, 0, ec);
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
                        if (!session->m_hasUdpEndpoint || session->m_udpEndpoint != m_remoteEndpoint) {
                            session->m_udpEndpoint = m_remoteEndpoint;
                            session->m_hasUdpEndpoint = true;
                            std::cout << "UDP Endpoint locked for Session " << sessionId << ": " << m_remoteEndpoint << std::endl;
                        }

                        uint8_t payloadType = header->payloadType;
                        if (payloadType == 3 && bytes_recvd > 92) {
                            if (session->m_watching.size() >= 100) {
                                session->m_watching.clear();
                            }
                            std::string targetA(reinterpret_cast<const char*>(m_recvBuffer.data()) + 92, bytes_recvd - 92);
                            session->m_watching[targetA] = std::chrono::steady_clock::now();
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

int main(int argc, char* argv[]) {
    try {
        std::string redis_uri = "tcp://127.0.0.1:6379";
        std::string my_endpoint = "127.0.0.1:4000";
        if (argc > 1) my_endpoint = argv[1];
        if (argc > 2) redis_uri = argv[2];

        std::cout << "Starting SFU Chat Server..." << std::endl;
        std::cout << "Endpoint: " << my_endpoint << std::endl;
        std::cout << "Redis: " << redis_uri << std::endl;
        
        ServerManager::get().initRedis(redis_uri, my_endpoint);

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
        
        asio::io_context tcp_io;
        asio::io_context udp_io;
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
