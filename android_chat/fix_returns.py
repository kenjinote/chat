import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'val text = message.msg ?: message.content ?: return',
    'val text = message.msg ?: message.content ?: return true'
)
content = content.replace(
    'if (filteredText == null) return',
    'if (filteredText == null) return true'
)
content = content.replace(
    'val uid = message.userId ?: return',
    'val uid = message.userId ?: return true'
)
content = content.replace(
    'val newName = message.newName ?: return',
    'val newName = message.newName ?: return true'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed returns in handleIncomingMessage")
