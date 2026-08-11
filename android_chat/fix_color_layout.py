import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

target = '''                  for (row in 0 until 6) {
                      Row {
                          for (colIdx in 0 until 8) {
                              val col = palette[row * 8 + colIdx]
                              Box(modifier = Modifier
                                  .size(24.dp).padding(1.dp).background(col).border(1.dp, Win95Black)
                                  .clickable { onColorSelected(col) }
                              )
                          }
                      }
                  }'''

replacement = '''                  for (row in 0 until 6) {
                      Row(modifier = Modifier.fillMaxWidth()) {
                          for (colIdx in 0 until 8) {
                              val col = palette[row * 8 + colIdx]
                              Box(modifier = Modifier
                                  .weight(1f).height(24.dp).padding(1.dp).background(col).border(1.dp, Win95Black)
                                  .clickable { onColorSelected(col) }
                              )
                          }
                      }
                  }'''

content = content.replace(target, replacement)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied weight to color boxes")
