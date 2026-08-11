import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the connection header UI
pattern = r'// Connection Header\s*var ipAddress by remember \{ mutableStateOf\("chat-lb-2026\.japaneast\.cloudapp\.azure\.com"\) \}\s*Row\(modifier = Modifier\.fillMaxWidth\(\)\.padding\(bottom = 8\.dp\), verticalAlignment = Alignment\.CenterVertically\) \{\s*Text\("Server:", style = win95TextStyle\)\s*Spacer\(modifier = Modifier\.width\(4\.dp\)\)\s*Win95TextField\(value = ipAddress, onValueChange = \{ ipAddress = it \}, modifier = Modifier\.weight\(1f\)\)\s*Spacer\(modifier = Modifier\.width\(8\.dp\)\)\s*Win95Button\(onClick = \{ viewModel\.connect\(ipAddress\) \}\) \{\s*Text\(if \(connectionState == "Disconnected" \|\| connectionState == "Failed"\) "Connect" else connectionState, style = win95TextStyle\)\s*\}\s*\}'
content = re.sub(pattern, '', content)

# Restore the LaunchedEffect
effect = '''
    LaunchedEffect(Unit) {
        viewModel.connect("chat-lb-2026.japaneast.cloudapp.azure.com")
    }
'''
# Find where to put it (after uriHandler)
uri_pattern = r'(val uriHandler = LocalUriHandler\.current\s*)'
content = re.sub(uri_pattern, r'\1' + effect, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Reverted UI changes and hardcoded production server")
