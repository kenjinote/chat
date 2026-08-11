import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

if 'val SpeakActiveGreen = Color(100, 255, 100)' not in content:
    content = content.replace('val Win95Black = Color(0, 0, 0)', 'val Win95Black = Color(0, 0, 0)\nval SpeakActiveGreen = Color(100, 255, 100)')

old_speak = '''                Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        if (isTalkOn) {
                            viewModel.startAudioStream()
                        } else {
                            viewModel.stopAudioStream()
                        }
                    },
                    modifier = Modifier.padding(end=8.dp),
                    checked = isTalkOn,
                    checkedBackgroundColor = Win95Gray
                )'''
new_speak = '''                Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        if (isTalkOn) {
                            viewModel.startAudioStream()
                        } else {
                            viewModel.stopAudioStream()
                        }
                    },
                    modifier = Modifier.padding(end=8.dp),
                    checked = isTalkOn,
                    checkedBackgroundColor = SpeakActiveGreen
                )'''
content = content.replace(old_speak, new_speak)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated Speak button active background color to RGB(100, 255, 100)")
