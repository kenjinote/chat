import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# Modify Win95Panel signature to accept backgroundColor
content = content.replace('fun Win95Panel(modifier: Modifier = Modifier, content: @Composable () -> Unit) {\n    Box(\n        modifier = modifier\n            .background(Win95Gray)', 'fun Win95Panel(modifier: Modifier = Modifier, backgroundColor: Color = Win95Gray, content: @Composable () -> Unit) {\n    Box(\n        modifier = modifier\n            .background(backgroundColor)')

# Update the Chat list and User list to use backgroundColor = Color.White
content = content.replace('Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight()) {', 'Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight(), backgroundColor = Color.White) {')
content = content.replace('Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {', 'Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth(), backgroundColor = Color.White) {')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated Win95Panel")
