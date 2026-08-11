import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('Box(modifier = Modifier.size(16.dp)) {', 'Box(modifier = Modifier.size(32.dp)) {')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated user icon size to 32.dp")
