import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# I want to add an IP address input at the very top of the Column in Win95Panel
# Right now, Win95Panel has Column(modifier = Modifier.padding(12.dp)) {
#   Row(modifier = Modifier.weight(1f)) { ... }
# Let's find that.

pattern = r'(Column\(modifier = Modifier.padding\(12\.dp\)\) \{\s*)(Row\(modifier = Modifier.weight\(1f\)\) \{)'
replacement = r'''\1
            // Connection Header
            var ipAddress by remember { mutableStateOf("chat-lb-2026.japaneast.cloudapp.azure.com") }
            Row(modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                Text("Server:", style = win95TextStyle)
                Spacer(modifier = Modifier.width(4.dp))
                Win95TextField(value = ipAddress, onValueChange = { ipAddress = it }, modifier = Modifier.weight(1f))
                Spacer(modifier = Modifier.width(8.dp))
                Win95Button(onClick = { viewModel.connect(ipAddress) }) {
                    Text(if (connectionState == "Disconnected" || connectionState == "Failed") "Connect" else connectionState, style = win95TextStyle)
                }
            }
            \2'''
            
content = re.sub(pattern, replacement, content)

# I should also remove the hardcoded LaunchedEffect connect
launched_effect_pattern = r'LaunchedEffect\(Unit\) \{\s*viewModel\.connect\("chat-lb-2026\.japaneast\.cloudapp\.azure\.com"\)\s*\}'
content = re.sub(launched_effect_pattern, '', content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated MainActivity.kt with Server IP input")
