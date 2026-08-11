import re

file_path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Chat padding
content = content.replace(
    'LazyColumn(modifier = Modifier.fillMaxSize().padding(8.dp)) {',
    'LazyColumn(modifier = Modifier.fillMaxSize().padding(0.5.dp)) {',
    1
)

# 2. User list padding
content = content.replace(
    'LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {',
    'LazyColumn(modifier = Modifier.fillMaxSize().padding(0.5.dp)) {',
    1
)

# 3. User items prefix and font size and padding
target_user = '''                                val displayUser = if (isMe) baseDisplayUser + stringResource(R.string.you_suffix) else baseDisplayUser
                                val isMenuExpanded = menuExpandedUserId == user.userId'''
replacement_user = '''                                val displayUser = if (isMe) baseDisplayUser + stringResource(R.string.you_suffix) else baseDisplayUser
                                val videoStreamers by viewModel.videoStreamers.collectAsState()
                                val isStreaming = videoStreamers[user.userId] ?: false
                                val prefix = "😊" + (if(isStreaming) "📺" else "")
                                val finalDisplayUser = prefix + displayUser
                                val isMenuExpanded = menuExpandedUserId == user.userId'''
content = content.replace(target_user, replacement_user)

target_text = '''                                    Text(
                                        text = displayUser,
                                        style = win95TextStyle.copy(color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
                                        modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp).fillMaxWidth()
                                    )'''
replacement_text = '''                                    Text(
                                        text = finalDisplayUser,
                                        style = win95TextStyle.copy(fontSize = 16.sp, color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
                                        modifier = Modifier.padding(horizontal = 0.5.dp, vertical = 0.5.dp).fillMaxWidth()
                                    )'''
content = content.replace(target_text, replacement_text)

# 4. Watch video click
target_watch = '''                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                                onClick = { menuExpandedUserId = null }
                                            )'''
replacement_watch = '''                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                                onClick = { 
                                                    viewModel.watchVideo(user.userId)
                                                    menuExpandedUserId = null 
                                                }
                                            )'''
content = content.replace(target_watch, replacement_watch)

# 5. Add Win95VideoWindow and watchingUserId
target_chat = '''fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    val connectionState by viewModel.connectionState.collectAsState()
    val messages by viewModel.messages.collectAsState()
    val uriHandler = LocalUriHandler.current'''
replacement_chat = '''fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    val connectionState by viewModel.connectionState.collectAsState()
    val messages by viewModel.messages.collectAsState()
    val watchingUserId by viewModel.watchingUserId.collectAsState()
    val uriHandler = LocalUriHandler.current'''
content = content.replace(target_chat, replacement_chat)

target_win95 = '''    Win95Panel(modifier = Modifier.fillMaxSize().padding(4.dp)) {
        Column(modifier = Modifier.fillMaxSize().padding(8.dp)) {'''
replacement_win95 = '''    Win95Panel(modifier = Modifier.fillMaxSize().padding(4.dp)) {
        if (watchingUserId != null) {
            Win95VideoWindow(
                title = "Video - ",
                onClose = { viewModel.watchVideo(null) },
                videoEngine = viewModel.videoEngine
            )
        }
        Column(modifier = Modifier.fillMaxSize().padding(8.dp)) {'''
content = content.replace(target_win95, replacement_win95)


# 6. Sysmsg color fix
target_sysmsg = '''                                val msgColor = if (msg.type == "system" || msg.type == "sysmsg") {
                                    Color.Red
                                } else {
                                    if (msg.clr != null) Color(msg.clr) else Win95Black
                                }'''
replacement_sysmsg = '''                                val msgColor = if (msg.type == "system" || msg.type == "sysmsg") {
                                    Win95DarkGray
                                } else {
                                    if (msg.clr != null) Color(msg.clr) else Win95Black
                                }'''
content = content.replace(target_sysmsg, replacement_sysmsg)


with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Patch applied")
