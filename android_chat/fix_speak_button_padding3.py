import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_row = 'Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(horizontal = 2.dp))'
new_row = 'Row(verticalAlignment = Alignment.CenterVertically)'
content = content.replace(old_row, new_row)

old_icon = 'Modifier.size(32.dp).padding(end = 2.dp)'
new_icon = 'Modifier.size(32.dp)'
content = content.replace(old_icon, new_icon)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Removed all padding inside speak button")
