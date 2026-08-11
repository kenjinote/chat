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
import androidx.compose.foundation.border
import androidx.compose.ui.Alignment
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
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.gestures.detectDragGestures
import kotlin.math.roundToInt
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
    var ipAddress by remember { mutableStateOf("127.0.0.1") }
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

    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        // Connection Header
        if (connectionState == "Disconnected" || connectionState == "Failed") {
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
                TextField(
                    value = ipAddress,
                    onValueChange = { ipAddress = it },
                    label = { Text("Server IP") },
                    modifier = Modifier.weight(1f)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Row(verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
                    Checkbox(checked = isPrivate, onCheckedChange = { isPrivate = it })
                    Text("Private")
                }
                Spacer(modifier = Modifier.width(8.dp))
                Button(onClick = { viewModel.connect(ipAddress, isPrivate) }) {
                    Text("Connect")
                }
            }
        } else {
            // Connected State Header (Rename UI)
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
                Text("Connected", fontWeight = FontWeight.Bold)
                Spacer(modifier = Modifier.width(16.dp))
                TextField(
                    value = renameText,
                    onValueChange = { renameText = it },
                    label = { Text("New Name") },
                    modifier = Modifier.weight(1f)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Button(onClick = {
                    if (renameText.isNotBlank()) {
                        viewModel.rename(renameText)
                        renameText = ""
                    }
                }) {
                    Text("Rename")
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Video Views Area (only if connected)
        if (connectionState == "Connected") {
            Row(modifier = Modifier.fillMaxWidth().height(150.dp)) {
                // Remote Video Decoder View
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
                
                // Local Camera Setup and encoding trigger
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
                            // Init encoder once camera is ready
                            viewModel.getVideoEngine().initEncoder()
                        } catch(exc: Exception) {
                            exc.printStackTrace()
                        }
                    }, ContextCompat.getMainExecutor(context))
                }
            }
            Spacer(modifier = Modifier.height(8.dp))
        }

        // Message List
        LazyColumn(modifier = Modifier.weight(1f)) {
            items(messages) { msg ->
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    Column(modifier = Modifier.padding(8.dp)) {
                        Text(text = msg.userName ?: "Unknown", fontWeight = FontWeight.Bold, color = if (msg.type == "system") Color.Red else Color.Unspecified)
                        
                        val contentToDisplay = msg.msg ?: msg.content ?: ""
                        val annotatedStr = TextProcessor.formatMessage(contentToDisplay)
                        
                        ClickableText(
                            text = annotatedStr,
                            style = MaterialTheme.typography.bodyLarge.copy(color = if (msg.type == "system") Color.Red else MaterialTheme.colorScheme.onSurface),
                            onClick = { offset ->
                                annotatedStr.getStringAnnotations(tag = "URL", start = offset, end = offset)
                                    .firstOrNull()?.let { annotation ->
                                        uriHandler.openUri(annotation.item)
                                    }
                            }
                        )
                    }
                }
            }
        }

        // Input Area
        Row(modifier = Modifier.fillMaxWidth()) {
            TextField(
                value = inputText,
                onValueChange = { inputText = it },
                modifier = Modifier.weight(1f),
                placeholder = { Text("Type a message") },
                enabled = connectionState == "Connected"
            )
            Spacer(modifier = Modifier.width(8.dp))
            Button(
                onClick = { 
                    if (inputText.isNotBlank()) {
                        viewModel.sendMessage(inputText)
                        inputText = ""
                    }
                },
                enabled = connectionState == "Connected"
            ) {
                Text("Send")
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

