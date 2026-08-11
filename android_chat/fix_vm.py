import re

vm = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(vm, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('val watchingUserId = MutableStateFlow<String?>(null)\n    fun watchVideo(userId: String?) { watchingUserId.value = userId }\n', '')
with open(vm, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed VM")
