import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add Log import if not exists
if 'import android.util.Log' not in content:
    content = content.replace('import androidx.compose.ui.window.Dialog', 'import androidx.compose.ui.window.Dialog\nimport android.util.Log')

content = content.replace('videoEngine.initDecoder(h.surface)', 'Log.d("Win95VideoWindow", "SurfaceCreated! Calling initDecoder")\n                                    videoEngine.initDecoder(h.surface)')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added logs to MainActivity.kt")
