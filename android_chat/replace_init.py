import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        try {'''

new_code = '''    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        activeSurface = surface
        try {'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replaced initDecoder successfully.")
else:
    print("Could not find initDecoder.")
