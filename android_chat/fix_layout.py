import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Make Win95ComboBox use fillMaxHeight() internally
combo_pattern = r'(Win95Button\(onClick = \{ expanded = !expanded \})(\) \{)'
content = re.sub(combo_pattern, r'\1, modifier = Modifier.fillMaxHeight()\2', content)

# Fix Toolbar row
toolbar_pattern = r'Row\(modifier = Modifier.fillMaxWidth\(\).horizontalScroll\(rememberScrollState\(\)\).padding\(vertical = 8.dp\).height\(androidx.compose.foundation.layout.IntrinsicSize.Min\), verticalAlignment = Alignment.CenterVertically\)'
toolbar_replacement = 'Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp).height(28.dp), verticalAlignment = Alignment.CenterVertically)'
content = content.replace(toolbar_pattern, toolbar_replacement)

# Fix Talk button to use fillMaxHeight()
talk_pattern = r'Win95Button\(\s*onClick = \{\s*isTalkOn = !isTalkOn\s*viewModel.setTalkOn\(isTalkOn\)\s*\}, \s*modifier = Modifier.padding\(end=8.dp\),\s*checked = isTalkOn\s*\) \{\s*Text\("話す", style = win95TextStyle, modifier = Modifier.padding\(horizontal = 8.dp, vertical = 2.dp\)\) \s*\}'

talk_replacement = '''Win95Button(
                    onClick = { 
                        isTalkOn = !isTalkOn 
                        viewModel.setTalkOn(isTalkOn)
                    }, 
                    modifier = Modifier.padding(end=8.dp).fillMaxHeight(),
                    checked = isTalkOn
                ) { 
                    Text("話す", style = win95TextStyle, modifier = Modifier.padding(horizontal = 8.dp)) 
                }'''

content = re.sub(talk_pattern, talk_replacement, content)

# Fix MicVolumeIndicator modifier
mic_pattern = r'MicVolumeIndicator\(volume = micVolume, modifier = Modifier.width\(100.dp\).height\(20.dp\).padding\(end=8.dp\)\)'
mic_replacement = 'MicVolumeIndicator(volume = micVolume, modifier = Modifier.width(100.dp).fillMaxHeight().padding(end=8.dp))'
content = content.replace(mic_pattern, mic_replacement)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated MainActivity.kt layout")
