import re

file_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Imports
imports = """
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.TextUnit
"""
content = content.replace("import androidx.compose.ui.Alignment\n", "import androidx.compose.ui.Alignment\n" + imports)

# 2. Win95Button
btn_old = """fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .clickable(onClick = onClick)
            .background(Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95Black, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95Black, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                drawRect(Win95DarkGray, topLeft = Offset(bw, size.height - bw * 2), size = Size(size.width - bw * 2, bw))
                drawRect(Win95DarkGray, topLeft = Offset(size.width - bw * 2, bw), size = Size(bw, size.height - bw * 2))
            }"""
btn_new = """fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, checked: Boolean = false, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .clickable(onClick = onClick)
            .background(if (checked) Win95White else Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                val tl1 = if (checked) Win95Black else Win95White
                val br1 = if (checked) Win95White else Win95Black
                val br2 = if (checked) Win95White else Win95DarkGray
                drawRect(tl1, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(tl1, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(br1, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(br1, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
                if (!checked) {
                    drawRect(br2, topLeft = Offset(bw, size.height - bw * 2), size = Size(size.width - bw * 2, bw))
                    drawRect(br2, topLeft = Offset(size.width - bw * 2, bw), size = Size(bw, size.height - bw * 2))
                }
            }"""
content = content.replace(btn_old, btn_new)

# 3. Localizations
replacements = {
    'Text("Server IP:", style = win95TextStyle)': 'Text(stringResource(R.string.server_ip), style = win95TextStyle)',
    'Text("Private", style = win95TextStyle)': 'Text(stringResource(R.string.private_room), style = win95TextStyle)',
    'Text("Connect", style = win95TextStyle)': 'Text(stringResource(R.string.connect), style = win95TextStyle)',
    'Text("Name:", style = win95TextStyle)': 'Text(stringResource(R.string.name_label), style = win95TextStyle)',
    'Text("Rename", style = win95TextStyle)': 'Text(stringResource(R.string.rename_btn), style = win95TextStyle)',
    'Text("Users (${users.size})", fontWeight = FontWeight.Bold, style = win95TextStyle)': 'Text(stringResource(R.string.user_list_fmt, users.size), fontWeight = FontWeight.Bold, style = win95TextStyle)',
    'Text("Move", style = win95TextStyle)': 'Text(stringResource(R.string.move_room_btn), style = win95TextStyle)',
    'Text(if(isTalkOn) "Talk" else "Mute", style = win95TextStyle)': 'Text(if(isTalkOn) stringResource(R.string.talk) else stringResource(R.string.mute), style = win95TextStyle)',
    'Text(if(isVideoOn) "Vid:ON" else "Vid:OFF", style = win95TextStyle)': 'Text(if(isVideoOn) stringResource(R.string.vid_on) else stringResource(R.string.vid_off), style = win95TextStyle)',
    'Text("Vol:", style = win95TextStyle)': 'Text(stringResource(R.string.vol), style = win95TextStyle)',
    'Text("Send", style = win95TextStyle)': 'Text(stringResource(R.string.send), style = win95TextStyle)'
}
for k, v in replacements.items():
    content = content.replace(k, v)

# 4. Remove System Text Label
sys_label_old = """                                    if (isSystem) {
                                        Text(
                                            text = "system",
                                            style = win95TextStyle.copy(color = Color.Blue),
                                            modifier = Modifier.padding(end = 8.dp)
                                        )
                                    }"""
content = content.replace(sys_label_old, "")

# 5. Toolbar state and Combobox
toolbar_old = """            var isVideoOn by remember { mutableStateOf(true) }
            val isTalkOn by viewModel.isTalkOn.collectAsState()
            var volumeLevel by remember { mutableStateOf(50f) }

            Row(modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                // Formatting (Dummy for UI match)
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("B", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("I", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("U", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Sz", style = win95TextStyle) }
                Win95Button(onClick = {}, modifier = Modifier.padding(end=4.dp)) { Text("Col", style = win95TextStyle) }"""
toolbar_new = """            var isVideoOn by remember { mutableStateOf(true) }
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

# 6. Apply Styles to Message and Send
send_old = """                            viewModel.sendMessage(inputText)"""
send_new = """                            viewModel.sendMessage(inputText, isBold, isItalic, isUnderline, fontSize)"""
content = content.replace(send_old, send_new)

style_old = """                                    val msgColor = if (isSystem) Color.Gray else Win95Black
                                    withStyle(SpanStyle(color = msgColor)) {
                                        append(messageAnnotated)
                                    }"""
style_new = """                                    val msgColor = if (isSystem) Color.Gray else Win95Black
                                    val isBoldMsg = msg.b ?: false
                                    val isItalicMsg = msg.i ?: false
                                    val isUnderlineMsg = msg.u ?: false
                                    val fontSizeSp = (msg.sz ?: 10) * 2

                                    withStyle(SpanStyle(
                                        color = msgColor,
                                        fontWeight = if (isBoldMsg) FontWeight.Bold else FontWeight.Normal,
                                        fontStyle = if (isItalicMsg) FontStyle.Italic else FontStyle.Normal,
                                        textDecoration = if (isUnderlineMsg) TextDecoration.Underline else TextDecoration.None,
                                        fontSize = fontSizeSp.sp
                                    )) {
                                        append(messageAnnotated)
                                    }"""
content = content.replace(style_old, style_new)

# Append ComboBox
combo_box = """

@Composable
fun Win95ComboBox(
    items: List<Int>,
    selectedItem: Int,
    onItemSelected: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    var expanded by remember { mutableStateOf(false) }

    Box(
        modifier = modifier
            .background(Win95White)
            .border(2.dp, Win95DarkGray)
            .clickable { expanded = true }
            .padding(horizontal = 4.dp, vertical = 2.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.width(40.dp)) {
            Text(selectedItem.toString(), style = win95TextStyle)
            Text("▼", style = win95TextStyle.copy(fontSize = 12.sp))
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
            modifier = Modifier.background(Win95White).border(2.dp, Win95Black)
        ) {
            items.forEach { item ->
                DropdownMenuItem(
                    text = { Text(item.toString(), style = win95TextStyle) },
                    onClick = {
                        onItemSelected(item)
                        expanded = false
                    }
                )
            }
        }
    }
}
"""
content += combo_box

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)

print("Patched!")
