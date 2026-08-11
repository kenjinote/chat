import re

main = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('val nclr: Int? = null,', 'val nclr: Long? = null,')
content = content.replace('val nclr: Int? = null', 'val nclr: Long? = null')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed nclr to Long")
