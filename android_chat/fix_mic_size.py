import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = 'Modifier.size(16.dp).padding(end = 4.dp)'
new_code = 'Modifier.size(32.dp).padding(end = 4.dp)'
content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Increased mic icon size to 32.dp")
