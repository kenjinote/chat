import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
final2 = 'MainActivity_final2.kt'
vm = 'app/src/main/java/com/example/chat/ChatViewModel.kt'

with open(final2, 'r', encoding='utf-8') as f:
    content2 = f.read()

# Extract Win95VideoWindow from final2
m = re.search(r'@Composable\s*fun Win95VideoWindow.*?^}', content2, re.MULTILINE | re.DOTALL)
if m:
    video_window_code = m.group(0)
    
    with open(main, 'r', encoding='utf-8') as f:
        main_content = f.read()
    main_content += '\n\n' + video_window_code
    with open(main, 'w', encoding='utf-8') as f:
        f.write(main_content)
else:
    print("Could not find Win95VideoWindow in final2")

# Edit ChatViewModel.kt to add watchVideo and make videoEngine public
with open(vm, 'r', encoding='utf-8') as f:
    vm_content = f.read()

vm_content = vm_content.replace('private val videoEngine', 'val videoEngine')
if 'val watchingUserId = MutableStateFlow' not in vm_content:
    vm_content = vm_content.replace('class ChatViewModel(application: Application) : AndroidViewModel(application) {',
        'class ChatViewModel(application: Application) : AndroidViewModel(application) {\n    val watchingUserId = MutableStateFlow<String?>(null)\n    fun watchVideo(userId: String?) { watchingUserId.value = userId }\n')

with open(vm, 'w', encoding='utf-8') as f:
    f.write(vm_content)
print("Patched VM and Main")
