import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

if 'import androidx.compose.foundation.layout.offset' not in content:
    content = content.replace('import androidx.compose.foundation.layout.*', 'import androidx.compose.foundation.layout.*\nimport androidx.compose.foundation.layout.offset')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Checked imports")
