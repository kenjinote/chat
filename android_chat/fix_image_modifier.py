import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('contentDescription = "TV", modifier = Modifier.fillMaxSize()', 'contentDescription = "TV", modifier = Modifier.fillMaxHeight()')
content = content.replace('contentDescription = "User", modifier = Modifier.fillMaxSize()', 'contentDescription = "User", modifier = Modifier.fillMaxHeight()')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Changed Image modifiers to fillMaxHeight().")
