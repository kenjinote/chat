import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_speak = '''                Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        viewModel.setTalkOn(isTalkOn)
                    }, 
                    modifier = Modifier.padding(end=8.dp),
                    checked = isTalkOn
                )'''

new_speak = '''                Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        viewModel.setTalkOn(isTalkOn)
                    }, 
                    modifier = Modifier.padding(end=8.dp),
                    checked = isTalkOn,
                    checkedBackgroundColor = SpeakActiveGreen
                )'''

if old_speak in content:
    content = content.replace(old_speak, new_speak)
    print("Replaced Speak button invocation.")
else:
    print("Could not find the old speak button invocation!")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

