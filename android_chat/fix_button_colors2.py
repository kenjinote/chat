import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('checkedBackgroundColor = Win95White', 'checkedBackgroundColor = Win95Gray')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Set all checked buttons to use Win95Gray background")
