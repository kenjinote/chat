import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('setZOrderMediaOverlay(true)', 'setZOrderOnTop(true)')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

path2 = 'app/src/main/java/com/example/chat/VideoComponents.kt'
with open(path2, 'r', encoding='utf-8') as f:
    content2 = f.read()

content2 = content2.replace('setZOrderMediaOverlay(true)', 'setZOrderOnTop(true)')

with open(path2, 'w', encoding='utf-8') as f:
    f.write(content2)

print("Changed to setZOrderOnTop(true)")
