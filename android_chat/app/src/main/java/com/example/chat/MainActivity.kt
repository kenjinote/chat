package com.example.chat

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.ClickableText
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.chat.network.ChatMessage
import com.example.chat.utils.TextProcessor
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.res.stringResource

val Win95Gray = Color(192, 192, 192)
val Win95White = Color(255, 255, 255)
val Win95Black = Color(0, 0, 0)
val Win95DarkGray = Color(128, 128, 128)
val Win95TitleBlue = Color(0xFF000080)
val Win95Blue = Color(0xFF0000FF)

val win95TextStyle = TextStyle(
    fontFamily = FontFamily.Monospace,
    fontSize = 20.sp,
    color = Win95Black
)

@Composable
fun Win95Panel(modifier: Modifier = Modifier, backgroundColor: Color = Win95Gray, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .background(backgroundColor)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(2.dp)
    ) {
        content()
    }
}

@Composable
fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, checked: Boolean = false, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .clickable(onClick = onClick)
            .background(if (checked) Win95White else Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                val topColor = if (checked) Win95DarkGray else Win95White
                val bottomColor = if (checked) Win95White else Win95Black
                drawRect(topColor, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(topColor, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(bottomColor, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(bottomColor, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(horizontal = 8.dp, vertical = 4.dp),
        contentAlignment = Alignment.Center
    ) {
        content()
    }
}

@Composable
fun Win95TextField(value: String, onValueChange: (String) -> Unit, modifier: Modifier = Modifier, textStyle: TextStyle = win95TextStyle) {
    Box(
        modifier = modifier
            .height(60.dp)
            .background(Win95White)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(4.dp)
    ) {
        BasicTextField(
            value = value,
            onValueChange = onValueChange,
            textStyle = textStyle,
            modifier = Modifier.fillMaxSize(),
            cursorBrush = SolidColor(Win95Black)
        )
    }
}

@Composable
fun Win95ComboBox(items: List<String>, selectedItem: String, onItemSelected: (String) -> Unit, modifier: Modifier = Modifier) {
    var expanded by remember { mutableStateOf(false) }
    Box(modifier = modifier) {
        Win95Button(onClick = { expanded = !expanded }) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(selectedItem, style = win95TextStyle)
                Spacer(modifier = Modifier.width(4.dp))
                Text("▼", style = win95TextStyle.copy(fontSize = 12.sp))
            }
        }
        DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
            items.forEach { item ->
                DropdownMenuItem(
                    text = { Text(item, style = win95TextStyle) },
                    onClick = {
                        onItemSelected(item)
                        expanded = false
                    }
                )
            }
        }
    }
}

@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = lightColorScheme(
            background = Win95Gray,
            surface = Win95Gray
        ),
        content = content
    )
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            AndroidChatTheme {
                Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                    ChatScreen()
                }
            }
        }
    }
}
@Composable
fun ColorPickerDialog(onColorSelected: (Color) -> Unit, onDismiss: () -> Unit) {
    var customR by remember { mutableStateOf(0f) }
    var customG by remember { mutableStateOf(0f) }
    var customB by remember { mutableStateOf(0f) }

    val palette = listOf(
        Color(255, 128, 128), Color(255, 255, 128), Color(128, 255, 128), Color(0, 255, 128), Color(128, 255, 255), Color(0, 128, 255), Color(255, 128, 192), Color(255, 128, 255),
        Color(255, 0, 0), Color(255, 255, 0), Color(128, 255, 0), Color(0, 255, 64), Color(0, 255, 255), Color(0, 128, 192), Color(128, 128, 192), Color(255, 0, 255),
        Color(128, 64, 64), Color(255, 128, 64), Color(0, 255, 0), Color(0, 128, 128), Color(0, 64, 128), Color(128, 128, 255), Color(128, 0, 64), Color(255, 0, 128),
        Color(128, 0, 0), Color(255, 128, 0), Color(0, 128, 0), Color(0, 128, 64), Color(0, 0, 255), Color(0, 0, 160), Color(128, 0, 128), Color(128, 0, 255),
        Color(64, 0, 0), Color(128, 64, 0), Color(0, 64, 0), Color(0, 64, 64), Color(0, 0, 128), Color(0, 0, 64), Color(64, 0, 64), Color(64, 0, 128),
        Color.Black, Color(64, 64, 64), Color.Gray, Color(160, 160, 160), Color.LightGray, Color(224, 224, 224), Color.White, Color(255, 255, 255)
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
                for (row in 0 until 6) {
                    Row {
                        for (colIdx in 0 until 8) {
                            val col = palette[row * 8 + colIdx]
                            Box(modifier = Modifier
                                .size(24.dp).padding(1.dp).background(col).border(1.dp, Win95Black)
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
                    LazyColumn(modifier = Modifier.fillMaxSize().padding(0.5.dp)) {
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
@Composable
fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    LaunchedEffect(Unit) {
        viewModel.connect("chat-lb-2026.japaneast.cloudapp.azure.com")
    }
    val connectionState by viewModel.connectionState.collectAsState()
    val messages by viewModel.messages.collectAsState()
    val watchingUserId by viewModel.watchingUserId.collectAsState()
    val uriHandler = LocalUriHandler.current

    var inputText by remember { mutableStateOf("") }
    var isVideoOn by remember { mutableStateOf(false) }
    var isTalkOn by remember { mutableStateOf(false) }
    var showMoveRoomDialog by remember { mutableStateOf(false) }
    var showChangeNameDialog by remember { mutableStateOf(false) }
    
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

    Box(modifier = Modifier.fillMaxSize().background(Win95Gray)) {
        if (watchingUserId != null) {
            Win95VideoWindow(
                title = "Video - ",
                onClose = { viewModel.setWatchingUserId(null) },
                videoEngine = viewModel.getVideoEngine()
            )
        }
        Column(modifier = Modifier.fillMaxSize()) {
            // Main Chat Area
            Row(modifier = Modifier.weight(1f).fillMaxWidth()) {
                // Left: Chat Messages
                Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight(), backgroundColor = Color.White) {
                    LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {
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
                    
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth(), backgroundColor = Color.White) {
                        LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {
                            items(users.sortedBy { it.userName }) { user ->
                                val uName = user.userName
                                val baseDisplayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val isMe = user.userId == viewModel.myUserId
                                val displayUser = if (isMe) baseDisplayUser + stringResource(R.string.you_suffix) else baseDisplayUser
                                val videoStreamers by viewModel.videoStreamers.collectAsState()
                                val isStreaming = videoStreamers.containsKey(user.userId)
                                val prefix = "😊" + (if(isStreaming) "📺" else "")
                                val finalDisplayUser = prefix + displayUser
                                val isMenuExpanded = menuExpandedUserId == user.userId
                                
                                val baseColor = if (user.nclr != -1) {
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
                                        text = finalDisplayUser,
                                        style = win95TextStyle.copy(fontSize = 16.sp, color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
                                        modifier = Modifier.padding(horizontal = 0.5.dp, vertical = 0.5.dp).fillMaxWidth()
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
                                            Divider(color = Win95DarkGray, thickness = 1.dp)
                                            Divider(color = Win95White, thickness = 1.dp)
                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.rename_menu), style = win95TextStyle) },
                                                onClick = {
                                                    menuExpandedUserId = null
                                                    showChangeNameDialog = true
                                                }
                                            )
                                        } else {
                                            val isIgnored = ignoredUsers.contains(user.userId)
                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                                onClick = { 
                                                    viewModel.setWatchingUserId(user.userId)
                                                    menuExpandedUserId = null 
                                                }
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
                
                val micVolume by viewModel.micVolume.collectAsState()
                
                Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        viewModel.setTalkOn(isTalkOn)
                    }, 
                    modifier = Modifier.padding(end=8.dp),
                    checked = isTalkOn
                ) { 
                    Text("話す", style = win95TextStyle, modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)) 
                }
                
                MicVolumeIndicator(volume = micVolume, modifier = Modifier.width(100.dp).height(20.dp).padding(end=8.dp))
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
                            val cInt = (b shl 16) or (g shl 8) or r
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


@Composable
fun Win95VideoWindow(title: String, onClose: () -> Unit, videoEngine: com.example.chat.media.VideoEngine) {
    var offsetX by remember { mutableStateOf(50f) }
    var offsetY by remember { mutableStateOf(50f) }
    
    Box(
        modifier = Modifier
            .offset { androidx.compose.ui.unit.IntOffset(offsetX.toInt(), offsetY.toInt()) }
            .size(200.dp, 180.dp)
            .background(Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95White, topLeft = androidx.compose.ui.geometry.Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(size.width, bw))
                drawRect(Win95White, topLeft = androidx.compose.ui.geometry.Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                drawRect(Win95Black, topLeft = androidx.compose.ui.geometry.Offset(0f, size.height - bw), size = androidx.compose.ui.geometry.Size(size.width, bw))
                drawRect(Win95Black, topLeft = androidx.compose.ui.geometry.Offset(size.width - bw, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
            }
            .padding(2.dp)
            .pointerInput(Unit) {
                detectDragGestures { change, dragAmount ->
                    change.consume()
                    offsetX += dragAmount.x
                    offsetY += dragAmount.y
                }
            }
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            // Title bar
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Win95TitleBlue)
                    .padding(2.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = title,
                    style = win95TextStyle.copy(color = Color.White, fontWeight = FontWeight.Bold),
                    modifier = Modifier.weight(1f).padding(start = 2.dp)
                )
                Box(
                    modifier = Modifier
                        .size(16.dp)
                        .background(Win95Gray)
                        .clickable { onClose() }
                ) {
                    Text("X", style = win95TextStyle.copy(fontSize = 10.sp), modifier = Modifier.align(Alignment.Center))
                }
            }
            
            // Video surface
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(4.dp)
                    .background(Color.Black)
            ) {
                androidx.compose.ui.viewinterop.AndroidView(
                    factory = { ctx ->
                        android.view.SurfaceView(ctx).apply {
                            holder.addCallback(object : android.view.SurfaceHolder.Callback {
                                override fun surfaceCreated(h: android.view.SurfaceHolder) {
                                    videoEngine.initDecoder(h.surface)
                                }
                                override fun surfaceChanged(h: android.view.SurfaceHolder, format: Int, w: Int, h2: Int) {}
                                override fun surfaceDestroyed(h: android.view.SurfaceHolder) {
                                    videoEngine.stopDecoder()
                                }
                            })
                        }
                    },
                    modifier = Modifier.fillMaxSize()
                )
            }
        }
    }
}

@Composable
fun MicVolumeIndicator(volume: Int, modifier: Modifier = Modifier) {
    androidx.compose.foundation.Canvas(modifier = modifier.background(Color.Black)) {
        val bars = 20
        val activeBars = (volume * bars) / 100
        val barWidth = size.width / bars
        for (i in 0 until bars) {
            val color = if (i >= activeBars) {
                Color(0, 50, 0)
            } else if (i < 12) {
                Color(0, 255, 0)
            } else if (i < 16) {
                Color(255, 255, 0)
            } else {
                Color(255, 0, 0)
            }
            
            drawRect(
                color = color,
                topLeft = androidx.compose.ui.geometry.Offset(i * barWidth + 1.dp.toPx(), 2.dp.toPx()),
                size = androidx.compose.ui.geometry.Size(barWidth - 1.dp.toPx(), size.height - 4.dp.toPx())
            )
        }
    }
}

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
                            val nclr = if (selectedColor == Win95Black) -1 else {
                                val r = (selectedColor.red * 255).toInt()
                                val g = (selectedColor.green * 255).toInt()
                                val b = (selectedColor.blue * 255).toInt()
                                (b shl 16) or (g shl 8) or r
                            }
                            onConfirm(newName, nclr) 
                        }, modifier = Modifier.width(80.dp)) {
                            Text("OK", style = win95TextStyle)
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                        Win95Button(onClick = onDismiss, modifier = Modifier.width(80.dp)) {
                            Text("Cancel", style = win95TextStyle)
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
