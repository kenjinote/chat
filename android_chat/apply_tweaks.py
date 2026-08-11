import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove outer Win95Panel
content = content.replace('Win95Panel(modifier = Modifier.fillMaxSize().padding(4.dp)) {', 'Box(modifier = Modifier.fillMaxSize().background(Win95Gray)) {')

# Remove Column padding
content = content.replace('Column(modifier = Modifier.fillMaxSize().padding(8.dp)) {', 'Column(modifier = Modifier.fillMaxSize()) {')

# Change chat messages LazyColumn
target_chat = 'LazyColumn(modifier = Modifier.fillMaxSize().padding(0.5.dp)) {\n                          items(messages)'
replacement_chat = 'LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {\n                          items(messages)'
content = content.replace(target_chat, replacement_chat)

# Change user list LazyColumn and sort items
target_users = '''                          LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                              items(users) { user ->'''
replacement_users = '''                          LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {
                              items(users.sortedBy { it.userName }) { user ->'''
content = content.replace(target_users, replacement_users)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied tweaks")
