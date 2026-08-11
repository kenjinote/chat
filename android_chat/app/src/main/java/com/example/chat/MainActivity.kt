package com.example.chat

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
import androidx.compose.ui.Alignment
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.chat.network.ChatMessage
import com.example.chat.utils.TextProcessor
import androidx.compose.ui.platform.LocalUriHandler
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.compose.ui.viewinterop.AndroidView
import java.util.concurrent.Executors
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.unit.sp

// Win95 Colors
val Win95Gray = Color(0xFFC0C0C0)
val Win95DarkGray = Color(0xFF808080)
val Win95Black = Color(0xFF000000)
val Win95White = Color(0xFFFFFFFF)

val win95TextStyle = TextStyle(fontSize = 28.sp, color = Win95Black, fontFamily = FontFamily.SansSerif)

@Composable
fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .clickable(onClick = onClick)
            .background(Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                drawRect(Win95DarkGray, topLeft = Offset(bw, size.height - bw * 2), size = Size(size.width - bw * 2, bw))
                drawRect(Win95DarkGray, topLeft = Offset(size.width - bw * 2, bw), size = Size(bw, size.height - bw * 2))
            }
            .padding(horizontal = 12.dp, vertical = 8.dp),
        contentAlignment = Alignment.Center
    ) {
        content()
    }
}

@Composable
fun Win95TextField(value: String, onValueChange: (String) -> Unit, modifier: Modifier = Modifier, singleLine: Boolean = true) {
    BasicTextField(
        value = value,
        onValueChange = onValueChange,
        singleLine = singleLine,
        textStyle = win95TextStyle,
        modifier = modifier
            .background(Win95White)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95Black, topLeft = Offset(bw, bw), size = Size(size.width - bw * 2, bw))
                drawRect(Win95Black, topLeft = Offset(bw, bw), size = Size(bw, size.height - bw * 2))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                drawRect(Win95Gray, topLeft = Offset(bw, size.height - bw * 2), size = Size(size.width - bw * 2, bw))
                drawRect(Win95Gray, topLeft = Offset(size.width - bw * 2, bw), size = Size(bw, size.height - bw * 2))
            }
            .padding(8.dp)
    )
}

@Composable
fun Win95Panel(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .background(Win95White)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95Black, topLeft = Offset(bw, bw), size = Size(size.width - bw * 2, bw))
                drawRect(Win95Black, topLeft = Offset(bw, bw), size = Size(bw, size.height - bw * 2))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                drawRect(Win95Gray, topLeft = Offset(bw, size.height - bw * 2), size = Size(size.width - bw * 2, bw))
                drawRect(Win95Gray, topLeft = Offset(size.width - bw * 2, bw), size = Size(bw, size.height - bw * 2))
            }
    ) {
        content()
    }
}

@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        content = content
    )
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
    val connectionState by viewModel.connectionState.collectAsState()
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

    Column(modifier = Modifier.fillMaxSize().background(Win95Gray).padding(8.dp)) {
        // Connection Header
        if (connectionState == "Disconnected" || connectionState == "Failed") {
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Text("Server IP:", style = win95TextStyle)
                Spacer(modifier = Modifier.width(8.dp))
                Win95TextField(
                    value = ipAddress,
                    onValueChange = { ipAddress = it },
                    modifier = Modifier.weight(1f),
                    singleLine = true
                )
            }
            Spacer(modifier = Modifier.height(8.dp))
            Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                Checkbox(checked = isPrivate, onCheckedChange = { isPrivate = it })
                Text("Private", style = win95TextStyle)
                Spacer(modifier = Modifier.width(16.dp))
                Win95Button(onClick = { viewModel.connect(ipAddress, isPrivate) }) {
                    Text("Connect", style = win95TextStyle)
                }
            }
            Spacer(modifier = Modifier.weight(1f))
        } else {
            // No top header in connected state (matches Windows UI)


            // Main Area (Chat Log + User List)
            Row(modifier = Modifier.weight(1f).fillMaxWidth()) {
                // Left: Chat Log
                Column(modifier = Modifier.weight(7f).fillMaxHeight()) {
                    // Video View Area is currently hidden to remove the black rectangle
                    /*
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
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {
                        LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                            items(messages) { msg ->
                                Column(modifier = Modifier.padding(vertical = 4.dp)) {
                                    Text(text = msg.userName ?: "Unknown", fontWeight = FontWeight.Bold, color = if (msg.type == "system") Color.Red else Color.Unspecified, style = win95TextStyle)
                                    val contentToDisplay = msg.msg ?: msg.content ?: ""
                                    val annotatedStr = TextProcessor.formatMessage(contentToDisplay)
                                    ClickableText(
                                        text = annotatedStr,
                                        style = win95TextStyle.copy(color = if (msg.type == "system") Color.Red else Win95Black),
                                        onClick = { offset ->
                                            annotatedStr.getStringAnnotations(tag = "URL", start = offset, end = offset)
                                                .firstOrNull()?.let { annotation -> uriHandler.openUri(annotation.item) }
                                        }
                                    )
                                }
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.width(4.dp))

                // Right: User List
                Column(modifier = Modifier.weight(3f).fillMaxHeight()) {
                    Text("ユーザー一覧", fontWeight = FontWeight.Bold, style = win95TextStyle)
                    Spacer(modifier = Modifier.height(4.dp))
                    val users by viewModel.users.collectAsState()
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {
                        LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                            items(users) { user ->
                                Text(user, style = win95TextStyle, modifier = Modifier.padding(vertical = 4.dp))
                            }
                        }
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    Win95Button(onClick = { /* Move Room */ }, modifier = Modifier.fillMaxWidth()) {
                        Text("Move", style = win95TextStyle)
                    }
                }
            }

            Spacer(modifier = Modifier.height(4.dp))

            // Toolbar Area
            var isVideoOn by remember { mutableStateOf(true) }
            var isTalkOn by remember { mutableStateOf(false) }
            var volumeLevel by remember { mutableStateOf(50f) }

            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                // Formatting (Dummy for UI match)
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Sz", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Col", style = win95TextStyle) }
                
                Spacer(modifier = Modifier.width(16.dp))
                
                // Media Controls
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
                    modifier = Modifier.weight(1f)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Win95Button(
                    onClick = { 
                        if (inputText.isNotBlank()) {
                            viewModel.sendMessage(inputText)
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

@Preview(showBackground = true)
@Composable
fun ChatScreenPreview() {
    AndroidChatTheme {
        ChatScreen()
    }
}

