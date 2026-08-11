import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('R.drawable.ic_tv', 'R.drawable.ic_tv_win')
content = content.replace('R.drawable.ic_smile', 'R.drawable.ic_smile_win')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated MainActivity to use new VectorDrawables.")
