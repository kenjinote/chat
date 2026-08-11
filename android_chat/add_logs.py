import re

main = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'tcpWriter?.println(jsonStr)',
    'android.util.Log.d("ChatApp", "Sending TCP: " + jsonStr)\n            tcpWriter?.println(jsonStr)'
)

content = content.replace(
    'val msgStr = networkClient.receiveTcpMessage() ?: break',
    'val msgStr = networkClient.receiveTcpMessage() ?: break\n                android.util.Log.d("ChatApp", "Received TCP: " + msgStr)'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added logs")
