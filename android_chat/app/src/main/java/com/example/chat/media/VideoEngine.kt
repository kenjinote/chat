package com.example.chat.media

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.view.Surface
import androidx.camera.core.ImageProxy
import java.nio.ByteBuffer

class VideoEngine {
    private var encoder: MediaCodec? = null
    private var decoder: MediaCodec? = null
    
    fun initEncoder(width: Int = 640, height: Int = 480, fps: Int = 30) {
        try {
            val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height).apply {
                setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible)
                setInteger(MediaFormat.KEY_BIT_RATE, 500000)
                setInteger(MediaFormat.KEY_FRAME_RATE, fps)
                setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1) // 1 second between I-frames
            }
            
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
            encoder?.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            encoder?.start()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        try {
            val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height)
            decoder = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
            decoder?.configure(format, surface, null, 0)
            decoder?.start()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun encodeFrame(image: ImageProxy) {
        val encoder = encoder ?: return
        try {
            val inputBufferId = encoder.dequeueInputBuffer(10000)
            if (inputBufferId >= 0) {
                val inputBuffer = encoder.getInputBuffer(inputBufferId)
                inputBuffer?.clear()

                val yBuffer = image.planes[0].buffer
                val uBuffer = image.planes[1].buffer
                val vBuffer = image.planes[2].buffer

                val ySize = yBuffer.remaining()
                val uSize = uBuffer.remaining()
                val vSize = vBuffer.remaining()

                inputBuffer?.put(yBuffer)
                inputBuffer?.put(uBuffer)
                inputBuffer?.put(vBuffer)

                encoder.queueInputBuffer(inputBufferId, 0, ySize + uSize + vSize, image.imageInfo.timestamp * 1000, 0)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        } finally {
            image.close()
        }
    }

    fun processEncoderOutput(onH264Data: (ByteArray) -> Unit) {
        val bufferInfo = MediaCodec.BufferInfo()
        val encoder = encoder ?: return
        
        Thread {
            while (true) {
                try {
                    val outputBufferId = encoder.dequeueOutputBuffer(bufferInfo, 10000)
                    if (outputBufferId >= 0) {
                        val outputBuffer: ByteBuffer? = encoder.getOutputBuffer(outputBufferId)
                        if (outputBuffer != null && bufferInfo.size > 0) {
                            val outData = ByteArray(bufferInfo.size)
                            outputBuffer.get(outData)
                            onH264Data(outData)
                        }
                        encoder.releaseOutputBuffer(outputBufferId, false)
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                    break
                }
            }
        }.start()
    }

    fun decodeH264(data: ByteArray) {
        val decoder = decoder ?: return
        try {
            val inputBufferId = decoder.dequeueInputBuffer(10000)
            if (inputBufferId >= 0) {
                val inputBuffer = decoder.getInputBuffer(inputBufferId)
                inputBuffer?.clear()
                inputBuffer?.put(data)
                decoder.queueInputBuffer(inputBufferId, 0, data.size, 0, 0)
            }

            val bufferInfo = MediaCodec.BufferInfo()
            var outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, 10000)
            while (outputBufferId >= 0) {
                // Render to surface
                decoder.releaseOutputBuffer(outputBufferId, true)
                outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, 0)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun release() {
        encoder?.stop()
        encoder?.release()
        decoder?.stop()
        decoder?.release()
    }
}
