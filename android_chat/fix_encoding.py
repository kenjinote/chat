import re

path = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('"UTF-8"', '"Shift_JIS"')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated encoding to Shift_JIS")
