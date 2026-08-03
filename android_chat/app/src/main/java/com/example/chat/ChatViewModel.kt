package com.example.chat

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.chat.media.AudioEngine
import com.example.chat.media.VideoEngine
import com.example.chat.network.ChatMessage
import com.example.chat.network.NetworkClient
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.serialization.json.Json

class ChatViewModel : ViewModel() {
    private val networkClient = NetworkClient()
    private val audioEngine = AudioEngine()
    private val videoEngine = VideoEngine()

    private val _messages = MutableStateFlow<List<ChatMessage>>(emptyList())
    val messages: StateFlow<List<ChatMessage>> = _messages.asStateFlow()
    
    private val _connectionState = MutableStateFlow("Disconnected")
    val connectionState: StateFlow<String> = _connectionState.asStateFlow()

    fun connect(ip: String) {
        viewModelScope.launch {
            _connectionState.value = "Connecting..."
            val success = networkClient.connect(ip)
            if (success) {
                _connectionState.value = "Connected"
                startListening()
                
                // Init Media
                audioEngine.initAudio()
                audioEngine.startRecording { opusData ->
                    // networkClient.sendUdpPacket(...)
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
                    val message = Json.decodeFromString<ChatMessage>(msgStr)
                    _messages.value = _messages.value + message
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
            _connectionState.value = "Disconnected"
        }
        
        Thread {
            while (true) {
                val udpData = networkClient.receiveUdpPacket() ?: break
                // parse MediaHeader
                // val header = parseMediaHeader(udpData)
                // if (header.payloadType == AUDIO) {
                //     audioEngine.playOpusData(...)
                // } else {
                //     videoEngine.decodeH264(...)
                // }
            }
        }.start()
    }

    fun sendMessage(text: String) {
        viewModelScope.launch {
            val msg = ChatMessage(type = "text", content = text)
            networkClient.sendTcpMessage(msg)
            _messages.value = _messages.value + msg // optimistic UI
        }
    }

    override fun onCleared() {
        super.onCleared()
        networkClient.disconnect()
        audioEngine.release()
        videoEngine.release()
    }
}
