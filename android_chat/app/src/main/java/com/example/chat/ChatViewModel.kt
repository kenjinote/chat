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
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.serialization.json.Json
import java.util.UUID

class ChatViewModel(application: Application) : AndroidViewModel(application) {
    private val networkClient = NetworkClient()
    private val audioEngine = AudioEngine()
    private val videoEngine = VideoEngine()
    private val moderationManager = ModerationManager(application)

    private val _messages = MutableStateFlow<List<ChatMessage>>(emptyList())
    val messages: StateFlow<List<ChatMessage>> = _messages.asStateFlow()
    
    private val _connectionState = MutableStateFlow("Disconnected")
    val connectionState: StateFlow<String> = _connectionState.asStateFlow()

    private val _users = MutableStateFlow<List<String>>(emptyList())
    val users: StateFlow<List<String>> = _users.asStateFlow()
    private val userMap = mutableMapOf<String, String>()

    val myUserId = java.util.UUID.randomUUID().toString().replace("-", "")
    var myUserName = "User_${System.currentTimeMillis() % 1000}"
    
    private var serverSessionId: Int = 0
    private var audioSeqNum: Short = 0
    private var videoSeqNum: Short = 0
    
    // Expose videoEngine for UI setup
    fun getVideoEngine() = videoEngine

    fun connect(ip: String, isPrivate: Boolean) {
        viewModelScope.launch {
            _connectionState.value = "Connecting..."
            val success = networkClient.connect(ip)
            if (success) {
                _connectionState.value = "Connected"
                startListening()
                
                // Send join request
                val joinMsg = ChatMessage(
                    type = "join", 
                    userName = myUserName, 
                    secretId = myUserId, 
                    pub = !isPrivate, 
                    roomName = "ロビー", 
                    nclr = -1
                )
                networkClient.sendTcpMessage(joinMsg)

                // Init Media (Video encoder is initialized in UI when CameraX is ready)
                audioEngine.initAudio()
                audioEngine.startRecording { opusData ->
                    if (serverSessionId != 0) {
                        val header = MediaHeader(
                            sessionId = serverSessionId,
                            seqNum = audioSeqNum++,
                            payloadType = MediaHeader.AUDIO,
                            flags = 0,
                            timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                            userId = myUserId
                        )
                        val packetData = header.toByteArray(opusData)
                        networkClient.sendUdpPacket(packetData)
                    }
                }
                
                videoEngine.processEncoderOutput { h264Data ->
                    if (serverSessionId != 0) {
                        val header = MediaHeader(
                            sessionId = serverSessionId,
                            seqNum = videoSeqNum++,
                            payloadType = MediaHeader.VIDEO,
                            flags = 0,
                            timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                            userId = myUserId
                        )
                        val packetData = header.toByteArray(h264Data)
                        networkClient.sendUdpPacket(packetData)
                    }
                }
            } else {
                _connectionState.value = "Failed"
            }
        }
    }

    private fun startListening() {
        viewModelScope.launch {
            while (true) {
                val msgStr = networkClient.receiveTcpMessage() ?: break
                try {
                    val message = Json { ignoreUnknownKeys = true }.decodeFromString<ChatMessage>(msgStr)
                    if (message.sessionId != null) {
                        serverSessionId = message.sessionId
                    }
                    handleIncomingMessage(message)
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
            _connectionState.value = "Disconnected"
            appendSystemMessage("Disconnected from server.")
        }
        
        Thread {
            while (true) {
                val udpData = networkClient.receiveUdpPacket() ?: break
                val parsed = MediaHeader.parse(udpData)
                if (parsed != null) {
                    val (header, payload) = parsed
                    if (header.payloadType == MediaHeader.AUDIO && header.userId != myUserId) {
                        audioEngine.playOpusData(header.userId, payload)
                    } else if (header.payloadType == MediaHeader.VIDEO && header.userId != myUserId) {
                        videoEngine.decodeH264(payload)
                    }
                }
            }
        }.start()
    }

    private fun handleIncomingMessage(message: ChatMessage) {
        when (message.type) {
            "chat" -> {
                val text = message.msg ?: message.content ?: return
                val senderId = message.userId ?: ""
                
                // Moderation
                val filteredText = moderationManager.filterProfanity(text, senderId, myUserId)
                if (filteredText == null) return // Ignored user or immediate ban
                
                // Add sanitized message
                val finalMsg = message.copy(msg = TextProcessor.sanitizeText(filteredText), content = null)
                _messages.value = _messages.value + finalMsg
            }
            "sysmsg", "system" -> {
                appendSystemMessage(message.msg ?: "System message")
            }
            "disconnect" -> {
                appendSystemMessage("Disconnected: ${message.reason ?: "Unknown reason"}")
            }
            "users" -> {
                userMap.clear()
                message.users?.forEach { u -> userMap[u.userId] = u.userName }
                _users.value = userMap.values.toList()
            }
            "join" -> {
                val uid = message.userId ?: return
                val uname = message.userName ?: "Unknown"
                userMap[uid] = uname
                _users.value = userMap.values.toList()
                appendSystemMessage("$uname joined the room.")
            }
            "leave" -> {
                val uid = message.userId ?: return
                val uname = userMap.remove(uid) ?: "Unknown"
                _users.value = userMap.values.toList()
                appendSystemMessage("$uname left the room.")
            }
            "rename" -> {
                val uid = message.userId ?: return
                val newName = message.newName ?: return
                val oldName = userMap[uid] ?: "Unknown"
                userMap[uid] = newName
                _users.value = userMap.values.toList()
                appendSystemMessage("$oldName renamed to $newName")
            }
        }
    }

    private fun appendSystemMessage(text: String) {
        val sysMsg = ChatMessage(type = "system", msg = text, userName = "System")
        _messages.value = _messages.value + sysMsg
    }

    fun sendMessage(text: String) {
        viewModelScope.launch {
            val sanitized = TextProcessor.sanitizeText(text)
            if (sanitized.isBlank()) return@launch
            
            val msg = ChatMessage(
                type = "chat",
                msg = sanitized,
                user = myUserName,
                b = false,
                i = false,
                u = false,
                sz = 10,
                clr = 0,
                nclr = -1
            )
            networkClient.sendTcpMessage(msg)
            // optimistic UI update with mapping to received format
            val optimisticMsg = ChatMessage(
                type = "chat",
                msg = sanitized,
                userName = myUserName,
                userId = myUserId
            )
            _messages.value = _messages.value + optimisticMsg
        }
    }

    fun rename(newName: String) {
        val sanitized = TextProcessor.sanitizeText(newName)
        if (sanitized.isBlank() || sanitized == myUserName) return
        viewModelScope.launch {
            myUserName = sanitized
            val msg = ChatMessage(type = "rename", newName = sanitized, userId = myUserId)
            networkClient.sendTcpMessage(msg)
            appendSystemMessage("You changed your name to $sanitized")
        }
    }

    override fun onCleared() {
        super.onCleared()
        networkClient.disconnect()
        audioEngine.release()
        videoEngine.release()
    }
}
