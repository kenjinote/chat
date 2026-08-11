import re
main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('val isStreaming = videoStreamers[user.userId] ?: false', 'val isStreaming = videoStreamers.containsKey(user.userId)')
content = content.replace('if (user.nclr != null && user.nclr != -1)', 'if (user.nclr != -1)')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed isStreaming and nclr")
