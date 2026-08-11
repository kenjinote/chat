import re

cvm_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\ChatViewModel.kt"
with open(cvm_path, "r", encoding="utf-8") as f:
    cvm = f.read()

# 1. Change val myUserId to var myUserId
cvm = cvm.replace('val myUserId = java.util.UUID.randomUUID().toString().replace("-", "")',
                  'var myUserId = java.util.UUID.randomUUID().toString().replace("-", "")')

# 2. Add ignoredUserIds and ignoreUser method
insert_ignored = """    var myUserName = "User_${System.currentTimeMillis() % 1000}"
    
    val ignoredUserIds = mutableSetOf<String>()
    
    fun ignoreUser(userId: String) {
        ignoredUserIds.add(userId)
        _users.value = sortUsers(userMap.values.toList())
    }
    
    private fun sortUsers(list: List<UserItem>): List<UserItem> {
        return list.sortedWith(compareBy({ ignoredUserIds.contains(it.userId) }, { it.userName.lowercase() }))
    }"""
cvm = cvm.replace('    var myUserName = "User_${System.currentTimeMillis() % 1000}"', insert_ignored)

# 3. Use sortUsers in places where _users.value is assigned
cvm = cvm.replace('_users.value = userMap.values.toList()', '_users.value = sortUsers(userMap.values.toList())')

# 4. Handle 'welcome' message to update myUserId and sort users
welcome_handler = """            "sysmsg", "system" -> {
                appendSystemMessage(message.msg ?: "System message")
            }
            "welcome" -> {
                message.userId?.let { 
                    myUserId = it
                    _users.value = sortUsers(userMap.values.toList())
                }
            }"""
cvm = cvm.replace("""            "sysmsg", "system" -> {
                appendSystemMessage(message.msg ?: "System message")
            }""", welcome_handler)

with open(cvm_path, "w", encoding="utf-8") as f:
    f.write(cvm)

ma_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(ma_path, "r", encoding="utf-8") as f:
    ma = f.read()

# 5. Bold the text if isMe and connect ignore action
old_item = """                                    Text(
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
                                    }"""

new_item = """                                    Text(
                                        text = displayUser,
                                        style = win95TextStyle.copy(color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
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
                                            onClick = { 
                                                menuExpandedUserId = null 
                                                viewModel.ignoreUser(user.userId)
                                            }
                                        )
                                        DropdownMenuItem(
                                            text = { Text(stringResource(R.string.report_user), style = win95TextStyle) },
                                            onClick = { menuExpandedUserId = null }
                                        )
                                    }"""
ma = ma.replace(old_item, new_item)

with open(ma_path, "w", encoding="utf-8") as f:
    f.write(ma)

print("Patch 5 applied!")
