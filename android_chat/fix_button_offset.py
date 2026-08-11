import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add offset to content inside Win95Button
old_content_call = '        contentAlignment = Alignment.Center\n    ) {\n        content()\n    }'
new_content_call = '''        contentAlignment = Alignment.Center\n    ) {\n        Box(modifier = Modifier.offset(x = if (checked) 1.dp else 0.dp, y = if (checked) 1.dp else 0.dp)) {\n            content()\n        }\n    }'''
content = content.replace(old_content_call, new_content_call)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added 1dp offset to pressed button content")
