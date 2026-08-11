package com.example.chat

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.example.chat.media.AudioEngine
import com.example.chat.media.VideoEngine
import com.example.chat.network.ChatMessage
import com.example.chat.network.MediaHeader
import com.example.chat.network.NetworkClient
import com.example.chat.utils.ModerationManager
import com.example.chat.utils.TextProcessor
import com.example.chat.R
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.serialization.json.Json
import java.util.UUID

data class UserItem(val userId: String, val userName: String, val nclr: Int = -1)
data class RoomItem(val name: String, val count: Int)

class ChatViewModel(application: Application) : AndroidViewModel(application) {
    private fun fixString(s: String?): String? {
        if (s == null) return null
        return try {
            val latin1Bytes = s.toByteArray(java.nio.charset.Charset.forName("ISO-8859-1"))
            String(latin1Bytes, java.nio.charset.Charset.forName("Shift_JIS"))
        } catch (e: Exception) {
            s
        }
    }


    
    private val networkClient = NetworkClient()
    private val audioEngine = AudioEngine()
    private val videoEngine = VideoEngine()
    private val moderationManager = ModerationManager(application)

    private val _messages = MutableStateFlow<List<ChatMessage>>(emptyList())
    val messages: StateFlow<List<ChatMessage>> = _messages.asStateFlow()
    
    private val _connectionState = MutableStateFlow("Disconnected")
    val connectionState: StateFlow<String> = _connectionState.asStateFlow()

    private val _roomList = MutableStateFlow<List<RoomItem>>(emptyList())
    val roomList: StateFlow<List<RoomItem>> = _roomList.asStateFlow()

    private val _users = MutableStateFlow<List<UserItem>>(emptyList())
    val users: StateFlow<List<UserItem>> = _users.asStateFlow()
    private val userMap = mutableMapOf<String, UserItem>()

    var myUserId = UUID.randomUUID().toString().replace("-", "")
    var myUserName = "User_${System.currentTimeMillis() % 1000}"
    var myNameColor = -1
    
    private var serverSessionId: Int = 0
    private var audioSeqNum: Short = 0
    private var videoSeqNum: Short = 0
    
    private val _videoStreamers = MutableStateFlow<Map<String, Long>>(emptyMap())
    val videoStreamers: StateFlow<Map<String, Long>> = _videoStreamers

    private val _isTalkOn = MutableStateFlow(false)
    val isTalkOn: StateFlow<Boolean> = _isTalkOn

    private val _isVideoOn = MutableStateFlow(false)
    val isVideoOn: StateFlow<Boolean> = _isVideoOn

    fun setVideoOn(isOn: Boolean) {
        _isVideoOn.value = isOn
    }

    private val _micVolume = MutableStateFlow(0)
    val micVolume: StateFlow<Int> = _micVolume

    private val _watchingUserId = MutableStateFlow<String?>(null)
    val watchingUserId: StateFlow<String?> = _watchingUserId

    private val _ignoredUserIds = MutableStateFlow<Set<String>>(emptySet())
    val ignoredUserIds: StateFlow<Set<String>> = _ignoredUserIds

    private var currentIp: String = ""
    private var isReconnecting: Boolean = false
    private var currentRoom: String = ""

    init {
        audioEngine.initAudio()
        
        videoEngine.processEncoderOutput { h264Data ->
            if (serverSessionId != 0) {
                val header = MediaHeader(
                    sessionId = serverSessionId,
                    seqNum = videoSeqNum++,
                    payloadType = MediaHeader.VIDEO_H264,
                    flags = 0,
                    timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                    userId = myUserId
                )
                networkClient.sendUdpPacket(header.toByteArray(h264Data))
            }
        }

        viewModelScope.launch {
            while (true) {
                delay(1000)
                val now = System.currentTimeMillis()
                _videoStreamers.value = _videoStreamers.value.filterValues { now - it < 5000 }
            }
        }
        viewModelScope.launch {
            while (true) {
                delay(2000)
                if (_isVideoOn.value && serverSessionId != 0) {
                    val header = MediaHeader(
                        sessionId = serverSessionId,
                        seqNum = 0,
                        payloadType = MediaHeader.STREAM_STATUS,
                        flags = 0,
                        timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                        userId = myUserId
                    )
                    networkClient.sendUdpPacket(header.toByteArray(ByteArray(0)))
                }
            }
        }
    }

    fun getVideoEngine() = videoEngine

    fun setTalkOn(isOn: Boolean) {
        _isTalkOn.value = isOn
        if (isOn) {
            audioEngine.startRecording { opusData, vol ->
                _micVolume.value = vol
                if (serverSessionId != 0 && _isTalkOn.value) {
                    val header = MediaHeader(
                        sessionId = serverSessionId,
                        seqNum = audioSeqNum++,
                        payloadType = MediaHeader.AUDIO,
                        flags = 0,
                        timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                        userId = myUserId
                    )
                    networkClient.sendUdpPacket(header.toByteArray(opusData))
                }
            }
        } else {
            audioEngine.stopRecording()
            _micVolume.value = 0
        }
    }

    fun setWatchingUserId(userId: String?) {
        _watchingUserId.value = userId
    }

    fun toggleIgnoreUser(userId: String) {
        val current = _ignoredUserIds.value.toMutableSet()
        if (current.contains(userId)) current.remove(userId) else current.add(userId)
        _ignoredUserIds.value = current
    }

    fun connect(ip: String, isPrivate: Boolean = false, room: String? = null, serverRoomOverride: String? = null) {
        if ((_connectionState.value == "Connected" || _connectionState.value == "Connecting...") && !isReconnecting) {
            return
        }
        currentIp = ip
        val prevRoom = currentRoom
        currentRoom = room ?: getApplication<Application>().getString(R.string.lobby)
        viewModelScope.launch {
            _connectionState.value = "Connecting..."
            val success = networkClient.connect(ip)
            val wasReconnecting = isReconnecting
            isReconnecting = false
            if (success) {
                _connectionState.value = "Connected"
                startListening()

        
        Thread {
            while (true) {
                val udpData = networkClient.receiveUdpPacket() ?: break
                val parsed = MediaHeader.parse(udpData)
                if (parsed != null) {
                    val (header, payload) = parsed
                    if (_ignoredUserIds.value.contains(header.userId)) continue

                    if (header.payloadType == MediaHeader.AUDIO && header.userId != myUserId) {
                        audioEngine.playOpusData(header.userId, payload)
                    } else if ((header.payloadType == MediaHeader.VIDEO || header.payloadType == MediaHeader.VIDEO_H264 || header.payloadType == MediaHeader.STREAM_STATUS) && header.userId != myUserId) {
                        _videoStreamers.value = _videoStreamers.value + (header.userId to System.currentTimeMillis())
                        if (_watchingUserId.value == header.userId || _watchingUserId.value == myUserId && header.userId == myUserId) {
                            if (header.payloadType == MediaHeader.VIDEO_H264) {
                                videoEngine.decodeH264(payload)
                            }
                        }
                    }
                }
            }
        }.start()
                
                val displayMyName = if (myUserName.contains("#")) myUserName.substringBefore("#") else myUserName
                if (!wasReconnecting) {
                    appendSystemMessage("${getApplication<Application>().getString(R.string.joined_network)} ($displayMyName)")
                }
                val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)
                if (!wasReconnecting || actualRoom != prevRoom) {
                    appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))
                    appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayMyName, actualRoom))
                }
                
                val serverRoomName = serverRoomOverride ?: actualRoom
                val joinMsg = ChatMessage(
                    type = "join", 
                    userName = myUserName, 
                    secretId = myUserId, 
                    pub = !isPrivate, 
                    roomName = serverRoomName, 
                    nclr = myNameColor.toLong()
                )
                networkClient.sendTcpMessage(joinMsg)
            } else {
                _connectionState.value = "Failed"
            }
        }
    }

    private fun startListening() {
        viewModelScope.launch {
            while (true) {
                val msgStr = networkClient.receiveTcpMessage() ?: break
                  android.util.Log.d("ChatApp", "Received TCP: " + msgStr)
                try {
                    val message = Json { ignoreUnknownKeys = true }.decodeFromString<ChatMessage>(msgStr)
                    if (message.sessionId != null) {
                        if (serverSessionId == 0) {
                            serverSessionId = message.sessionId
                            viewModelScope.launch(Dispatchers.IO) {
                                while(isActive) {
                                    val header = MediaHeader(
                                        sessionId = serverSessionId,
                                        seqNum = 0,
                                        payloadType = MediaHeader.HOLE_PUNCH,
                                        flags = 0,
                                        timestamp = 0,
                                        userId = myUserId
                                    )
                                    networkClient.sendUdpPacket(header.toByteArray(ByteArray(0)))
                                    delay(5000)
                                }
                            }
                        } else {
                            serverSessionId = message.sessionId
                        }
                    }
                    if (!handleIncomingMessage(message)) break
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
            if (!isReconnecting) {
                _connectionState.value = "Disconnected"
                appendSystemMessage(getApplication<Application>().getString(R.string.disconnected_from_server))
            }
        }    }

    private fun handleIncomingMessage(message: ChatMessage): Boolean {
        when (message.type) {
            "chat" -> {
                val text = message.msg ?: message.content ?: return true
                val senderId = message.userId ?: ""
                
                val filteredText = moderationManager.filterProfanity(text, senderId, myUserId)
                if (filteredText == null) return true
                
                val finalMsg = message.copy(msg = TextProcessor.sanitizeText(filteredText), content = null)
                _messages.value = _messages.value + finalMsg
            }
            "sysmsg", "system" -> {
                appendSystemMessage(message.msg ?: "System message")
            }
            "disconnect" -> {
                appendSystemMessage(getApplication<Application>().getString(R.string.disconnected_reason, message.reason ?: "Unknown reason"))
            }
                        "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: ""
                    isReconnecting = true
                    networkClient.disconnect()
                    viewModelScope.launch {
                        connect(newIp, false, currentRoom, redirectRoom)
                    }
                    return false
                }
            }
            "welcome" -> {
                val newId = message.userId
                if (!newId.isNullOrEmpty()) {
                    myUserId = newId
                }
            }
            "users" -> {
                userMap.clear()
                message.users?.forEach { u -> userMap[u.userId] = UserItem(u.userId, u.userName ?: "Unknown", (u.nclr ?: -1L).toInt()) }
                _users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })
            }
            "join" -> {
                val uid = message.userId ?: return true
                val uname = message.userName ?: "Unknown"
                userMap[uid] = UserItem(uid, uname, (message.nclr ?: -1L).toInt())
                _users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })
                val displayUname = if (uname.contains("#")) uname.substringBefore("#") else uname
                appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayUname, currentRoom))
            }
            "leave" -> {
                val uid = message.userId ?: return true
                val uname = userMap.remove(uid)?.userName ?: "Unknown"
                _users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })
                val displayUname = if (uname.contains("#")) uname.substringBefore("#") else uname
                appendSystemMessage(getApplication<Application>().getString(R.string.room_leave_fmt, displayUname, currentRoom))
            }
            "rename" -> {
                val uid = message.userId ?: return true
                val newName = message.newName ?: return true
                val oldName = userMap[uid]?.userName ?: "Unknown"
                userMap[uid] = UserItem(uid, newName, userMap[uid]?.nclr ?: -1)
                _users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })
                val displayOld = if (oldName.contains("#")) oldName.substringBefore("#") else oldName
                val displayNew = if (newName.contains("#")) newName.substringBefore("#") else newName
                appendSystemMessage(getApplication<Application>().getString(R.string.renamed_fmt, displayOld, displayNew))
            }
            "room_list" -> {
                val rooms = message.rooms?.map { RoomItem(it.name ?: "", it.count ?: 0) } ?: emptyList()
                _roomList.value = rooms
            }
        }
        return true
    }

    private fun appendSystemMessage(text: String) {
        val sysMsg = ChatMessage(type = "system", msg = text, userName = "System")
        _messages.value = _messages.value + sysMsg
    }

    fun sendMessage(text: String, isBold: Boolean = false, isItalic: Boolean = false, isUnderline: Boolean = false, fontSize: Int = 10, colorInt: Int = 0) {
        viewModelScope.launch {
            val sanitized = TextProcessor.sanitizeText(text)
            if (sanitized.isBlank()) return@launch
            
            val msg = ChatMessage(
                type = "chat",
                msg = sanitized,
                user = myUserName,
                b = isBold,
                i = isItalic,
                u = isUnderline,
                sz = fontSize,
                clr = colorInt,
                nclr = -1
            )
            networkClient.sendTcpMessage(msg)
            val optimisticMsg = ChatMessage(
                type = "chat",
                msg = sanitized,
                userName = myUserName,
                userId = myUserId,
                b = isBold,
                i = isItalic,
                u = isUnderline,
                sz = fontSize,
                clr = colorInt
            )
            _messages.value = _messages.value + optimisticMsg
        }
    }

    fun rename(newName: String, newColor: Int = -1) {
        val sanitized = TextProcessor.sanitizeText(newName)
        if ((sanitized.isBlank() || sanitized == myUserName) && newColor == myNameColor) return
        viewModelScope.launch {
            myUserName = if (sanitized.isNotBlank()) sanitized else myUserName
            myNameColor = newColor
            val msg = ChatMessage(type = "rename", userName = myUserName, newName = myUserName, userId = myUserId, nclr = myNameColor.toLong())
            networkClient.sendTcpMessage(msg)
            
            userMap[myUserId] = UserItem(myUserId, myUserName, myNameColor)
            _users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })
        }
    }

    fun requestRooms() {
        viewModelScope.launch {
            networkClient.sendTcpMessage(ChatMessage(type = "get_rooms"))
        }
    }

    fun closeRooms() {
        viewModelScope.launch {
            networkClient.sendTcpMessage(ChatMessage(type = "close_rooms"))
        }
    }

    fun moveRoom(roomName: String, isPrivate: Boolean) {
        isReconnecting = true
        networkClient.disconnect()
        connect(currentIp, isPrivate, roomName)
    }

    override fun onCleared() {
        super.onCleared()
        networkClient.disconnect()
        audioEngine.release()
        videoEngine.release()
    }
}

