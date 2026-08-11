import re

file_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# 1. State for selected user and menu
users_state_old = """                    val users by viewModel.users.collectAsState()
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {"""
users_state_new = """                    val users by viewModel.users.collectAsState()
                    var selectedUserId by remember { mutableStateOf<String?>(null) }
                    var menuExpandedUserId by remember { mutableStateOf<String?>(null) }
                    
                    Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {"""
content = content.replace(users_state_old, users_state_new)

# 2. Render user item with selection and menu
items_old = """                            items(users) { user ->
                                val uName = user.userName
                                val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val nameColor = if (user.nclr != null) {
                                    val b = (user.nclr shr 16) and 0xFF
                                    val g = (user.nclr shr 8) and 0xFF
                                    val r = user.nclr and 0xFF
                                    Color(r, g, b, 255)
                                } else {
                                    Win95Black
                                }
                                Text(displayUser, style = win95TextStyle.copy(color = nameColor), modifier = Modifier.padding(vertical = 4.dp))
                            }"""

items_new = """                            items(users) { user ->
                                val uName = user.userName
                                val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val isSelected = selectedUserId == user.userId
                                val isMenuExpanded = menuExpandedUserId == user.userId
                                
                                val baseColor = if (user.nclr != null) {
                                    val b = (user.nclr shr 16) and 0xFF
                                    val g = (user.nclr shr 8) and 0xFF
                                    val r = user.nclr and 0xFF
                                    Color(r, g, b, 255)
                                } else {
                                    Win95Black
                                }
                                
                                val textColor = if (isSelected) Color.White else baseColor
                                val bgColor = if (isSelected) Color(0xFF000080) else Color.Transparent

                                Box(modifier = Modifier.fillMaxWidth().background(bgColor).clickable {
                                    selectedUserId = user.userId
                                    menuExpandedUserId = user.userId
                                }) {
                                    Text(
                                        text = displayUser,
                                        style = win95TextStyle.copy(color = textColor),
                                        modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp).fillMaxWidth()
                                    )
                                    
                                    DropdownMenu(
                                        expanded = isMenuExpanded,
                                        onDismissRequest = { menuExpandedUserId = null },
                                        modifier = Modifier.background(Win95White).border(2.dp, Win95Black)
                                    ) {
                                        DropdownMenuItem(
                                            text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                            onClick = { menuExpandedUserId = null }
                                        )
                                        Divider(color = Win95Gray, thickness = 1.dp)
                                        DropdownMenuItem(
                                            text = { Text(stringResource(R.string.ignore_user), style = win95TextStyle) },
                                            onClick = { menuExpandedUserId = null }
                                        )
                                        DropdownMenuItem(
                                            text = { Text(stringResource(R.string.report_user), style = win95TextStyle) },
                                            onClick = { menuExpandedUserId = null }
                                        )
                                    }
                                }
                            }"""

content = content.replace(items_old, items_new)

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)

print("Patched users list selection and menu!")
