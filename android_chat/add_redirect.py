import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

redirect_logic = '''            "redirect" -> {
                val newIp = message.ip
                if (!newIp.isNullOrEmpty()) {
                    val redirectRoom = message.roomName ?: currentRoom
                    viewModelScope.launch {
                        networkClient.disconnect()
                        connect(newIp, false, redirectRoom)
                    }
                }
            }
'''

content = content.replace(
    '"welcome" -> {',
    redirect_logic + '            "welcome" -> {'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added redirect logic")
