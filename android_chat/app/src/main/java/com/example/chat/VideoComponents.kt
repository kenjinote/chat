package com.example.chat

import android.Manifest
import android.content.pm.PackageManager
import android.util.Size
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.compose.ui.window.Dialog
import androidx.core.content.ContextCompat
import com.example.chat.media.VideoEngine
import java.util.concurrent.Executors

@Composable
fun VideoShareDialog(
    videoEngine: VideoEngine,
    onDismiss: () -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    var hasPermission by remember { 
        mutableStateOf(ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) 
    }
    
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        hasPermission = isGranted
        if (!isGranted) {
            onDismiss()
        }
    }

    LaunchedEffect(Unit) {
        if (!hasPermission) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    if (hasPermission) {
        Dialog(onDismissRequest = onDismiss) {
            Column(
                modifier = Modifier
                    .width(340.dp)
                    .background(Win95Gray)
                    .drawBehind {
                        val bw = 2.dp.toPx()
                        drawRect(Win95White, topLeft = Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(size.width, bw))
                        drawRect(Win95White, topLeft = Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                        drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = androidx.compose.ui.geometry.Size(size.width, bw))
                        drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                    }
                    .padding(2.dp)
            ) {
                // Title bar
                Row(
                    modifier = Modifier.fillMaxWidth().background(Win95TitleBlue).padding(horizontal = 4.dp, vertical = 2.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(text = "Video Share", color = Color.White, fontWeight = FontWeight.Bold, style = win95TextStyle)
                    Win95Button(onClick = onDismiss, modifier = Modifier.size(24.dp)) {
                        Text("X", style = win95TextStyle.copy(fontWeight = FontWeight.Bold))
                    }
                }
                
                Spacer(modifier = Modifier.height(4.dp))
                
                // Video Preview Area
                Box(modifier = Modifier.fillMaxWidth().height(260.dp).padding(4.dp).border(1.dp, Win95DarkGray)) {
                    AndroidView(
                        factory = { ctx ->
                            val previewView = PreviewView(ctx)
                            val cameraProviderFuture = ProcessCameraProvider.getInstance(ctx)
                            val executor = Executors.newSingleThreadExecutor()
                            
                            cameraProviderFuture.addListener({
                                val cameraProvider = cameraProviderFuture.get()
                                val preview = Preview.Builder().build().also {
                                    it.setSurfaceProvider(previewView.surfaceProvider)
                                }
                                
                                val imageAnalysis = ImageAnalysis.Builder()
                                    .setTargetResolution(Size(640, 480))
                                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                                    .build()
                                    
                                videoEngine.initEncoder(640, 480, 30)
                                imageAnalysis.setAnalyzer(executor) { imageProxy ->
                                    videoEngine.encodeFrame(imageProxy)
                                }
                                
                                val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA
                                
                                try {
                                    cameraProvider.unbindAll()
                                    cameraProvider.bindToLifecycle(
                                        lifecycleOwner,
                                        cameraSelector,
                                        preview,
                                        imageAnalysis
                                    )
                                } catch (e: Exception) {
                                    e.printStackTrace()
                                }
                            }, ContextCompat.getMainExecutor(ctx))
                            previewView
                        },
                        modifier = Modifier.fillMaxSize()
                    )
                }
            }
        }
    }
}

@Composable
fun VideoWatchDialog(
    videoEngine: VideoEngine,
    userId: String,
    userName: String,
    onDismiss: () -> Unit
) {
    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .width(340.dp)
                .background(Win95Gray)
                .drawBehind {
                    val bw = 2.dp.toPx()
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(size.width, bw))
                    drawRect(Win95White, topLeft = Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                    drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = androidx.compose.ui.geometry.Size(size.width, bw))
                    drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                }
                .padding(2.dp)
        ) {
            // Title bar
            Row(
                modifier = Modifier.fillMaxWidth().background(Win95TitleBlue).padding(horizontal = 4.dp, vertical = 2.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                val displayUser = if (userName.contains("#")) userName.substringBefore("#") else userName
                Text(text = "Watch: $displayUser", color = Color.White, fontWeight = FontWeight.Bold, style = win95TextStyle)
                Win95Button(onClick = onDismiss, modifier = Modifier.size(24.dp)) {
                    Text("X", style = win95TextStyle.copy(fontWeight = FontWeight.Bold))
                }
            }
            
            Spacer(modifier = Modifier.height(4.dp))
            
            // Video Playback Area
            Box(modifier = Modifier.fillMaxWidth().height(260.dp).padding(4.dp).border(1.dp, Win95DarkGray)) {
                AndroidView(
                    factory = { ctx ->
                        val surfaceView = android.view.SurfaceView(ctx)
                        surfaceView.holder.addCallback(object : android.view.SurfaceHolder.Callback {
                            override fun surfaceCreated(holder: android.view.SurfaceHolder) {
                                videoEngine.initDecoder(holder.surface, 640, 480)
                            }
                            override fun surfaceChanged(holder: android.view.SurfaceHolder, format: Int, width: Int, height: Int) {}
                            override fun surfaceDestroyed(holder: android.view.SurfaceHolder) {}
                        })
                        surfaceView
                    },
                    modifier = Modifier.fillMaxSize()
                )
            }
        }
    }
}
