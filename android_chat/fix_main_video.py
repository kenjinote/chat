import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('videoEngine.setRemoteSurface(h.surface)', 'videoEngine.initDecoder(h.surface)')
content = content.replace('videoEngine.setRemoteSurface(null)', 'videoEngine.stopDecoder()')

imports = '''import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.gestures.detectDragGestures'''

if 'pointerInput' not in content:
    content = content.replace('import androidx.compose.ui.Modifier', imports + '\nimport androidx.compose.ui.Modifier')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed MainActivity imports and VideoEngine methods")
