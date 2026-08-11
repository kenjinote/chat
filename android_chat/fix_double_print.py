import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# Add prevRoom variable
content = content.replace(
    'currentRoom = room ?: getApplication<Application>().getString(R.string.lobby)',
    'val prevRoom = currentRoom\n        currentRoom = room ?: getApplication<Application>().getString(R.string.lobby)'
)

# Update the print logic
old_print = '''                val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)
                appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))
                appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayMyName, actualRoom))'''

new_print = '''                val actualRoom = room ?: getApplication<Application>().getString(R.string.lobby)
                if (!wasReconnecting || actualRoom != prevRoom) {
                    appendSystemMessage(getApplication<Application>().getString(R.string.moved_to, actualRoom))
                    appendSystemMessage(getApplication<Application>().getString(R.string.room_enter_fmt, displayMyName, actualRoom))
                }'''

content = content.replace(old_print, new_print)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed double print")
