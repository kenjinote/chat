import re

file_path_cvm = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\ChatViewModel.kt"
with open(file_path_cvm, "r", encoding="utf-8") as f:
    cvm_content = f.read()

cvm_old = """appendSystemMessage(getApplication<Application>().getString(R.string.joined_network, myUserName))"""
cvm_new = """appendSystemMessage(getApplication<Application>().getString(R.string.joined_network) + " (" + myUserName.substringBefore("#") + ")")"""
cvm_content = cvm_content.replace(cvm_old, cvm_new)

with open(file_path_cvm, "w", encoding="utf-8") as f:
    f.write(cvm_content)

file_path_ma = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(file_path_ma, "r", encoding="utf-8") as f:
    ma_content = f.read()

ma_old = """                            items(users) { user ->
                                val uName = user.userName
                                val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val isSelected = selectedUserId == user.userId"""
ma_new = """                            items(users) { user ->
                                val uName = user.userName
                                val baseDisplayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                val isMe = user.userId == viewModel.myUserId
                                val displayUser = if (isMe) baseDisplayUser + stringResource(R.string.you_suffix) else baseDisplayUser
                                val isSelected = selectedUserId == user.userId"""
ma_content = ma_content.replace(ma_old, ma_new)

with open(file_path_ma, "w", encoding="utf-8") as f:
    f.write(ma_content)

print("Patched!")
