import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'userMap[myUserId] = UserItem(myUserId, myUserName, myNameColor)',
    'userMap[myUserId] = UserItem(myUserId, myUserName, myNameColor.toLong())'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed UserItem in rename")
