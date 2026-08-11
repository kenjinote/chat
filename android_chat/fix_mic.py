import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

target = '''                Win95Button(onClick = { isTalkOn = !isTalkOn }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isTalkOn) "Talk" else "Mute", style = win95TextStyle) 
                }
                Win95Button(onClick = { isVideoOn = !isVideoOn }, modifier = Modifier.padding(end=4.dp)) { 
                    Text(if(isVideoOn) "Vid:ON" else "Vid:OFF", style = win95TextStyle) 
                }
                Text("Vol:", style = win95TextStyle)
                Slider(value = volumeLevel, onValueChange = { volumeLevel = it }, valueRange = 0f..100f, modifier = Modifier.width(100.dp))'''

replacement = '''                val micVolume by viewModel.micVolume.collectAsState()
                
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
                
                MicVolumeIndicator(volume = micVolume, modifier = Modifier.width(100.dp).height(20.dp).padding(end=8.dp))'''

content = content.replace(target, replacement)

mic_indicator = '''
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
'''
content += mic_indicator

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied Mic volume changes")
