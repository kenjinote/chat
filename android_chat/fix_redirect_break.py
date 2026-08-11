import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update startListening to check return value
content = content.replace(
    'handleIncomingMessage(message)',
    'if (!handleIncomingMessage(message)) break'
)

# 2. Update handleIncomingMessage signature
content = content.replace(
    'private fun handleIncomingMessage(message: ChatMessage) {',
    'private fun handleIncomingMessage(message: ChatMessage): Boolean {'
)

# 3. Add return true at the end of handleIncomingMessage
content = content.replace(
    '        }\n    }\n\n    private fun appendSystemMessage',
    '        }\n        return true\n    }\n\n    private fun appendSystemMessage'
)

# 4. Update redirect logic to return false
old_redirect = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: currentRoom
                    isReconnecting = true
                    networkClient.disconnect()
                    connect(newIp, false, redirectRoom)
                }
            }'''
new_redirect = '''            "redirect" -> {
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
content = content.replace(old_redirect, new_redirect)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed redirect loop break")
