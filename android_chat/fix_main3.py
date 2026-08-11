import re

vm = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(vm, 'r', encoding='utf-8') as f:
    vm_content = f.read()

# Add watchVideo method to ChatViewModel
if 'fun watchVideo' not in vm_content:
    vm_content = vm_content.replace('class ChatViewModel(application: Application) : AndroidViewModel(application) {',
        'class ChatViewModel(application: Application) : AndroidViewModel(application) {\n    fun watchVideo(userId: String?) { watchingUserId.value = userId }\n')

with open(vm, 'w', encoding='utf-8') as f:
    f.write(vm_content)


main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    main_content = f.read()

imports = '''import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.gestures.detectDragGestures'''

if 'androidx.compose.ui.input.pointer.pointerInput' not in main_content:
    main_content = main_content.replace('import androidx.compose.ui.Modifier', imports + '\nimport androidx.compose.ui.Modifier')

with open(main, 'w', encoding='utf-8') as f:
    f.write(main_content)

print("Fixed again")
