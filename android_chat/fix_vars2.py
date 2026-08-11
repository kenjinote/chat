import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('      var isTalkOn by remember { mutableStateOf(false) }', '      var isVideoOn by remember { mutableStateOf(false) }\n      var isTalkOn by remember { mutableStateOf(false) }')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Restored isVideoOn")
