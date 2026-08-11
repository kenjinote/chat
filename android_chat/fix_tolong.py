import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('nclr = myNameColor\n', 'nclr = myNameColor.toLong()\n')
content = content.replace('nclr = myNameColor)', 'nclr = myNameColor.toLong())')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed toLong in ChatViewModel")
