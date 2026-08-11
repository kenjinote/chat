import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    private var decoder: MediaCodec? = null'''

new_code = '''    private var decoder: MediaCodec? = null
    private var activeSurface: Surface? = null'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replaced variables successfully.")
else:
    print("Could not find variables.")
