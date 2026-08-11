import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

if 'import androidx.core.view.WindowCompat' not in content:
    content = content.replace('import androidx.compose.ui.platform.LocalView', 'import androidx.compose.ui.platform.LocalView\nimport androidx.core.view.WindowCompat')

old_code = 'window.statusBarColor = androidx.compose.ui.graphics.Color(0xFF000080).toArgb()'
new_code = '''window.statusBarColor = androidx.compose.ui.graphics.Color(0xFF000080).toArgb()
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = false'''

content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added WindowCompat for light status bar text")
