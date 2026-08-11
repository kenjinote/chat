import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

video_block = '''
        if (watchingUserId != null) {
            Win95VideoWindow(
                title = "Video - ",
                onClose = { viewModel.setWatchingUserId(null) },
                videoEngine = viewModel.getVideoEngine()
            )
        }
'''

# Find the end of ChatScreen, which is marked by:
#                 }
#             }
#         }
#     }
# }
# 
# 
# @Composable

target = r'(\s*\}\s*\}\s*\}\s*\n\n\s*@Composable\s*fun Win95VideoWindow)'
match = re.search(target, content, re.MULTILINE)
if match:
    # insert before the last 2 closing braces of ChatScreen?
    # Wait, ChatScreen ends with:
    #             } // ends Row
    #         } // ends Column
    #     } // ends Box
    # } // ends ChatScreen
    # Let's just insert it right before     }\n} which closes Box and ChatScreen.
    
    # Actually, let's just find         }\n    }\n} at the end of ChatScreen.
    # It's safer to just replace         }\n    }\n}\n\n\n@Composable (whatever spaces there are)
    
    def repl(m):
        return '        }' + video_block + '    }\n}\n\n\n@Composable\nfun Win95VideoWindow'
        
    content = re.sub(r'\s*\}\s*\}\s*\}\s*@Composable\s*fun Win95VideoWindow', repl, content)
    print("Re-inserted video block.")
else:
    print("Could not find target.")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
