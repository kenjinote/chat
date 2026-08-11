import re

file_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# 5. Toolbar state and Combobox
toolbar_old = """            // Toolbar Area
            var isVideoOn by remember { mutableStateOf(true) }
            var isTalkOn by remember { mutableStateOf(false) }
            var volumeLevel by remember { mutableStateOf(50f) }

            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                // Formatting (Dummy for UI match)
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Sz", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Col", style = win95TextStyle) }"""

toolbar_new = """            // Toolbar Area
            var isVideoOn by remember { mutableStateOf(true) }
            val isTalkOn by viewModel.isTalkOn.collectAsState()
            var volumeLevel by remember { mutableStateOf(50f) }
            
            var isBold by remember { mutableStateOf(false) }
            var isItalic by remember { mutableStateOf(false) }
            var isUnderline by remember { mutableStateOf(false) }
            var fontSize by remember { mutableStateOf(10) }
            val fontSizes = listOf(8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72)

            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                // Formatting
                Win95Button(onClick = { isBold = !isBold }, checked = isBold, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle.copy(fontWeight = FontWeight.Bold)) }
                Win95Button(onClick = { isItalic = !isItalic }, checked = isItalic, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle.copy(fontStyle = FontStyle.Italic)) }
                Win95Button(onClick = { isUnderline = !isUnderline }, checked = isUnderline, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle.copy(textDecoration = TextDecoration.Underline)) }
                
                Win95ComboBox(
                    items = fontSizes,
                    selectedItem = fontSize,
                    onItemSelected = { fontSize = it },
                    modifier = Modifier.padding(end=4.dp)
                )
                
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Col", style = win95TextStyle) }"""
content = content.replace(toolbar_old, toolbar_new)

# Also fix the onClick for talk button that was reverted!
talk_old = """                Win95Button(onClick = { isTalkOn = !isTalkOn }, modifier = Modifier.padding(end=4.dp)) {"""
talk_new = """                Win95Button(onClick = { viewModel.setTalkOn(!isTalkOn) }, modifier = Modifier.padding(end=4.dp)) {"""
content = content.replace(talk_old, talk_new)


with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)

print("Patched Toolbar!")
