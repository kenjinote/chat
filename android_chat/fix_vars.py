import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = re.sub(r'\s*var volumeLevel by remember \{ mutableStateOf\(50f\) \}', '', content)
content = re.sub(r'\s*var isVideoOn by remember \{ mutableStateOf\(false\) \}', '', content)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Removed unused vars")
