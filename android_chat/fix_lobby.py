import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace:
# val serverRoomName = serverRoomOverride ?: (if (actualRoom == getApplication<Application>().getString(R.string.lobby)) "" else actualRoom)
# With:
# val serverRoomName = serverRoomOverride ?: actualRoom

pattern = r'val serverRoomName = serverRoomOverride \?: \(if \(actualRoom == getApplication<Application>\(\)\.getString\(R\.string\.lobby\)\) "" else actualRoom\)'
replacement = 'val serverRoomName = serverRoomOverride ?: actualRoom'

content = re.sub(pattern, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Removed empty string lobby logic")
