import re
main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

target = '''fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    val connectionState by viewModel.connectionState.collectAsState()'''
replacement = '''fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    LaunchedEffect(Unit) {
        viewModel.connect("chat-lb-2026.japaneast.cloudapp.azure.com")
    }
    val connectionState by viewModel.connectionState.collectAsState()'''
content = content.replace(target, replacement)

content = content.replace('var isVideoOn by remember { mutableStateOf(true) }', 'var isVideoOn by remember { mutableStateOf(false) }')

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Added LaunchedEffect")
