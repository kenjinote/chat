import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Change Win95Button signature
old_sig = 'fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, checked: Boolean = false, content: @Composable () -> Unit) {'
new_sig = 'fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, checked: Boolean = false, checkedBackgroundColor: androidx.compose.ui.graphics.Color = Win95Gray, content: @Composable () -> Unit) {'
content = content.replace(old_sig, new_sig)

# Change background drawing
old_bg = '.background(if (checked) Win95White else Win95Gray)'
new_bg = '.background(if (checked) checkedBackgroundColor else Win95Gray)'
content = content.replace(old_bg, new_bg)

# Update Speak button to use Win95White for checked background
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
                    checked = isTalkOn
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
                    checkedBackgroundColor = Win95White
                )'''
content = content.replace(old_speak, new_speak)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated Win95Button to allow custom checked background")
