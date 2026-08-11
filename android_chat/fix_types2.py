import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'userMap[myUserId] = UserItem(myUserId, myUserName, myNameColor.toLong())',
    'userMap[myUserId] = UserItem(myUserId, myUserName, myNameColor)'
)

# And fix the users/join updates!
content = content.replace(
    'userMap[uid] = UserItem(uid, newName, userMap[uid]?.nclr ?: -1)',
    'userMap[uid] = UserItem(uid, newName, userMap[uid]?.nclr ?: -1)'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed UserItem in ChatViewModel")

main2 = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main2, 'r', encoding='utf-8') as f:
    content2 = f.read()

content2 = content2.replace('nclr != -1LL', 'nclr != -1L')
content2 = content2.replace('user.nclr != -1LL', 'user.nclr != -1L')

# And user.nclr in MainActivity is Int, because _users is List of the ChatViewModel UserItem (with Int)!
# So we shouldn't use .toInt() or Long operations for user.nclr!
content2 = content2.replace('user.nclr != -1L', 'user.nclr != -1')
content2 = content2.replace('val r = (user.nclr and 0xFF).toInt()', 'val r = user.nclr and 0xFF')
content2 = content2.replace('val g = ((user.nclr shr 8) and 0xFF).toInt()', 'val g = (user.nclr shr 8) and 0xFF')
content2 = content2.replace('val b = ((user.nclr shr 16) and 0xFF).toInt()', 'val b = (user.nclr shr 16) and 0xFF')

with open(main2, 'w', encoding='utf-8') as f:
    f.write(content2)

print("Fixed MainActivity Long issues")
