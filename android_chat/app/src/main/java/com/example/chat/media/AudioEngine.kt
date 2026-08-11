package com.example.chat.media

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.AudioTrack
import android.media.MediaRecorder
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue

class AudioEngine {

    companion object {
        init {
            System.loadLibrary("opus_wrapper")
        }
    }

    private val sampleRate = 16000
    private val channels = 1 // Mono
    private val frameSize = 960 // 60ms at 16kHz
    private val pcmBufferSize = frameSize

    private var encoderHandle: Long = 0
    private var decoderHandle: Long = 0

    private var audioRecord: AudioRecord? = null
    private var audioTrack: AudioTrack? = null
    
    private val jitterBuffers = ConcurrentHashMap<String, ConcurrentLinkedQueue<ShortArray>>()
    private var isMixing = false

    private external fun createEncoder(sampleRate: Int, channels: Int, application: Int): Long
    private external fun encode(encoderHandle: Long, pcm: ShortArray, frameSize: Int, outOpus: ByteArray): Int
    private external fun destroyEncoder(encoderHandle: Long)

    private external fun createDecoder(sampleRate: Int, channels: Int): Long
    private external fun decode(decoderHandle: Long, opusData: ByteArray?, len: Int, outPcm: ShortArray, frameSize: Int): Int
    private external fun destroyDecoder(decoderHandle: Long)

    @SuppressLint("MissingPermission")
    fun initAudio() {
        encoderHandle = createEncoder(sampleRate, channels, 2048)
        decoderHandle = createDecoder(sampleRate, channels)

        val minRecBufSize = AudioRecord.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT
        )
        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            sampleRate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
            minRecBufSize.coerceAtLeast(pcmBufferSize * 2)
        )

        val minPlayBufSize = AudioTrack.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_OUT_MONO,
            AudioFormat.ENCODING_PCM_16BIT
        )
        audioTrack = AudioTrack.Builder()
            .setAudioFormat(
                AudioFormat.Builder()
                    .setSampleRate(sampleRate)
                    .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                    .build()
            )
            .setBufferSizeInBytes(minPlayBufSize.coerceAtLeast(pcmBufferSize * 2))
            .build()
            
        startMixer()
    }

    fun startRecording(onOpusData: (ByteArray) -> Unit) {
        audioRecord?.startRecording()
        val pcmBuffer = ShortArray(pcmBufferSize)
        val opusBuffer = ByteArray(4000)

        Thread {
            while (audioRecord?.recordingState == AudioRecord.RECORDSTATE_RECORDING) {
                val read = audioRecord?.read(pcmBuffer, 0, pcmBufferSize) ?: 0
                if (read > 0) {
                    val encodedSize = encode(encoderHandle, pcmBuffer, frameSize, opusBuffer)
                    if (encodedSize > 0) {
                        onOpusData(opusBuffer.copyOf(encodedSize))
                    }
                }
            }
        }.start()
    }
    
    private fun startMixer() {
        isMixing = true
        Thread {
            if (audioTrack?.playState != AudioTrack.PLAYSTATE_PLAYING) {
                audioTrack?.play()
            }
            val mixedBuffer = FloatArray(pcmBufferSize)
            val outBuffer = ShortArray(pcmBufferSize)
            
            while (isMixing) {
                mixedBuffer.fill(0f)
                var hasData = false
                
                for ((_, queue) in jitterBuffers) {
                    val frame = queue.poll()
                    if (frame != null) {
                        hasData = true
                        for (i in frame.indices) {
                            mixedBuffer[i] += frame[i].toFloat()
                        }
                    }
                }
                
                if (hasData) {
                    for (i in mixedBuffer.indices) {
                        val x = mixedBuffer[i] / 32768.0f
                        val absX = kotlin.math.abs(x)
                        val y: Float
                        if (absX < 0.5f) {
                            y = x
                        } else if (absX < 1.5f) {
                            val v = absX - 0.5f
                            val shaped = 0.5f + v - 0.5f * v * v
                            y = if (x > 0) shaped else -shaped
                        } else {
                            y = if (x > 0) 1.0f else -1.0f
                        }
                        outBuffer[i] = (y * 32767.0f).toInt().toShort()
                    }
                } else {
                    outBuffer.fill(0)
                }
                audioTrack?.write(outBuffer, 0, pcmBufferSize)
            }
        }.start()
    }

    fun playOpusData(userId: String, opusData: ByteArray) {
        val pcmBuffer = ShortArray(pcmBufferSize)
        val decodedSize = decode(decoderHandle, opusData, opusData.size, pcmBuffer, frameSize)
        if (decodedSize > 0) {
            val queue = jitterBuffers.getOrPut(userId) { ConcurrentLinkedQueue() }
            queue.offer(pcmBuffer.copyOf(decodedSize))
            // Limit jitter buffer size to prevent delay (e.g. max 5 frames)
            if (queue.size > 5) {
                queue.poll()
            }
        }
    }

    fun release() {
        isMixing = false
        audioRecord?.stop()
        audioRecord?.release()
        audioTrack?.stop()
        audioTrack?.release()
        if (encoderHandle != 0L) destroyEncoder(encoderHandle)
        if (decoderHandle != 0L) destroyDecoder(decoderHandle)
    }
}
