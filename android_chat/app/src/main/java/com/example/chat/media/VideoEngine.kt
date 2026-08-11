package com.example.chat.media

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.view.Surface
import androidx.camera.core.ImageProxy
import java.nio.ByteBuffer
import android.util.Log

class VideoEngine {
    private var encoder: MediaCodec? = null
    private var decoder: MediaCodec? = null
    private var activeSurface: Surface? = null
    
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
            Log.d("VideoEngine", "Encoder started")
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        activeSurface = surface
        try {
            val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height)
            decoder = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
            decoder?.configure(format, surface, null, 0)
            decoder?.start()
            Log.d("VideoEngine", "Decoder started with surface")
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
        Log.d("VideoEngine", "Decoder queued input buffer")
            val bufferInfo = MediaCodec.BufferInfo()
        val encoder = encoder ?: return
        var configData: ByteArray? = null
        
        Thread {
            while (true) {
                try {
                    val outputBufferId = encoder.dequeueOutputBuffer(bufferInfo, 10000)
                    // Log.d("VideoEngine", "Encoder dequeued: , size: , flags: ")
                    if (outputBufferId >= 0) {
                        val outputBuffer: ByteBuffer? = encoder.getOutputBuffer(outputBufferId)
                        if (outputBuffer != null && bufferInfo.size > 0) {
                            val outData = ByteArray(bufferInfo.size)
                            outputBuffer.get(outData)
                            
                            if ((bufferInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                                configData = outData
                            } else if ((bufferInfo.flags and MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0 && configData != null) {
                                val combined = ByteArray(configData!!.size + outData.size)
                                System.arraycopy(configData!!, 0, combined, 0, configData!!.size)
                                System.arraycopy(outData, 0, combined, configData!!.size, outData.size)
                                Log.d("VideoEngine", "Sending keyframe with config, size: ")
                                onH264Data(combined)
                            } else {
                                Log.d("VideoEngine", "Sending frame, size: , flags: ")
                                onH264Data(outData)
                            }
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
        android.util.Log.d("VideoEngine", "decodeH264 called with size: " + data.size)
        val decoder = decoder ?: return
        try {
            var naluData = data
            var offset = -1
            // Search for Annex-B start code 00 00 01 or 00 00 00 01
            for (i in 0 until data.size - 3) {
                if (data[i] == 0.toByte() && data[i+1] == 0.toByte() && data[i+2] == 1.toByte()) {
                    offset = i
                    break
                }
                if (data[i] == 0.toByte() && data[i+1] == 0.toByte() && data[i+2] == 0.toByte() && data[i+3] == 1.toByte()) {
                    offset = i
                    break
                }
            }

            if (offset > 0) {
                naluData = ByteArray(data.size - offset)
                System.arraycopy(data, offset, naluData, 0, naluData.size)
            } else if (offset < 0) {
                val startCode = byteArrayOf(0, 0, 0, 1)
                val combined = ByteArray(data.size + 4)
                System.arraycopy(startCode, 0, combined, 0, 4)
                System.arraycopy(data, 0, combined, 4, data.size)
                naluData = combined
            }

            val inputBufferId = decoder.dequeueInputBuffer(10000)
            if (inputBufferId >= 0) {
                val inputBuffer = decoder.getInputBuffer(inputBufferId)
                inputBuffer?.clear()
                inputBuffer?.put(naluData)
                decoder.queueInputBuffer(inputBufferId, 0, naluData.size, System.nanoTime() / 1000, 0)
            }

            Log.d("VideoEngine", "Decoder queued input buffer")
            val bufferInfo = MediaCodec.BufferInfo()
            var outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, 10000)
            while (outputBufferId >= 0) {
                // Render to surface
                Log.d("VideoEngine", "Decoder rendered frame to surface")
                decoder.releaseOutputBuffer(outputBufferId, true)
                outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, 0)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun stopDecoder() {
        try {
            decoder?.stop()
            decoder?.release()
            decoder = null
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun release() {
        encoder?.stop()
        encoder?.release()
        stopDecoder()
    }

    fun decodeLegacyVideo(data: ByteArray) {
        android.util.Log.d("VideoEngine", "decodeLegacyVideo called with size: " + data.size)
        val surface = activeSurface ?: return
        try {
            var offset = 0
            for (i in 0 until data.size - 2) {
                if (data[i] == 0xFF.toByte() && data[i+1] == 0xD8.toByte() && data[i+2] == 0xFF.toByte()) {
                    offset = i
                    break
                }
            }
            val bitmap = android.graphics.BitmapFactory.decodeByteArray(data, offset, data.size - offset)
            if (bitmap != null) {
                android.util.Log.d("VideoEngine", "Legacy video bitmap decoded successfully")
                val canvas = surface.lockCanvas(null)
                if (canvas != null) {
                    val rect = android.graphics.Rect(0, 0, canvas.width, canvas.height)
                    canvas.drawBitmap(bitmap, null, rect, null)
                    surface.unlockCanvasAndPost(canvas)
                }
            } else {
                android.util.Log.e("VideoEngine", "Failed to decode legacy video bitmap at offset " + offset)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}
