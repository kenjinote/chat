import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_spacer = 'Spacer(modifier = Modifier.width(16.dp))'
content = content.replace(old_spacer, '')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Removed spacer between color button and speak button")
