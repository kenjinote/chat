import re

path = 'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

target = r'decoder\.queueInputBuffer\(inputBufferId, 0, data\.size, 0, 0\)'
replacement = r'decoder.queueInputBuffer(inputBufferId, 0, data.size, System.nanoTime() / 1000, 0)'

content = re.sub(target, replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated decodeH264 to use System.nanoTime() / 1000 for PTS")
