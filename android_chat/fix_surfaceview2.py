import re

path = 'app/src/main/java/com/example/chat/VideoComponents.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

target = r'val surfaceView = android\.view\.SurfaceView\(ctx\)'
replacement = r'val surfaceView = android.view.SurfaceView(ctx).apply { setZOrderMediaOverlay(true) }'

content = re.sub(target, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added setZOrderMediaOverlay(true) to SurfaceView in VideoComponents.kt")
