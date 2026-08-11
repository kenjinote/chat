import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# Update UserItem constructor to cast to Int
content = content.replace('message.nclr ?: -1', '(message.nclr ?: -1L).toInt()')
content = content.replace('u.nclr ?: -1', '(u.nclr ?: -1L).toInt()')

# Add serverRoomName parameter to connect
content = content.replace(
    'fun connect(ip: String, isPrivate: Boolean = false, room: String = "ロビー") {',
    'fun connect(ip: String, isPrivate: Boolean = false, room: String = "ロビー", serverRoomOverride: String? = null) {'
)

# Use serverRoomOverride
content = content.replace(
    'val joinRoomStr = if (room == "ロビー") "" else room',
    'val joinRoomStr = serverRoomOverride ?: (if (room == "ロビー") "" else room)'
)

# Fix redirect logic
old_redirect = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: currentRoom
                    isReconnecting = true
                    networkClient.disconnect()
                    viewModelScope.launch {
                        connect(newIp, false, redirectRoom)
                    }
                    return false
                }
            }'''
new_redirect = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: ""
                    isReconnecting = true
                    networkClient.disconnect()
                    viewModelScope.launch {
                        connect(newIp, false, currentRoom, redirectRoom)
                    }
                    return false
                }
            }'''
content = content.replace(old_redirect, new_redirect)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed ChatViewModel")
