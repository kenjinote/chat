import re

file_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# Add imports if they don't exist
imports_to_add = """import androidx.compose.ui.platform.LocalView
import android.app.Activity
import androidx.core.view.WindowCompat
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.toArgb
"""

if "import androidx.core.view.WindowCompat" not in content:
    content = content.replace("import android.view.SurfaceView\n", "import android.view.SurfaceView\n" + imports_to_add)

old_theme = """@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        content = content
    )
}"""

new_theme = """@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = Color(0xFF000080).toArgb()
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = false
        }
    }
    MaterialTheme(
        content = content
    )
}"""

content = content.replace(old_theme, new_theme)

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)

print("Status bar theme patched!")
