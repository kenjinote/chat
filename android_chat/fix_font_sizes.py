import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

pattern = r'val fontSizes = listOf\([^)]+\)'
replacement = 'val fontSizes = listOf("6", "7", "8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "28", "32")'
content = re.sub(pattern, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated fontSizes list in MainActivity.kt")
