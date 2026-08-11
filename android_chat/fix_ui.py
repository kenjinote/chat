import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace:
# Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth(), backgroundColor = Color.White) {
# With:
# Win95Panel(modifier = Modifier.weight(1f).fillMaxHeight(), backgroundColor = Color.White) {

content = content.replace(
    'Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth(), backgroundColor = Color.White) {',
    'Win95Panel(modifier = Modifier.weight(1f).fillMaxHeight(), backgroundColor = Color.White) {'
)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Added fillMaxHeight to right panel")
