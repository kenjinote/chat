import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add import if missing
if 'import androidx.compose.ui.text.style.TextDecoration' not in content:
    content = content.replace('import androidx.compose.ui.text.font.FontStyle', 'import androidx.compose.ui.text.font.FontStyle\nimport androidx.compose.ui.text.style.TextDecoration')

old_code = 'Text("U", style = win95TextStyle)'
new_code = 'Text("U", style = win95TextStyle.copy(textDecoration = TextDecoration.Underline))'
content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated U button to have underline")
