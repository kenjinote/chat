@Composable
fun ColorPickerDialog(onColorSelected: (Color) -> Unit, onDismiss: () -> Unit) {
    var customR by remember { mutableStateOf(0f) }
    var customG by remember { mutableStateOf(0f) }
    var customB by remember { mutableStateOf(0f) }

    val palette = listOf(
        Color(200, 50, 50), Color(50, 150, 50), Color(50, 50, 200),
        Color(180, 180, 50), Color(180, 50, 180), Color(50, 180, 180),
        Color(255, 128, 0), Color(128, 0, 255), Color(0, 128, 255),
        Color(128, 200, 0), Color(255, 0, 128), Color(0, 200, 128),
        Color.Black, Color.DarkGray, Color.Gray, Color.LightGray
    )
    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .background(Win95Gray)
                .drawBehind {
                    val bw = 2.dp.toPx()
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                    drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                    drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                }
                .padding(8.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth().background(Win95TitleBlue).padding(4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Color", color = Win95White, style = win95TextStyle.copy(fontWeight = FontWeight.Bold))
            }
            Spacer(modifier = Modifier.height(8.dp))
            Column {
                for (row in 0 until 4) {
                    Row {
                        for (colIdx in 0 until 4) {
                            val col = palette[row * 4 + colIdx]
                            Box(modifier = Modifier
                                .size(32.dp).padding(2.dp).background(col).border(1.dp, Win95Black)
                                .clickable { onColorSelected(col) }
                            )
                        }
                    }
                }
            }
            Spacer(modifier = Modifier.height(8.dp))
            Text("Custom Color:", style = win95TextStyle)
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("R", style = win95TextStyle, modifier = Modifier.width(24.dp))
                Slider(value = customR, onValueChange = { customR = it }, modifier = Modifier.weight(1f))
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("G", style = win95TextStyle, modifier = Modifier.width(24.dp))
                Slider(value = customG, onValueChange = { customG = it }, modifier = Modifier.weight(1f))
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("B", style = win95TextStyle, modifier = Modifier.width(24.dp))
                Slider(value = customB, onValueChange = { customB = it }, modifier = Modifier.weight(1f))
            }
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Box(modifier = Modifier.size(32.dp).background(Color(customR, customG, customB)).border(1.dp, Win95Black))
                Win95Button(onClick = { onColorSelected(Color(customR, customG, customB)) }) {
                    Text("OK", style = win95TextStyle)
                }
            }
        }
    }
}

@Composable
fun MoveRoomDialog(viewModel: ChatViewModel, onDismiss: () -> Unit, onConfirm: (String, Boolean) -> Unit) {
    var roomNameInput by remember { mutableStateOf("") }
    var isPrivate by remember { mutableStateOf(false) }
    val roomList by viewModel.roomList.collectAsState()
    val filteredRooms = roomList.filter { it.name.contains(roomNameInput, ignoreCase = true) }
    
    DisposableEffect(Unit) {
        viewModel.requestRooms()
        onDispose { viewModel.closeRooms() }
    }

    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier.width(320.dp).background(Win95Gray)
                .drawBehind {
                    val bw = 2.dp.toPx()
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                    drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                    drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                }
                .padding(2.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth().background(Color(0xFF000080)).padding(horizontal = 4.dp, vertical = 2.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(text = stringResource(R.string.move_room_btn), color = Color.White, fontWeight = FontWeight.Bold, style = win95TextStyle)
            }
            Column(modifier = Modifier.padding(8.dp)) {
                Win95Panel(modifier = Modifier.fillMaxWidth().background(Win95White)) {
                    BasicTextField(
                        value = roomNameInput, onValueChange = { roomNameInput = it },
                        modifier = Modifier.fillMaxWidth().padding(4.dp), textStyle = win95TextStyle, singleLine = true, cursorBrush = SolidColor(Win95Black)
                    )
                }
                Spacer(modifier = Modifier.height(8.dp))
                Win95Panel(modifier = Modifier.fillMaxWidth().height(200.dp).background(Win95White)) {
                    LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                        items(filteredRooms) { room ->
                            Text(
                                text = " ()", style = win95TextStyle,
                                modifier = Modifier.fillMaxWidth().clickable { roomNameInput = room.name }.padding(vertical = 2.dp)
                            )
                        }
                    }
                }
                Spacer(modifier = Modifier.height(8.dp))
                Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = isPrivate, onCheckedChange = { isPrivate = it })
                        Text(stringResource(R.string.private_room), style = win95TextStyle)
                    }
                    Row {
                        Win95Button(onClick = { onConfirm(roomNameInput, isPrivate) }) { Text("OK", style = win95TextStyle) }
                        Spacer(modifier = Modifier.width(8.dp))
                        Win95Button(onClick = onDismiss) { Text("Cancel", style = win95TextStyle) }
                    }
                }
            }
        }
    }
}
