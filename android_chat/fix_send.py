import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('Text("Send", style = win95TextStyle)', 'Text(stringResource(R.string.send), style = win95TextStyle)')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed Send button text")
