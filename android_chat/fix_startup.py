import re

main = 'app/src/main/java/com/example/chat/network/MediaHeader.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('buffer.put(0)', 'buffer.put(0.toByte())')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed MediaHeader.kt buffer.put(0.toByte())")

main2 = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main2, 'r', encoding='utf-8') as f:
    content2 = f.read()

content2 = content2.replace('room: String = "ロビー"', 'room: String? = null')
content2 = content2.replace('currentRoom = room', 'currentRoom = room ?: getApplication<Application>().getString(R.string.lobby)')

# Update the connection logic to use localized lobby string
old_serverRoomName = 'val serverRoomName = serverRoomOverride ?: (if (room == "ロビー") "" else room)'
new_serverRoomName = 'val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)\n                val serverRoomName = serverRoomOverride ?: (if (actualRoom == getApplication<Application>().getString(R.string.lobby)) "" else actualRoom)'
content2 = content2.replace(old_serverRoomName, new_serverRoomName)

# And in MainActivity, we need to make sure we don't pass anything for lobby so it defaults to null, or pass the localized string.
# But MainActivity doesn't pass room parameter at startup (it uses default).
# Let's fix the appendSystemMessage logic to use actualRoom
content2 = content2.replace(
    'appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, room))',
    'appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))'
)
content2 = content2.replace(
    'appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayMyName, room))',
    'appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayMyName, actualRoom))'
)

with open(main2, 'w', encoding='utf-8') as f:
    f.write(content2)

print("Fixed ChatViewModel.kt lobby string")
