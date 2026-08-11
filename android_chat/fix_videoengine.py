import re

path = 'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_process = '''    fun processEncoderOutput(onH264Data: (ByteArray) -> Unit) {
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
    }'''

new_process = '''    fun processEncoderOutput(onH264Data: (ByteArray) -> Unit) {
        val bufferInfo = MediaCodec.BufferInfo()
        val encoder = encoder ?: return
        var configData: ByteArray? = null
        
        Thread {
            while (true) {
                try {
                    val outputBufferId = encoder.dequeueOutputBuffer(bufferInfo, 10000)
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
                                onH264Data(combined)
                            } else {
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
    }'''

if old_process in content:
    content = content.replace(old_process, new_process)
    print("Updated processEncoderOutput to prepend SPS/PPS config to keyframes")
else:
    print("Could not find processEncoderOutput")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
