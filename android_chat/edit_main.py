import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update DropdownMenuItem
old_menu = '''                                            DropdownMenuItem(
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle) },
                                                onClick = { 
                                                    viewModel.setWatchingUserId(user.userId)
                                                    menuExpandedUserId = null 
                                                }
                                            )'''

new_menu = '''                                            DropdownMenuItem(
                                                enabled = isStreaming,
                                                text = { Text(stringResource(R.string.watch_video), style = win95TextStyle.copy(color = if (isStreaming) Color.Black else Win95DarkGray)) },
                                                onClick = { 
                                                    viewModel.setWatchingUserId(user.userId)
                                                    menuExpandedUserId = null 
                                                }
                                            )'''

content = content.replace(old_menu, new_menu)

# 2. Move video window
video_block = '''        if (watchingUserId != null) {
            Win95VideoWindow(
                title = "Video - ",
                onClose = { viewModel.setWatchingUserId(null) },
                videoEngine = viewModel.getVideoEngine()
            )
        }\n'''

if video_block in content:
    content = content.replace(video_block, '')
    
    # Target location is right before:
    target = '''        }
    }
}

@Composable
fun Win95VideoWindow'''

    replacement = '''        }
''' + video_block + '''    }
}

@Composable
fun Win95VideoWindow'''

    content = content.replace(target, replacement)
    print("Moved video window successfully.")
else:
    print("Could not find video block.")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
