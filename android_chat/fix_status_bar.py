import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add missing imports if needed
if 'import android.app.Activity' not in content:
    content = content.replace('import android.os.Bundle', 'import android.os.Bundle\nimport android.app.Activity\nimport androidx.compose.ui.graphics.toArgb\nimport androidx.compose.ui.platform.LocalView')

old_theme = '''@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = lightColorScheme(
            background = Win95Gray,
            surface = Win95Gray
        ),
        content = content
    )
}'''

new_theme = '''@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = androidx.compose.ui.graphics.Color(0xFF000080).toArgb()
        }
    }
    MaterialTheme(
        colorScheme = lightColorScheme(
            background = Win95Gray,
            surface = Win95Gray
        ),
        content = content
    )
}'''

content = content.replace(old_theme, new_theme)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated status bar color to Win95 blue")
