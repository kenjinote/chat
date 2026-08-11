import re

path = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('fun receiveUdpPacket(bufferSize: Int = 1500)', 'fun receiveUdpPacket(bufferSize: Int = 65535)')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated receiveUdpPacket buffer size to 65535")
