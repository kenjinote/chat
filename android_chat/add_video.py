import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'

code = '''
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
                androidx.compose.foundation.gestures.detectDragGestures { change, dragAmount ->
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
                                    videoEngine.setRemoteSurface(h.surface)
                                }
                                override fun surfaceChanged(h: android.view.SurfaceHolder, format: Int, w: Int, h2: Int) {}
                                override fun surfaceDestroyed(h: android.view.SurfaceHolder) {
                                    videoEngine.setRemoteSurface(null)
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
'''

with open(main, 'a', encoding='utf-8') as f:
    f.write(code)

print("Added Win95VideoWindow")
