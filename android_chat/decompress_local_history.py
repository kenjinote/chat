import zlib
import re
import os

def extract_zlib(filename):
    with open(filename, 'rb') as f:
        data = f.read()
    
    count = 0
    # Common zlib headers
    # 78 01 - No Compression/low
    # 78 9C - Default Compression
    # 78 DA - Best Compression
    
    for i in range(len(data) - 2):
        if data[i] == 0x78 and data[i+1] in [0x01, 0x9c, 0xda]:
            try:
                decompressed = zlib.decompress(data[i:])
                if b'package com.example.chat' in decompressed and b'class MainActivity' in decompressed:
                    text = decompressed.decode('utf-8', errors='ignore')
                    with open(f'history_MainActivity_{count}.kt', 'w', encoding='utf-8') as out:
                        out.write(text)
                    print(f"Found candidate {count}, length: {len(text)}")
                    count += 1
            except zlib.error:
                pass

extract_zlib(r'C:\Users\kenjinote\AppData\Local\Google\AndroidStudio2025.2.2\LocalHistory\changes.storageData')
