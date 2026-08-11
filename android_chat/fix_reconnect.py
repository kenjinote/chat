import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
'''    fun connect(ip: String, isPrivate: Boolean = false, room: String = "ロビー") {
        if (_connectionState.value == "Connected" || _connectionState.value == "Connecting...") {
            return
        }''',
'''    fun connect(ip: String, isPrivate: Boolean = false, room: String = "ロビー") {
        if ((_connectionState.value == "Connected" || _connectionState.value == "Connecting...") && !isReconnecting) {
            return
        }'''
)

# And fix the redirect logic
old_redirect = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: currentRoom
                    viewModelScope.launch {
                        networkClient.disconnect()
                        connect(newIp, false, redirectRoom)
                    }
                }
            }'''
new_redirect = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: currentRoom
                    isReconnecting = true
                    networkClient.disconnect()
                    connect(newIp, false, redirectRoom)
                }
            }'''
content = content.replace(old_redirect, new_redirect)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed reconnect logic")
