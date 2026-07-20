import os

file_path = 'Source.cpp'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

target = "const int AUDIO_BUF_SIZE = 1920;"
replacement = "const int AUDIO_BUF_SIZE = 960;"

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("MTU fix applied")
else:
    print("Target not found")
