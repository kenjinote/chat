import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add imports if missing
if 'import androidx.compose.ui.draw.drawWithContent' not in content:
    content = content.replace('import androidx.compose.ui.draw.drawBehind', 'import androidx.compose.ui.draw.drawBehind\nimport androidx.compose.ui.draw.drawWithContent\nimport androidx.compose.ui.graphics.drawscope.drawIntoCanvas')

# Fix the win95Scrollbar function definition
old_scrollbar_start = '): androidx.compose.ui.Modifier = androidx.compose.ui.draw.drawWithContent {'
new_scrollbar_start = '): androidx.compose.ui.Modifier = this.drawWithContent {'
content = content.replace(old_scrollbar_start, new_scrollbar_start)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed win95Scrollbar syntax in MainActivity.kt")
