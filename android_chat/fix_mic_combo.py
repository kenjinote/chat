import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update Win95ComboBox modifier
content = content.replace(
    'Win95ComboBox(items = fontSizes, selectedItem = selectedSize, onItemSelected = { selectedSize = it }, modifier = Modifier.padding(end=4.dp))',
    'Win95ComboBox(items = fontSizes, selectedItem = selectedSize, onItemSelected = { selectedSize = it }, modifier = Modifier.padding(end=4.dp).fillMaxHeight())'
)

# 2. Update MicVolumeIndicator modifier
content = content.replace(
    'MicVolumeIndicator(volume = micVolume, modifier = Modifier.width(100.dp).height(24.dp))',
    'MicVolumeIndicator(volume = micVolume, modifier = Modifier.width(100.dp).fillMaxHeight())'
)

# 3. Replace MicVolumeIndicator logic
old_mic_vol = '''@Composable
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
}'''

new_mic_vol = '''@Composable
fun MicVolumeIndicator(volume: Int, modifier: Modifier = Modifier) {
    androidx.compose.foundation.Canvas(
        modifier = modifier
            .background(Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = androidx.compose.ui.geometry.Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = androidx.compose.ui.geometry.Offset(0f, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                drawRect(Win95White, topLeft = androidx.compose.ui.geometry.Offset(0f, size.height - bw), size = androidx.compose.ui.geometry.Size(size.width, bw))
                drawRect(Win95White, topLeft = androidx.compose.ui.geometry.Offset(size.width - bw, 0f), size = androidx.compose.ui.geometry.Size(bw, size.height))
                
                val innerRect = androidx.compose.ui.geometry.Rect(bw, bw, size.width - bw, size.height - bw)
                drawRect(Color.Black, topLeft = innerRect.topLeft, size = innerRect.size)
            }
    ) {
        val bw = 2.dp.toPx()
        val maxVolWidth = size.width - bw * 2
        val volWidth = (maxVolWidth * volume) / 100f
        
        if (volWidth > 0) {
            val brush = androidx.compose.ui.graphics.Brush.horizontalGradient(
                0.0f to Color(0, 220, 0),
                0.7f to Color(255, 220, 0),
                1.0f to Color(255, 0, 0),
                startX = bw,
                endX = size.width - bw
            )
            drawRect(
                brush = brush,
                topLeft = androidx.compose.ui.geometry.Offset(bw, bw),
                size = androidx.compose.ui.geometry.Size(volWidth, size.height - bw * 2)
            )
        }
    }
}'''

content = content.replace(old_mic_vol, new_mic_vol)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied MicVolume and ComboBox fixes")
