import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''            androidx.compose.ui.graphics.drawscope.drawIntoCanvas { canvas ->
                val paint = androidx.compose.ui.graphics.Paint().apply { color = androidx.compose.ui.graphics.Color(0xFFDFDFDF) }
                canvas.drawRect(androidx.compose.ui.geometry.Rect(trackX, 0f, size.width, scrollbarHeight), paint)
            }'''

new_code = '''            drawRect(
                color = androidx.compose.ui.graphics.Color(0xFFDFDFDF),
                topLeft = androidx.compose.ui.geometry.Offset(trackX, 0f),
                size = androidx.compose.ui.geometry.Size(width.toPx(), scrollbarHeight)
            )'''

content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Removed drawIntoCanvas")
