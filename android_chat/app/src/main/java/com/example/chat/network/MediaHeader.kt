package com.example.chat.network

import java.nio.ByteBuffer
import java.nio.ByteOrder

data class MediaHeader(
    val sessionId: Int,
    val seqNum: Short,
    val payloadType: Byte, // 1 = Audio, 2 = Video, 255 = HolePunch
    val flags: Byte,
    val timestamp: Int,
    val userId: String // 40 wchar_t -> up to 39 chars + null terminator
) {
    companion object {
        const val SIZE = 92
        const val AUDIO: Byte = 1
        const val VIDEO: Byte = 2
        const val STREAM_STATUS: Byte = 4
        const val VIDEO_H264: Byte = 5
        const val HOLE_PUNCH: Byte = 255.toByte()

        fun parse(data: ByteArray): Pair<MediaHeader, ByteArray>? {
            if (data.size < SIZE) return null
            val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
            val sessionId = buffer.int
            val seqNum = buffer.short
            val payloadType = buffer.get()
            val flags = buffer.get()
            val timestamp = buffer.int
            
            val userIdBytes = ByteArray(80)
            buffer.get(userIdBytes)
            // Parse UTF-16LE string (until double null)
            var strLen = 0
            while (strLen < 80 && !(userIdBytes[strLen].toInt() == 0 && userIdBytes[strLen+1].toInt() == 0)) {
                strLen += 2
            }
            val userId = String(userIdBytes, 0, strLen, Charsets.UTF_16LE)
            
            val header = MediaHeader(sessionId, seqNum, payloadType, flags, timestamp, userId)
            
            val payloadSize = data.size - SIZE
            val payload = ByteArray(payloadSize)
            buffer.get(payload)
            
            return Pair(header, payload)
        }
    }

    fun toByteArray(payload: ByteArray): ByteArray {
        val buffer = ByteBuffer.allocate(SIZE + payload.size).order(ByteOrder.LITTLE_ENDIAN)
        buffer.putInt(sessionId)
        buffer.putShort(seqNum)
        buffer.put(payloadType)
        buffer.put(flags)
        buffer.putInt(timestamp)
        
        val userBytes = userId.toByteArray(Charsets.UTF_16LE)
        val copyLen = userBytes.size.coerceAtMost(78) // Leave room for null terminator
        buffer.put(userBytes, 0, copyLen)
        // Pad the rest of the 80 bytes with 0
        for (i in copyLen until 80) {
            buffer.put(0.toByte())
        }
        
        buffer.put(payload)
        return buffer.array()
    }
}
