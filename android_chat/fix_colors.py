import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Fix the palette to have 48 colors
old_palette = '''    val palette = listOf(
        Color(200, 50, 50), Color(50, 150, 50), Color(50, 50, 200),
        Color(180, 180, 50), Color(180, 50, 180), Color(50, 180, 180),
        Color(255, 128, 0), Color(128, 0, 255), Color(0, 128, 255),
        Color(128, 200, 0), Color(255, 0, 128), Color(0, 200, 128),
        Color.Black, Color.DarkGray, Color.Gray, Color.LightGray
    )'''

new_palette = '''    val palette = listOf(
        Color(255, 128, 128), Color(255, 255, 128), Color(128, 255, 128), Color(0, 255, 128), Color(128, 255, 255), Color(0, 128, 255), Color(255, 128, 192), Color(255, 128, 255),
        Color(255, 0, 0), Color(255, 255, 0), Color(128, 255, 0), Color(0, 255, 64), Color(0, 255, 255), Color(0, 128, 192), Color(128, 128, 192), Color(255, 0, 255),
        Color(128, 64, 64), Color(255, 128, 64), Color(0, 255, 0), Color(0, 128, 128), Color(0, 64, 128), Color(128, 128, 255), Color(128, 0, 64), Color(255, 0, 128),
        Color(128, 0, 0), Color(255, 128, 0), Color(0, 128, 0), Color(0, 128, 64), Color(0, 0, 255), Color(0, 0, 160), Color(128, 0, 128), Color(128, 0, 255),
        Color(64, 0, 0), Color(128, 64, 0), Color(0, 64, 0), Color(0, 64, 64), Color(0, 0, 128), Color(0, 0, 64), Color(64, 0, 64), Color(64, 0, 128),
        Color.Black, Color(64, 64, 64), Color.Gray, Color(160, 160, 160), Color.LightGray, Color(224, 224, 224), Color.White, Color(255, 255, 255)
    )'''

content = content.replace(old_palette, new_palette)

# 2. Fix the loop to 6 rows and 8 columns
content = content.replace('for (row in 0 until 4) {', 'for (row in 0 until 6) {')
content = content.replace('for (colIdx in 0 until 4) {', 'for (colIdx in 0 until 8) {')
content = content.replace('val col = palette[row * 4 + colIdx]', 'val col = palette[row * 8 + colIdx]')
content = content.replace('.size(32.dp).padding(2.dp).background(col).border(1.dp, Win95Black)', '.size(24.dp).padding(1.dp).background(col).border(1.dp, Win95Black)')

# 3. Fix the color packing in sendMessage
content = content.replace('val cInt = (r shl 16) or (g shl 8) or b', 'val cInt = (b shl 16) or (g shl 8) or r')

# 4. Fix the color packing in ChangeNameDialog
old_nclr = 'val nclr = if (selectedColor == Win95Black) -1 else selectedColor.toArgb() and 0xFFFFFF'
new_nclr = '''val nclr = if (selectedColor == Win95Black) -1 else {
                                val r = (selectedColor.red * 255).toInt()
                                val g = (selectedColor.green * 255).toInt()
                                val b = (selectedColor.blue * 255).toInt()
                                (b shl 16) or (g shl 8) or r
                            }'''
content = content.replace(old_nclr, new_nclr)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied color dialog fixes")
