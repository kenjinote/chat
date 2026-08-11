import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_icon = 'Modifier.size(32.dp)'
new_icon = 'Modifier.size(24.dp)'
content = content.replace(old_icon, new_icon)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Reduced mic icon size to 24.dp")
