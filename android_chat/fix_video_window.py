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

# 2. Move Win95VideoWindow to the end of the Box
video_window_pattern = r'(\s*if\s*\(watchingUserId\s*!=\s*null\)\s*\{\s*Win95VideoWindow\([^)]+\)\s*\})'
match = re.search(video_window_pattern, content, re.MULTILINE | re.DOTALL)

if match:
    video_block = match.group(1)
    # Remove from current position
    content = content.replace(video_block, '', 1)
    
    # We know the Box ends with             } before the         } of the main content?
    # Let's find the end of the Box.
    # The Box starts at Box(modifier = Modifier.fillMaxSize().background(Win95Gray)) {
    # We can just insert the video_block before the last closing brace of the main composable, but it needs to be inside the Box.
    # Actually, the Box wraps the whole screen.
    # Let's find Column(modifier = Modifier.fillMaxSize()) { and the corresponding end.
    # It's easier to use a regex to find the end of the Column that is inside the Box.
    # Let's just insert it right before the } that closes the Box. 
    # Since I don't want to parse braces, I can search for a known marker near the end of the Box.
    # Let's look at the end of the Box in MainActivity.kt
