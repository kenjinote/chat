import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    main_content = f.read()

main_content = main_content.replace('import detectDragGestures', 'import androidx.compose.foundation.gestures.detectDragGestures')

with open(main, 'w', encoding='utf-8') as f:
    f.write(main_content)

print("Fixed import")
