package com.example.chat.network

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.Serializable
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.Socket

@Serializable
data class UserItem(
    val userId: String,
    val userName: String,
    val nclr: Int? = null
)

@Serializable
data class ChatMessage(
    val type: String,
    val content: String? = null,
    val roomId: String? = null,
    val userId: String? = null,
    val userName: String? = null,
    val isPrivate: Boolean? = null,
    val newName: String? = null,
    val reason: String? = null,
    val error: String? = null,
    val msg: String? = null,
    val sessionId: Int? = null,
    val secretId: String? = null,
    val roomName: String? = null,
    val pub: Boolean? = null,
    val nclr: Int? = null,
    val user: String? = null,
    val b: Boolean? = null,
    val i: Boolean? = null,
    val u: Boolean? = null,
    val sz: Int? = null,
    val clr: Int? = null,
    val users: List<UserItem>? = null
)

class NetworkClient {
    private var tcpSocket: Socket? = null
    private var tcpReader: BufferedReader? = null
    private var tcpWriter: PrintWriter? = null
    private var udpSocket: DatagramSocket? = null
    private var serverAddress: InetAddress? = null

    suspend fun connect(ip: String, tcpPort: Int = 4000, udpPort: Int = 4001): Boolean = withContext(Dispatchers.IO) {
        try {
            serverAddress = InetAddress.getByName(ip)
            tcpSocket = Socket(serverAddress, tcpPort)
            tcpReader = BufferedReader(InputStreamReader(tcpSocket!!.inputStream))
            tcpWriter = PrintWriter(tcpSocket!!.outputStream, true)
            
            udpSocket = DatagramSocket()
            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }

    suspend fun sendTcpMessage(message: ChatMessage) = withContext(Dispatchers.IO) {
        try {
            val jsonStr = Json.encodeToString(message)
            tcpWriter?.println(jsonStr)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    suspend fun receiveTcpMessage(): String? = withContext(Dispatchers.IO) {
        try {
            tcpReader?.readLine()
        } catch (e: Exception) {
            null
        }
    }

    fun sendUdpPacket(data: ByteArray) {
        try {
            val address = serverAddress ?: return
            val packet = DatagramPacket(data, data.size, address, 4001)
            udpSocket?.send(packet)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun receiveUdpPacket(bufferSize: Int = 1500): ByteArray? {
        return try {
            val buffer = ByteArray(bufferSize)
            val packet = DatagramPacket(buffer, buffer.size)
            udpSocket?.receive(packet)
            packet.data.copyOf(packet.length)
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    fun disconnect() {
        try {
            tcpSocket?.close()
            udpSocket?.close()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}
