import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Remove "Col" text
target_col = '''                Win95Button(onClick = { showColorPicker = true }, modifier = Modifier.padding(end=4.dp)) { 
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Col", style = win95TextStyle)
                        Spacer(modifier = Modifier.width(4.dp))
                        Box(modifier = Modifier.size(16.dp).background(selectedColor).border(1.dp, Win95Black))
                    }
                }'''
replacement_col = '''                Win95Button(onClick = { showColorPicker = true }, modifier = Modifier.padding(end=4.dp)) { 
                    Box(modifier = Modifier.size(16.dp).background(selectedColor).border(1.dp, Win95Black))
                }'''
content = content.replace(target_col, replacement_col)

# 2 & 3. Update Input Area Row and Send Button
target_input = '''              // Input Area
              Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                  Win95TextField(
                      value = inputText,
                      onValueChange = { inputText = it },
                      modifier = Modifier.weight(1f),
                      textStyle = win95TextStyle.copy(
                          color = selectedColor,
                          fontWeight = if (isBold) FontWeight.Bold else FontWeight.Normal,
                          fontStyle = if (isItalic) FontStyle.Italic else FontStyle.Normal,
                          fontSize = (selectedSize.toIntOrNull() ?: 10).sp * 2
                      )
                  )
                  Spacer(modifier = Modifier.width(8.dp))
                  Win95Button(
                      onClick = { 
                          if (inputText.isNotBlank()) {
                              val r = (selectedColor.red * 255).toInt()
                              val g = (selectedColor.green * 255).toInt()
                              val b = (selectedColor.blue * 255).toInt()
                              val cInt = (b shl 16) or (g shl 8) or r
                              viewModel.sendMessage(
                                  text = inputText,
                                  isBold = isBold,
                                  isItalic = isItalic,
                                  isUnderline = isUnderline,
                                  fontSize = selectedSize.toIntOrNull() ?: 10,
                                  colorInt = cInt
                              )
                              inputText = ""
                          }
                      }
                  ) {
                      Text("Send", style = win95TextStyle)
                  }'''
replacement_input = '''              // Input Area
              Row(modifier = Modifier.fillMaxWidth().height(androidx.compose.foundation.layout.IntrinsicSize.Min), verticalAlignment = Alignment.CenterVertically) {
                  Win95TextField(
                      value = inputText,
                      onValueChange = { inputText = it },
                      modifier = Modifier.weight(1f),
                      textStyle = win95TextStyle.copy(
                          color = selectedColor,
                          fontWeight = if (isBold) FontWeight.Bold else FontWeight.Normal,
                          fontStyle = if (isItalic) FontStyle.Italic else FontStyle.Normal,
                          fontSize = (selectedSize.toIntOrNull() ?: 10).sp * 2
                      )
                  )
                  Spacer(modifier = Modifier.width(8.dp))
                  Win95Button(
                      modifier = Modifier.fillMaxHeight(),
                      onClick = { 
                          if (inputText.isNotBlank()) {
                              val r = (selectedColor.red * 255).toInt()
                              val g = (selectedColor.green * 255).toInt()
                              val b = (selectedColor.blue * 255).toInt()
                              val cInt = (b shl 16) or (g shl 8) or r
                              viewModel.sendMessage(
                                  text = inputText,
                                  isBold = isBold,
                                  isItalic = isItalic,
                                  isUnderline = isUnderline,
                                  fontSize = selectedSize.toIntOrNull() ?: 10,
                                  colorInt = cInt
                              )
                              inputText = ""
                          }
                      }
                  ) {
                      Text(stringResource(R.string.send), style = win95TextStyle)
                  }'''
content = content.replace(target_input, replacement_input)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied minor UI tweaks")
