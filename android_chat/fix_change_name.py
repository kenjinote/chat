import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add var showChangeNameDialog
content = content.replace('    var showMoveRoomDialog by remember { mutableStateOf(false) }', '    var showMoveRoomDialog by remember { mutableStateOf(false) }\n    var showChangeNameDialog by remember { mutableStateOf(false) }')

# 2. Add DropdownMenuItem for rename
target_menu = '''                                            DropdownMenuItem(
                                                text = { Text(if (isVideoOn) "✔ " + stringResource(R.string.share_video) else stringResource(R.string.share_video), style = win95TextStyle) },
                                                onClick = {
                                                    menuExpandedUserId = null
                                                    isVideoOn = !isVideoOn
                                                }
                                            )'''

replacement_menu = '''                                            DropdownMenuItem(
                                                text = { Text(if (isVideoOn) "✔ " + stringResource(R.string.share_video) else stringResource(R.string.share_video), style = win95TextStyle) },
                                                onClick = {
                                                    menuExpandedUserId = null
                                                    isVideoOn = !isVideoOn
                                                }
                                            )
                                            Divider(color = Win95DarkGray, thickness = 1.dp)
                                            Divider(color = Win95White, thickness = 1.dp)
                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.rename_menu), style = win95TextStyle) },
                                                onClick = {
                                                    menuExpandedUserId = null
                                                    showChangeNameDialog = true
                                                }
                                            )'''
content = content.replace(target_menu, replacement_menu)

# 3. Add ChangeNameDialog usage
target_usage = '''    if (showMoveRoomDialog) {
        MoveRoomDialog(
            viewModel = viewModel,
            onDismiss = { showMoveRoomDialog = false },
            onConfirm = { roomName, isPrivate ->
                showMoveRoomDialog = false
                viewModel.moveRoom(roomName, isPrivate)
            }
        )
    }'''

replacement_usage = '''    if (showMoveRoomDialog) {
        MoveRoomDialog(
            viewModel = viewModel,
            onDismiss = { showMoveRoomDialog = false },
            onConfirm = { roomName, isPrivate ->
                showMoveRoomDialog = false
                viewModel.moveRoom(roomName, isPrivate)
            }
        )
    }
    
    if (showChangeNameDialog) {
        ChangeNameDialog(
            initialName = viewModel.myUserName,
            initialColor = viewModel.myNameColor,
            onDismiss = { showChangeNameDialog = false },
            onConfirm = { newName, newColor ->
                showChangeNameDialog = false
                viewModel.rename(newName, newColor)
            }
        )
    }'''
content = content.replace(target_usage, replacement_usage)

# 4. Add ChangeNameDialog definition
change_name_def = '''
@Composable
fun ChangeNameDialog(
    initialName: String,
    initialColor: Int,
    onDismiss: () -> Unit,
    onConfirm: (String, Int) -> Unit
) {
    var newName by remember { mutableStateOf(initialName) }
    var selectedColor by remember { mutableStateOf(if (initialColor == -1) Win95Black else Color(initialColor or 0xFF000000.toInt())) }
    var showColorPicker by remember { mutableStateOf(false) }
    
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
                Text(text = stringResource(R.string.rename_menu), color = Color.White, fontWeight = FontWeight.Bold, style = win95TextStyle)
            }
            Column(modifier = Modifier.padding(16.dp)) {
                Win95TextField(
                    value = newName,
                    onValueChange = { newName = it },
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(modifier = Modifier.height(16.dp))
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                    Box(modifier = Modifier
                        .size(24.dp)
                        .background(selectedColor)
                        .border(1.dp, Win95Black)
                        .clickable { showColorPicker = true }
                    )
                    Row {
                        Win95Button(onClick = { 
                            val nclr = if (selectedColor == Win95Black) -1 else selectedColor.toArgb() and 0xFFFFFF
                            onConfirm(newName, nclr) 
                        }, modifier = Modifier.width(80.dp)) {
                            Text("OK", style = win95TextStyle)
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                        Win95Button(onClick = onDismiss, modifier = Modifier.width(80.dp)) {
                            Text(stringResource(R.string.cancel_btn), style = win95TextStyle)
                        }
                    }
                }
            }
        }
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
}
'''
content += change_name_def

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied change name")
