import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.withStylepackage com.example.chat

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.core.content.ContextCompat

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.ClickableText
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.border
import androidx.compose.ui.Alignment
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.unit.TextUnit
import androidx.compose.ui.unit.sp
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.withStyle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.chat.network.ChatMessage
import com.example.chat.network.UserItem
import com.example.chat.utils.TextProcessor
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.SpanStyle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.compose.ui.viewinterop.AndroidView
import java.util.concurrent.Executors
@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        content = content
    )
}

@Composable
fun Win95ComboBox(
    items: List<Int>,
    selectedItem: Int,
    onItemSelected: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    var expanded by remember { mutableStateOf(false) }

    Box(
        modifier = modifier
            .background(Win95White)
            .border(2.dp, Win95DarkGray)
            .clickable { expanded = true }
            .padding(horizontal = 4.dp, vertical = 2.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.width(40.dp)) {
            Text(selectedItem.toString(), style = win95TextStyle)
            Text("▼", style = win95TextStyle.copy(fontSize = 12.sp))
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
            modifier = Modifier
                .background(Win95Gray)
                .drawBehind {
                    val bw = 2.dp.toPx()
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                    drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                    drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                }
                .padding(2.dp)
        ) {
            items.forEach { item ->
                DropdownMenuItem(
                    text = { Text(item.toString(), style = win95TextStyle) },
                    onClick = {
                        onItemSelected(item)
                        expanded = false
                    }
                )
    }
}
@Composable
fun ColorPickerDialog(onColorSelected: (Color) -> Unit, onDismiss: () -> Unit) {
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
            // Title Bar
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Win95TitleBlue)
                    .padding(4.dp),
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
                                .size(32.dp)
                                .padding(2.dp)
                                .background(col)
                                .border(1.dp, Win95Black)
                                .clickable { onColorSelected(col) }
                            )
                        }
                    }
                }
            }
        }
    }
}

class MainActivity : ComponentActivity() {

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        // Handle permission responses
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        checkAndRequestPermissions()

        setContent {
            AndroidChatTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    ChatScreen()
                }
            }
        }
    }

    private fun checkAndRequestPermissions() {
        val requiredPermissions = arrayOf(
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO
        )

        val missingPermissions = requiredPermissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (missingPermissions.isNotEmpty()) {
            requestPermissionLauncher.launch(missingPermissions.toTypedArray())
        }
    }
}

@Composable
fun ChatScreen(viewModel: ChatViewModel = viewModel()) {
    val messages by viewModel.messages.collectAsState()

    var inputText by remember { mutableStateOf("") }
    var ipAddress by remember { mutableStateOf("chat-lb-2026.japaneast.cloudapp.azure.com") }
    var isPrivate by remember { mutableStateOf(false) }
    var renameText by remember { mutableStateOf("") }
    val uriHandler = LocalUriHandler.current
    
    val context = androidx.compose.ui.platform.LocalContext.current
    val lifecycleOwner = androidx.compose.ui.platform.LocalLifecycleOwner.current

    LaunchedEffect(Unit) {
        if (connectionState == "Disconnected") {
            viewModel.connect(ipAddress, isPrivate)
        }
    }

    Column(modifier = Modifier.fillMaxSize().padding(8.dp)) {
        // Connection Header
        if (connectionState == "Disconnected" || connectionState == "Failed") {
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                TextField(
                    value = ipAddress,
                    onValueChange = { ipAddress = it },
                    label = { Text("Server IP") },
                    modifier = Modifier.weight(1f)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = isPrivate, onCheckedChange = { isPrivate = it })
                    Text("Private")
                }
                Spacer(modifier = Modifier.width(8.dp))
                Button(onClick = { viewModel.connect(ipAddress, isPrivate) }) {
                    Text("Connect")
                }
            }
            Spacer(modifier = Modifier.weight(1f))
        } else {
            // No top header in connected state (matches Windows UI)


            // Main Area (Chat Log + User List)
            Row(modifier = Modifier.weight(1f).fillMaxWidth()) {
                // Left: Chat Log
                Column(modifier = Modifier.weight(7f).fillMaxHeight()) {
                    // Video View Area
                    Row(modifier = Modifier.fillMaxWidth().height(120.dp)) {
                        AndroidView(
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                            factory = { ctx ->
                                SurfaceView(ctx).apply {
                                    holder.addCallback(object : SurfaceHolder.Callback {
                                        override fun surfaceCreated(holder: SurfaceHolder) {
                                            viewModel.getVideoEngine().initDecoder(holder.surface)
                                        }
                                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}
                                        override fun surfaceDestroyed(holder: SurfaceHolder) {}
                                    })
                                }
                            }
                        )
                        Spacer(modifier = Modifier.width(8.dp))
                        LaunchedEffect(Unit) {
                            val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
                            cameraProviderFuture.addListener({
                                val cameraProvider = cameraProviderFuture.get()
                                val imageAnalysis = ImageAnalysis.Builder()
                                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                                    .build()
                                    .also {
                                        it.setAnalyzer(Executors.newSingleThreadExecutor()) { imageProxy ->
                                            viewModel.getVideoEngine().encodeFrame(imageProxy)
                                        }
                                    }
                                val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA
                                try {
                                    cameraProvider.unbindAll()
                                    cameraProvider.bindToLifecycle(lifecycleOwner, cameraSelector, imageAnalysis)
                                    viewModel.getVideoEngine().initEncoder()
                                } catch(exc: Exception) {
                                    exc.printStackTrace()
                                }
                            }, ContextCompat.getMainExecutor(context))
                        }
                    }
                    */
                    Spacer(modifier = Modifier.height(4.dp))
                    
                    // Message List
                    val chatListState = rememberLazyListState()
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth(), backgroundColor = Color.White) {
                        LazyColumn(state = chatListState, modifier = Modifier.fillMaxSize().padding(4.dp).win95Scrollbar(chatListState)) {
                            items(messages) { msg ->
                                val isSystem = msg.type == "system" || msg.type == "sysmsg"
                                val contentToDisplay = msg.msg ?: msg.content ?: ""
                                val messageAnnotated = TextProcessor.formatMessage(contentToDisplay)
                                
                                val finalStr = buildAnnotatedString {
                                    if (!isSystem) {
                                        val uName = msg.userName ?: "Unknown"
                                        val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                        withStyle(SpanStyle(fontWeight = FontWeight.Bold, color = Color.Unspecified)) {
                                            append("$displayUser: ")
                                        }
                                    }
                                    
                                    val msgColor = if (isSystem) Color.Gray else Win95Black
                                    
                                    val isBoldMsg = msg.b ?: false
                                    val isItalicMsg = msg.i ?: false
                                    val isUnderlineMsg = msg.u ?: false
                                    val fontSizeSp = (msg.sz ?: 10) * 2

                                    withStyle(SpanStyle(
                                        color = msgColor,
                                        fontWeight = if (isBoldMsg) FontWeight.Bold else FontWeight.Normal,
                                        fontStyle = if (isItalicMsg) FontStyle.Italic else FontStyle.Normal,
                                        textDecoration = if (isUnderlineMsg) TextDecoration.Underline else TextDecoration.None,
                                        fontSize = fontSizeSp.sp
                                    )) {
                                        append(messageAnnotated)
                                    }
                                }
                                
                                ClickableText(
                                    text = finalStr,
                                    style = win95TextStyle,
                                    onClick = { offset ->
                                        finalStr.getStringAnnotations(tag = "URL", start = offset, end = offset)
                                            .firstOrNull()?.let { annotation -> uriHandler.openUri(annotation.item) }
                                    },
                                    modifier = Modifier.padding(vertical = 4.dp).padding(end = 16.dp)
                                )
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.width(4.dp))

                // Right: User List
                Column(modifier = Modifier.weight(3f).fillMaxHeight()) {
                    val users by viewModel.users.collectAsState()
                    Text(stringResource(R.string.user_list_fmt, users.size), fontWeight = FontWeight.Bold, style = win95TextStyle)
                    Spacer(modifier = Modifier.height(4.dp))
                    val userListState = rememberLazyListState()
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {
                        LazyColumn(state = userListState, modifier = Modifier.fillMaxSize().padding(4.dp).win95Scrollbar(userListState)) {
                            items(users) { user ->
                                val displayName = if (user.contains("#")) user.substringBefore("#") else user
                                Text(displayName, style = win95TextStyle, modifier = Modifier.padding(vertical = 4.dp).padding(end = 16.dp))
                            }
                        }
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    Win95Button(onClick = { /* Move Room */ }, modifier = Modifier.fillMaxWidth()) {
                        Text(stringResource(R.string.move_room_btn), style = win95TextStyle)
                    }
                }
            }

            Spacer(modifier = Modifier.height(4.dp))

            // Toolbar Area
            var isVideoOn by remember { mutableStateOf(true) }
            val isTalkOn by viewModel.isTalkOn.collectAsState()
            var volumeLevel by remember { mutableStateOf(50f) }
            
            var isBold by remember { mutableStateOf(false) }
            var isItalic by remember { mutableStateOf(false) }
            var isUnderline by remember { mutableStateOf(false) }
            var fontSize by remember { mutableStateOf(10) }
            val fontSizes = listOf(6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32)
            var textColor by remember { mutableStateOf(Color.Black) }
            var showColorDialog by remember { mutableStateOf(false) }

            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                // Formatting
                Win95Button(onClick = { isBold = !isBold }, checked = isBold, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle.copy(fontWeight = FontWeight.Bold)) }
                Win95Button(onClick = { isItalic = !isItalic }, checked = isItalic, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle.copy(fontStyle = FontStyle.Italic)) }
                Win95Button(onClick = { isUnderline = !isUnderline }, checked = isUnderline, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle.copy(textDecoration = TextDecoration.Underline)) }
                
                Win95ComboBox(
                    items = fontSizes,
                    selectedItem = fontSize,
                    onItemSelected = { fontSize = it },
                    modifier = Modifier.padding(end=4.dp)
                )
                
                Win95Button(onClick = { showColorDialog = true }, modifier = Modifier.padding(end=4.dp)) {
                    Box(modifier = Modifier.size(16.dp).background(textColor).border(1.dp, Win95Black))
                }
                if (showColorDialog) {
                    ColorPickerDialog(
                        onColorSelected = { textColor = it; showColorDialog = false },
                        onDismiss = { showColorDialog = false }
                    )
                }
                
                Spacer(modifier = Modifier.width(16.dp))
                
                // Media Controls
                Win95Button(onClick = { viewModel.setTalkOn(!isTalkOn) }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isTalkOn) stringResource(R.string.talk) else stringResource(R.string.mute), style = win95TextStyle) 
                }
                Win95Button(onClick = { isVideoOn = !isVideoOn }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isVideoOn) stringResource(R.string.vid_on) else stringResource(R.string.vid_off), style = win95TextStyle) 
                }
                Text(stringResource(R.string.vol), style = win95TextStyle)
                Slider(value = volumeLevel, onValueChange = { volumeLevel = it }, valueRange = 0f..100f, modifier = Modifier.width(100.dp))
            }

            Spacer(modifier = Modifier.height(4.dp))

            // Input Area
            Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                val inputTextStyle = win95TextStyle.copy(
                    fontWeight = if (isBold) FontWeight.Bold else FontWeight.Normal,
                    fontStyle = if (isItalic) FontStyle.Italic else FontStyle.Normal,
                    textDecoration = if (isUnderline) TextDecoration.Underline else TextDecoration.None,
                    fontSize = fontSize.sp,
                    color = textColor
                )
                Win95TextField(
                    value = inputText,
                    onValueChange = { inputText = it },
                    modifier = Modifier.weight(1f).height(60.dp),
                    textStyle = inputTextStyle
                )
                Spacer(modifier = Modifier.width(8.dp))
                Win95Button(
                    onClick = { 
                        if (inputText.isNotBlank()) {
                            val r = (textColor.red * 255).toInt()
                            val g = (textColor.green * 255).toInt()
                            val b = (textColor.blue * 255).toInt()
                            val colorInt = r or (g shl 8) or (b shl 16)
                            viewModel.sendMessage(inputText, isBold, isItalic, isUnderline, fontSize, colorInt)
                            inputText = ""
                        }
                    }
                ) {
                    Text(stringResource(R.string.send), style = win95TextStyle)
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun ChatScreenPreview() {
    AndroidChatTheme {
        ChatScreen()
    }
}

