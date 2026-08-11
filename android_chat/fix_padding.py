import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace padding for messages
old_msg_padding = 'Box(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {'
new_msg_padding = 'Box(modifier = Modifier.fillMaxWidth().padding(start = 0.dp, end = 8.dp, top = 4.dp, bottom = 4.dp)) {'
content = content.replace(old_msg_padding, new_msg_padding)

# Replace padding for users
old_usr_padding = 'Box(modifier = Modifier.fillMaxWidth().padding(horizontal = 4.dp, vertical = 2.dp)) {'
new_usr_padding = 'Box(modifier = Modifier.fillMaxWidth().padding(start = 4.dp, end = 8.dp, top = 2.dp, bottom = 2.dp)) {'
content = content.replace(old_usr_padding, new_usr_padding)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated padding to prevent text overlap with scrollbar")
