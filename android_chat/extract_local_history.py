import sys
import re

def extract_strings(filename):
    with open(filename, 'rb') as f:
        data = f.read()
    
    # search for package com.example.chat followed by text
    # the file might have null bytes or lengths interleaved
    # Let's just use a regex for printable chars (including newlines and tabs)
    # We want a very long string that contains package com.example.chat
    
    pattern = re.compile(b'package com\\.example\\.chat[\\s\\S]{5000,30000}?class MainActivity')
    matches = pattern.finditer(data)
    
    count = 0
    for match in matches:
        text = match.group(0).decode('utf-8', errors='ignore')
        with open(f'recovered_{count}.kt', 'w', encoding='utf-8') as out:
            out.write(text)
        print(f"Found match {count}, length: {len(text)}")
        count += 1
        
extract_strings(r'C:\Users\kenjinote\AppData\Local\Google\AndroidStudio2025.2.2\LocalHistory\changes.storageData')
