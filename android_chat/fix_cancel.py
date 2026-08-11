import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('stringResource(R.string.cancel_btn)', '"Cancel"')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed cancel button text")
