import re
import sys

with open('Source.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

matches = set(re.findall(r'L"([^"]*[\x80-\uFFFF]+[^"]*)"', text))
with open('ja_strings.txt', 'w', encoding='utf-8') as out:
    for m in matches:
        out.write(m + '\n')
