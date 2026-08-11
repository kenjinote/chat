import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Modify the Row
content = re.sub(
    r'Row\(modifier = Modifier\.fillMaxWidth\(\), verticalAlignment = Alignment\.CenterVertically\) \{\s+Win95TextField\(',
    r'Row(modifier = Modifier.fillMaxWidth().height(androidx.compose.foundation.layout.IntrinsicSize.Min), verticalAlignment = Alignment.CenterVertically) {\n                Win95TextField(',
    content
)

# 2. Modify the Win95Button
content = re.sub(
    r'Win95Button\(\s*onClick = \{\s*if \(inputText\.isNotBlank\(\)\)',
    r'Win95Button(\n                    modifier = Modifier.fillMaxHeight().padding(vertical = 2.dp),\n                    onClick = {\n                        if (inputText.isNotBlank())',
    content
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied layout fixes")
