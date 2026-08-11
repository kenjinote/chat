import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Change default width
content = content.replace('width: androidx.compose.ui.unit.Dp = 16.dp', 'width: androidx.compose.ui.unit.Dp = 8.dp')

# Change border width inside win95Scrollbar
content = content.replace('val bw = 2.dp.toPx()', 'val bw = 1.dp.toPx()')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scrollbar width to 8.dp and border to 1.dp")
