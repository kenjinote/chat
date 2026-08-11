import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''                                val isStreaming = videoStreamers.containsKey(user.userId)
                                val prefix = "😊" + (if(isStreaming) "📺" else "")
                                val finalDisplayUser = prefix + displayUser
                                val isMenuExpanded = menuExpandedUserId == user.userId
                                
                                val baseColor = if (user.nclr != -1) {
                                    val b = (user.nclr shr 16) and 0xFF
                                    val g = (user.nclr shr 8) and 0xFF
                                    val r = user.nclr and 0xFF
                                    Color(r, g, b, 255)
                                } else Win95Black
                                
                                val textColor = if (isMenuExpanded) Color.White else baseColor
                                val bgColor = if (isMenuExpanded) Color(0xFF000080) else Color.Transparent

                                Box(modifier = Modifier.fillMaxWidth().background(bgColor).clickable {
                                    menuExpandedUserId = user.userId
                                }) {
                                    Text(
                                        text = finalDisplayUser,
                                        style = win95TextStyle.copy(fontSize = 16.sp, color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal),
                                        modifier = Modifier.padding(horizontal = 0.5.dp, vertical = 0.5.dp).fillMaxWidth()
                                    )'''

new_code = '''                                val isStreaming = videoStreamers.containsKey(user.userId)
                                val isMenuExpanded = menuExpandedUserId == user.userId
                                
                                val baseColor = if (user.nclr != -1) {
                                    val b = (user.nclr shr 16) and 0xFF
                                    val g = (user.nclr shr 8) and 0xFF
                                    val r = user.nclr and 0xFF
                                    Color(r, g, b, 255)
                                } else Win95Black
                                
                                val textColor = if (isMenuExpanded) Color.White else baseColor
                                val bgColor = if (isMenuExpanded) Color(0xFF000080) else Color.Transparent

                                Box(modifier = Modifier.fillMaxWidth().background(bgColor).clickable {
                                    menuExpandedUserId = user.userId
                                }) {
                                    Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(horizontal = 0.5.dp, vertical = 0.5.dp).fillMaxWidth()) {
                                        Box(modifier = Modifier.size(16.dp)) {
                                            if (isStreaming) {
                                                Image(painter = painterResource(id = R.drawable.ic_tv), contentDescription = "TV", modifier = Modifier.fillMaxSize())
                                            }
                                            Image(painter = painterResource(id = R.drawable.ic_smile), contentDescription = "User", modifier = Modifier.fillMaxSize())
                                        }
                                        Spacer(modifier = Modifier.width(4.dp))
                                        Text(
                                            text = displayUser,
                                            style = win95TextStyle.copy(fontSize = 16.sp, color = textColor, fontWeight = if (isMe) FontWeight.Bold else FontWeight.Normal)
                                        )
                                    }'''

content = content.replace(old_code, new_code)

if 'import androidx.compose.foundation.Image' not in content:
    content = content.replace('import androidx.compose.foundation.layout.*', 'import androidx.compose.foundation.layout.*\nimport androidx.compose.foundation.Image')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated user list items to use icon images")
