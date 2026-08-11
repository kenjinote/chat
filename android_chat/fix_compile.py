import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))',
    'val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)\n                appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))'
)
content = content.replace(
    'val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)\n                val serverRoomName',
    'val serverRoomName'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed ChatViewModel compile issue")
