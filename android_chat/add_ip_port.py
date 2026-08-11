import re

main = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'val msg: String? = null,',
    'val msg: String? = null,\n    val ip: String? = null,\n    val port: Int? = null,'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added ip and port to ChatMessage")
