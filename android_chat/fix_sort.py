import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

pattern = r'_users\.value = userMap\.values\.toList\(\)\.sortedBy \{ it\.userName \}'
replacement = r'_users.value = userMap.values.toList().sortedWith(compareBy<UserItem> { if (moderationManager.isIgnored(it.userId)) 1 else 0 }.thenBy { it.userName.lowercase() })'
content = re.sub(pattern, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated sorting logic in ChatViewModel.kt")
