import re

path = 'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add Log import if not exists
if 'import android.util.Log' not in content:
    content = content.replace('import java.nio.ByteBuffer', 'import java.nio.ByteBuffer\nimport android.util.Log')

# initEncoder
content = content.replace('encoder?.start()', 'encoder?.start()\n            Log.d("VideoEngine", "Encoder started")')

# initDecoder
content = content.replace('decoder?.start()', 'decoder?.start()\n            Log.d("VideoEngine", "Decoder started with surface")')

# processEncoderOutput
content = content.replace('val outputBufferId = encoder.dequeueOutputBuffer(bufferInfo, 10000)', 'val outputBufferId = encoder.dequeueOutputBuffer(bufferInfo, 10000)\n                    // Log.d("VideoEngine", "Encoder dequeued: , size: , flags: ")')
content = content.replace('onH264Data(combined)', 'Log.d("VideoEngine", "Sending keyframe with config, size: ")\n                                onH264Data(combined)')
content = content.replace('onH264Data(outData)', 'Log.d("VideoEngine", "Sending frame, size: , flags: ")\n                                onH264Data(outData)')

# decodeH264
content = content.replace('fun decodeH264(data: ByteArray) {', 'fun decodeH264(data: ByteArray) {\n        Log.d("VideoEngine", "decodeH264 called with data size: ")')
content = content.replace('val bufferInfo = MediaCodec.BufferInfo()', 'Log.d("VideoEngine", "Decoder queued input buffer")\n            val bufferInfo = MediaCodec.BufferInfo()')
content = content.replace('decoder.releaseOutputBuffer(outputBufferId, true)', 'Log.d("VideoEngine", "Decoder rendered frame to surface")\n                decoder.releaseOutputBuffer(outputBufferId, true)')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added logs to VideoEngine.kt")
