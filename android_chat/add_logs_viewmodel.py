import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add Log import if not exists
if 'import android.util.Log' not in content:
    content = content.replace('import androidx.lifecycle.viewModelScope', 'import androidx.lifecycle.viewModelScope\nimport android.util.Log')

content = content.replace('networkClient.sendUdpPacket(header.toByteArray(h264Data))', 'Log.d("ChatViewModel", "Sending UDP video packet, size: ")\n                  networkClient.sendUdpPacket(header.toByteArray(h264Data))')

content = content.replace('if (header.payloadType == MediaHeader.VIDEO_H264) {', 'if (header.payloadType == MediaHeader.VIDEO_H264) {\n                                Log.d("ChatViewModel", "Received UDP video packet, size: ")')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added logs to ChatViewModel.kt")
