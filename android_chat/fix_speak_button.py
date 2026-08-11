import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

if 'import androidx.compose.ui.res.painterResource' not in content:
    content = content.replace('import androidx.compose.ui.Modifier', 'import androidx.compose.ui.Modifier\nimport androidx.compose.ui.res.painterResource')

old_code = 'Text("話す", style = win95TextStyle, modifier = Modifier.padding(horizontal = 8.dp))'
new_code = '''Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(horizontal = 8.dp)) {
                        Icon(painter = painterResource(id = R.drawable.ic_mic), contentDescription = "Mic", modifier = Modifier.size(16.dp).padding(end = 4.dp), tint = Color.Black)
                        Text("話す", style = win95TextStyle)
                    }'''

content = content.replace(old_code, new_code)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added mic icon to speak button")
