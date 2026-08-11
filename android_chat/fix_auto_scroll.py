import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace animateScrollToItem with scrollToItem
old_code = 'chatListState.animateScrollToItem(messagesSize - 1)'
new_code = 'chatListState.scrollToItem(messagesSize - 1)'
content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scroll to instant")
