@Composable
fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    val connectionState by viewModel.connectionState.collectAsState()
    val messages by viewModel.messages.collectAsState()
    val uriHandler = LocalUriHandler.current

    var inputText by remember { mutableStateOf("") }
    var isVideoOn by remember { mutableStateOf(true) }
    var isTalkOn by remember { mutableStateOf(false) }
    var volumeLevel by remember { mutableStateOf(50f) }
    var showMoveRoomDialog by remember { mutableStateOf(false) }
    
    // Formatting states
    var isBold by remember { mutableStateOf(false) }
    var isItalic by remember { mutableStateOf(false) }
    var isUnderline by remember { mutableStateOf(false) }
    var selectedSize by remember { mutableStateOf("10") }
    var selectedColor by remember { mutableStateOf(Win95Black) }
    var showColorPicker by remember { mutableStateOf(false) }
    
    val fontSizes = listOf("8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28", "36", "48", "72")

    if (showMoveRoomDialog) {
        MoveRoomDialog(
            viewModel = viewModel,
            onDismiss = { showMoveRoomDialog = false },
            onConfirm = { roomName, isPrivate ->
                showMoveRoomDialog = false
                viewModel.moveRoom(roomName, isPrivate)
            }
        )
    }

    if (showColorPicker) {
        ColorPickerDialog(
            onColorSelected = { col ->
                selectedColor = col
                showColorPicker = false
            },
            onDismiss = { showColorPicker = false }
        )
    }

    Win95Panel(modifier = Modifier.fillMaxSize().padding(4.dp)) {
        Column(modifier = Modifier.fillMaxSize().padding(8.dp)) {
            // Main Chat Area
            Row(modifier = Modifier.weight(1f).fillMaxWidth()) {
                // Left: Chat Messages
                Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight()) {
                    LazyColumn(modifier = Modifier.fillMaxSize().padding(8.dp)) {
                        items(messages) { msg ->
                            Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                                if (msg.type != "system" && msg.type != "sysmsg") {
                                    val uName = msg.userName ?: "Unknown"
                                    val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                    val nclr = msg.nclr ?: -1
                                    val userColor = if (nclr != -1) {
                                        val b = (nclr shr 16) and 0xFF
                                        val g = (nclr shr 8) and 0xFF
                                        val r = nclr and 0xFF
                                        Color(r, g, b, 255)
                                    } else Win95Black
                                    
                                    Text(text = "${displayUser}: ", fontWeight = FontWeight.Bold, color = userColor, style = win95TextStyle)
                                }
                                
                                val contentToDisplay = msg.msg ?: msg.content ?: ""
                                val annotatedStr = TextProcessor.formatMessage(contentToDisplay)
                                
                                val msgColor = if (msg.type == "system" || msg.type == "sysmsg") Win95DarkGray else {
                                    val c = msg.clr ?: 0
                                    if (c != 0) {
                                        val b = (c shr 16) and 0xFF
                                        val g = (c shr 8) and 0xFF
                                        val r = c and 0xFF
                                        Color(r, g, b, 255)
                                    } else Win95Black
                                }
                                
                                val msgFontSize = (msg.sz ?: 10).sp * 2
                                val msgFontWeight = if (msg.b == true) FontWeight.Bold else FontWeight.Normal
                                val msgFontStyle = if (msg.i == true) FontStyle.Italic else FontStyle.Normal
                                
                                ClickableText(
                                    text = annotatedStr,
                                    style = win95TextStyle.copy(
                                        color = msgColor,
                                        fontSize = msgFontSize,
                                        fontWeight = msgFontWeight,
                                        fontStyle = msgFontStyle
                                    ),
                                    onClick = { offset ->
                                        annotatedStr.getStringAnnotations(tag = "URL", start = offset, end = offset)
                                            .firstOrNull()?.let { annotation -> uriHandler.openUri(annotation.item) }
                                    }
                                )
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.width(4.dp))

                // Right: User List
                Column(modifier = Modifier.weight(3f).fillMaxHeight()) {
                    val users by viewModel.users.collectAsState()
                    val ignoredUsers by viewModel.ignoredUserIds.collectAsState()
                    Text(stringResource(R.string.user_list_fmt, users.size), fontWeight = FontWeight.Bold, style = win95TextStyle)
                    Spacer(modifier = Modifier.height(4.dp))
                    
                    var menuExpandedUserId by remember { mutableStateOf<String?>(null) }
                    
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {
                        LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                            items(users) { user ->
                                val uName = user.userName
                                val baseDisplayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val isMe = user.userId == viewModel.myUserId
                                val displayUser = if (isMe) baseDisplayUser + stringResource(R.string.you_suffix) else baseDisplayUser
                                val isMenuExpanded = menuExpandedUserId == user.userId
                                
                                val baseColor = if (user.nclr != null && user.nclr != -1) {
                                    val b = (user.nclr shr 16) and 0xFF
                                    val g = (user.nclr shr 8) and 0xFF
                                    val r = user.nclr and 0xFF
                                    Color(r, g, b, 255)
                                } else Win95Black
                                
                                val textColor = if (isMenuExpanded) Color.White else baseColor
                                val bgColor = if (isMenuExpanded) Color(0xFF000080) else Color.Transparent

                                Box(modifier = Modifier.fillMaxWidth().background(bgColor).clickable {
                                    menuExpandedUserId = user.userId
                                }) {
                                    Text(
                                        text = displayUser,
                                        style = win95TextStyle.copy(color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
                                        modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp).fillMaxWidth()
                                    )
                                    
                                    DropdownMenu(
                                        expanded = isMenuExpanded,
                                        onDismissRequest = { menuExpandedUserId = null },
                                        modifier = Modifier.background(Win95Gray)
                                            .drawBehind {
                                                val bw = 2.dp.toPx()
                                                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                                                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                                                drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                                                drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                                            }.padding(2.dp)
                                    ) {
                                        if (isMe) {
                                            DropdownMenuItem(
                                                text = { Text(if (isVideoOn) "✔ " + stringResource(R.string.share_video) else stringResource(R.string.share_video), style = win95TextStyle) },
                                                onClick = {
                                                    menuExpandedUserId = null
                                                    isVideoOn = !isVideoOn
                                                }
                                            )
                                        } else {
                                            val isIgnored = ignoredUsers.contains(user.userId)
                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                                onClick = { menuExpandedUserId = null }
                                            )
                                            Divider(color = Win95DarkGray, thickness = 1.dp)
                                            Divider(color = Win95White, thickness = 1.dp)
                                            DropdownMenuItem(
                                                text = { Text(if (isIgnored) "✔ " + stringResource(R.string.ignore_user) else stringResource(R.string.ignore_user), style = win95TextStyle) },
                                                onClick = { 
                                                    menuExpandedUserId = null 
                                                    viewModel.toggleIgnoreUser(user.userId)
                                                }
                                            )
                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.report_user), style = win95TextStyle) },
                                                onClick = { menuExpandedUserId = null }
                                            )
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    Win95Button(onClick = { showMoveRoomDialog = true }, modifier = Modifier.fillMaxWidth()) {
                        Text(stringResource(R.string.move_room_btn), style = win95TextStyle)
                    }
                }
            }

            Spacer(modifier = Modifier.height(4.dp))

            // Toolbar Area
            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                Win95Button(onClick = { isBold = !isBold }, checked = isBold, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle.copy(fontWeight = FontWeight.Bold)) }
                Win95Button(onClick = { isItalic = !isItalic }, checked = isItalic, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle.copy(fontStyle = FontStyle.Italic)) }
                Win95Button(onClick = { isUnderline = !isUnderline }, checked = isUnderline, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle) } 
                
                Win95ComboBox(items = fontSizes, selectedItem = selectedSize, onItemSelected = { selectedSize = it }, modifier = Modifier.padding(end=4.dp))
                
                Win95Button(onClick = { showColorPicker = true }, modifier = Modifier.padding(end=4.dp)) { 
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Col", style = win95TextStyle)
                        Spacer(modifier = Modifier.width(4.dp))
                        Box(modifier = Modifier.size(16.dp).background(selectedColor).border(1.dp, Win95Black))
                    }
                }
                
                Spacer(modifier = Modifier.width(16.dp))
                
                Win95Button(onClick = { isTalkOn = !isTalkOn }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isTalkOn) "Talk" else "Mute", style = win95TextStyle) 
                }
                Win95Button(onClick = { isVideoOn = !isVideoOn }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isVideoOn) "Vid:ON" else "Vid:OFF", style = win95TextStyle) 
                }
                Text("Vol:", style = win95TextStyle)
                Slider(value = volumeLevel, onValueChange = { volumeLevel = it }, valueRange = 0f..100f, modifier = Modifier.width(100.dp))
            }

            Spacer(modifier = Modifier.height(4.dp))

            // Input Area
            Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                Win95TextField(
                    value = inputText,
                    onValueChange = { inputText = it },
                    modifier = Modifier.weight(1f),
                    textStyle = win95TextStyle.copy(
                        color = selectedColor,
                        fontWeight = if (isBold) FontWeight.Bold else FontWeight.Normal,
                        fontStyle = if (isItalic) FontStyle.Italic else FontStyle.Normal,
                        fontSize = (selectedSize.toIntOrNull() ?: 10).sp * 2
                    )
                )
                Spacer(modifier = Modifier.width(8.dp))
                Win95Button(
                    onClick = { 
                        if (inputText.isNotBlank()) {
                            val r = (selectedColor.red * 255).toInt()
                            val g = (selectedColor.green * 255).toInt()
                            val b = (selectedColor.blue * 255).toInt()
                            val cInt = (r shl 16) or (g shl 8) or b
                            viewModel.sendMessage(
                                text = inputText,
                                isBold = isBold,
                                isItalic = isItalic,
                                isUnderline = isUnderline,
                                fontSize = selectedSize.toIntOrNull() ?: 10,
                                colorInt = cInt
                            )
                            inputText = ""
                        }
                    }
                ) {
                    Text("Send", style = win95TextStyle)
                }
            }
        }
    }
}

