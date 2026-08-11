import re
vm = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(vm, 'r', encoding='utf-8') as f:
    vm_content = f.read()

vm_content = vm_content.replace('val videoEngine = VideoEngine(', 'private val videoEngine = VideoEngine(')

with open(vm, 'w', encoding='utf-8') as f:
    f.write(vm_content)

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    main_content = f.read()

main_content = main_content.replace('viewModel.videoEngine', 'viewModel.getVideoEngine()')

with open(main, 'w', encoding='utf-8') as f:
    f.write(main_content)

print("Fixed VideoEngine clash")
