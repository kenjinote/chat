import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add height(IntrinsicSize.Min) to Toolbar Row
target_toolbar = 'Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {'
replacement_toolbar = 'Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp).height(androidx.compose.foundation.layout.IntrinsicSize.Min), verticalAlignment = Alignment.CenterVertically) {'
content = content.replace(target_toolbar, replacement_toolbar)

# 2. Modify B, I, U, Color buttons
# Bold
target_b = 'Win95Button(onClick = { isBold = !isBold }, checked = isBold, modifier = Modifier.padding(end=4.dp)) {'
rep_b = 'Win95Button(onClick = { isBold = !isBold }, checked = isBold, modifier = Modifier.padding(end=4.dp).fillMaxHeight().aspectRatio(1f)) {'
content = content.replace(target_b, rep_b)

# Italic
target_i = 'Win95Button(onClick = { isItalic = !isItalic }, checked = isItalic, modifier = Modifier.padding(end=4.dp)) {'
rep_i = 'Win95Button(onClick = { isItalic = !isItalic }, checked = isItalic, modifier = Modifier.padding(end=4.dp).fillMaxHeight().aspectRatio(1f)) {'
content = content.replace(target_i, rep_i)

# Underline
target_u = 'Win95Button(onClick = { isUnderline = !isUnderline }, checked = isUnderline, modifier = Modifier.padding(end=4.dp)) {'
rep_u = 'Win95Button(onClick = { isUnderline = !isUnderline }, checked = isUnderline, modifier = Modifier.padding(end=4.dp).fillMaxHeight().aspectRatio(1f)) {'
content = content.replace(target_u, rep_u)

# Color
target_c = 'Win95Button(onClick = { showColorPicker = true }, modifier = Modifier.padding(end=4.dp)) {'
rep_c = 'Win95Button(onClick = { showColorPicker = true }, modifier = Modifier.padding(end=4.dp).fillMaxHeight().aspectRatio(1f)) {'
content = content.replace(target_c, rep_c)

# 3. For the Talk button, make it fillMaxHeight to ensure it dictates the height (if ComboBox is taller) or stretches. Actually, Talk button and ComboBox will dictate height. 
target_talk = '''                  Win95Button(
                      onClick = { 
                          isTalkOn = !isTalkOn 
                          viewModel.setTalkOn(isTalkOn)
                      }, 
                      modifier = Modifier.padding(end=8.dp),'''
rep_talk = '''                  Win95Button(
                      onClick = { 
                          isTalkOn = !isTalkOn 
                          viewModel.setTalkOn(isTalkOn)
                      }, 
                      modifier = Modifier.padding(end=8.dp).fillMaxHeight(),'''
content = content.replace(target_talk, rep_talk)

# Also ComboBox might need fillMaxHeight? ComboBox is already whatever height it is. We can let it dictate the height.

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied toolbar layout tweaks")
