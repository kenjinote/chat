import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'val serverRoomName = serverRoomOverride ?: actualRoom',
    'val serverRoomName = serverRoomOverride ?: (if (actualRoom == getApplication<Application>().getString(R.string.lobby)) "" else actualRoom)'
)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Reverted empty string lobby logic")
