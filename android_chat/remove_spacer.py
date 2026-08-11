import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('Spacer(modifier = Modifier.width(4.dp))', '')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Removed Spacer next to the icon.")
