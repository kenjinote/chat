import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_mod = 'modifier = Modifier.padding(end=8.dp).fillMaxHeight(),'
new_mod = 'modifier = Modifier.padding(end=8.dp),'
content = content.replace(old_mod, new_mod)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Removed fillMaxHeight from speak button so it can define row height")
