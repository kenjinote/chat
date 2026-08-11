import re

main = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    'val serverRoomName = if (room == "ロビー") "" else room',
    'val serverRoomName = serverRoomOverride ?: (if (room == "ロビー") "" else room)'
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed ChatViewModel serverRoomName")

main2 = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main2, 'r', encoding='utf-8') as f:
    content2 = f.read()

content2 = content2.replace('nclr != -1', 'nclr != -1L')
content2 = content2.replace('user.nclr != -1', 'user.nclr != -1L')
content2 = content2.replace('val r = nclr and 0xFF', 'val r = (nclr and 0xFF).toInt()')
content2 = content2.replace('val g = (nclr shr 8) and 0xFF', 'val g = ((nclr shr 8) and 0xFF).toInt()')
content2 = content2.replace('val b = (nclr shr 16) and 0xFF', 'val b = ((nclr shr 16) and 0xFF).toInt()')
content2 = content2.replace('val r = user.nclr and 0xFF', 'val r = (user.nclr and 0xFF).toInt()')
content2 = content2.replace('val g = (user.nclr shr 8) and 0xFF', 'val g = ((user.nclr shr 8) and 0xFF).toInt()')
content2 = content2.replace('val b = (user.nclr shr 16) and 0xFF', 'val b = ((user.nclr shr 16) and 0xFF).toInt()')

with open(main2, 'w', encoding='utf-8') as f:
    f.write(content2)

print("Fixed MainActivity Long types")
