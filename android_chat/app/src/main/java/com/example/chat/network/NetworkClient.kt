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
data class ChatMessage(
    val type: String,
    val content: String? = null,
    val roomId: String? = null,
    val userId: String? = null
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
