import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

target = r'android\.view\.SurfaceView\(ctx\)\.apply\s*\{'
replacement = r'android.view.SurfaceView(ctx).apply { setZOrderMediaOverlay(true)'

content = re.sub(target, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added setZOrderMediaOverlay(true) to SurfaceView")
