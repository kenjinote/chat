import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'val msgStr = networkClient.receiveTcpMessage() ?: break',
    'val msgStr = networkClient.receiveTcpMessage() ?: break\n                  android.util.Log.d("ChatApp", "Received TCP: " + msgStr)'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added logs to ChatViewModel")
