import os

filepath = r'app/src/main/java/com/example/chat/MainActivity.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''                                        Box(modifier = Modifier.height(32.dp)) {
                                            if (isStreaming) {
                                                Image(painter = painterResource(id = R.drawable.ic_tv_win), contentDescription = "TV", modifier = Modifier.fillMaxHeight())
                                            }
                                            Image(painter = painterResource(id = R.drawable.ic_smile_win), contentDescription = "User", modifier = Modifier.fillMaxHeight())
                                        }'''

new_code = '''                                        Row(modifier = Modifier.height(32.dp), verticalAlignment = Alignment.CenterVertically) {
                                            if (isStreaming) {
                                                Image(painter = painterResource(id = R.drawable.ic_tv_win), contentDescription = "TV", modifier = Modifier.size(32.dp))
                                                Spacer(modifier = Modifier.width(4.dp))
                                            }
                                            Image(painter = painterResource(id = R.drawable.ic_smile_win), contentDescription = "User", modifier = Modifier.size(32.dp))
                                        }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replaced icon code successfully.")
else:
    print("Could not find old_code.")
