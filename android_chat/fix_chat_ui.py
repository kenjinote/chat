import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

pattern = r'Row\(modifier = Modifier\.fillMaxWidth\(\)\.padding\(vertical = 4\.dp\)\) \{\s*if \(msg\.type != "system" && msg\.type != "sysmsg"\) \{\s*val uName = msg\.userName \?: "Unknown"\s*val displayUser = if \(uName\.contains\("#"\)\) uName\.substringBefore\("#"\) else uName\s*val nclr = msg\.nclr \?: -1\s*val userColor = if \(nclr != -1L\) \{\s*val b = \(\(nclr shr 16\) and 0xFF\)\.toInt\(\)\s*val g = \(\(nclr shr 8\) and 0xFF\)\.toInt\(\)\s*val r = \(nclr and 0xFF\)\.toInt\(\)\s*Color\(r, g, b, 255\)\s*\} else Win95Black\s*Text\(text = "\$\{displayUser\}: ", fontWeight = FontWeight\.Bold, color = userColor, style = win95TextStyle\)\s*\}\s*val contentToDisplay = msg\.msg \?: msg\.content \?: ""\s*val annotatedStr = TextProcessor\.formatMessage\(contentToDisplay\)\s*val msgColor = if \(msg\.type == "system" \|\| msg\.type == "sysmsg"\) Win95DarkGray else \{\s*val c = msg\.color \?: 0\s*if \(c != 0\) \{\s*val b = \(c shr 16\) and 0xFF\s*val g = \(c shr 8\) and 0xFF\s*val r = c and 0xFF\s*Color\(r, g, b, 255\)\s*\} else Win95Black\s*\}\s*val msgFontSize = \(msg\.sz \?: 10\)\.sp \* 2\s*val msgFontWeight = if \(msg\.b == true\) FontWeight\.Bold else FontWeight\.Normal\s*val msgFontStyle = if \(msg\.i == true\) FontStyle\.Italic else FontStyle\.Normal\s*ClickableText\(\s*text = annotatedStr,\s*style = win95TextStyle\.copy\(\s*color = msgColor,\s*fontSize = msgFontSize,\s*fontWeight = msgFontWeight,\s*fontStyle = msgFontStyle\s*\),\s*onClick = \{ offset ->\s*annotatedStr\.getStringAnnotations\(tag = "URL", start = offset, end = offset\)\s*\.firstOrNull\(\)\?\.let \{ annotation -> uriHandler\.openUri\(annotation\.item\) \}\s*\}\s*\)\s*\}'

replacement = '''Box(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                                  val msgColor = if (msg.type == "system" || msg.type == "sysmsg") Win95DarkGray else {
                                      val c = msg.color ?: 0
                                      if (c != 0) {
                                          val b = (c shr 16) and 0xFF
                                          val g = (c shr 8) and 0xFF
                                          val r = c and 0xFF
                                          Color(r, g, b, 255)
                                      } else Win95Black
                                  }
                                  val msgFontSize = (msg.sz ?: 10).sp * 2
                                  val msgFontWeight = if (msg.b == true) FontWeight.Bold else FontWeight.Normal
                                  val msgFontStyle = if (msg.i == true) FontStyle.Italic else FontStyle.Normal
                                  
                                  val builder = androidx.compose.ui.text.AnnotatedString.Builder()
                                  if (msg.type != "system" && msg.type != "sysmsg") {
                                      val uName = msg.userName ?: "Unknown"
                                      val displayUser = if (uName.contains("#")) uName.substringBefore("#") else uName
                                      val nclr = msg.nclr ?: -1
                                      val userColor = if (nclr != -1L) {
                                          val b = ((nclr shr 16) and 0xFF).toInt()
                                          val g = ((nclr shr 8) and 0xFF).toInt()
                                          val r = (nclr and 0xFF).toInt()
                                          Color(r, g, b, 255)
                                      } else Win95Black
                                      
                                      builder.withStyle(androidx.compose.ui.text.SpanStyle(color = userColor, fontWeight = FontWeight.Bold, fontSize = 14.sp)) {
                                          append(":")
                                      }
                                  }
                                  
                                  val contentToDisplay = msg.msg ?: msg.content ?: ""
                                  val annotatedStr = TextProcessor.formatMessage(contentToDisplay)
                                  builder.withStyle(androidx.compose.ui.text.SpanStyle(color = msgColor, fontSize = msgFontSize, fontWeight = msgFontWeight, fontStyle = msgFontStyle)) {
                                      append(annotatedStr)
                                  }
                                  val finalStr = builder.toAnnotatedString()
                                  
                                  ClickableText(
                                      text = finalStr,
                                      style = win95TextStyle,
                                      onClick = { offset ->
                                          finalStr.getStringAnnotations(tag = "URL", start = offset, end = offset)
                                              .firstOrNull()?.let { annotation -> uriHandler.openUri(annotation.item) }
                                      }
                                  )
                              }'''

if re.search(pattern, content):
    content = re.sub(pattern, replacement, content)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replaced Row with Box and single ClickableText.")
else:
    print("Pattern not found! Check regex.")
