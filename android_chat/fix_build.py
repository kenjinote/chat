import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Add activeSurface
if "private var activeSurface" not in content:
    content = content.replace("private var decoder: MediaCodec? = null", 
        "private var decoder: MediaCodec? = null\n    private var activeSurface: android.view.Surface? = null")

# Add assignment
if "activeSurface = surface" not in content:
    content = content.replace("fun initDecoder(surface: android.view.Surface, width: Int = 640, height: Int = 480) {",
        "fun initDecoder(surface: android.view.Surface, width: Int = 640, height: Int = 480) {\n        activeSurface = surface")

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(content)
print("Fixed VideoEngine.kt")
