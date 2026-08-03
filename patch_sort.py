import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove WM_SETREDRAW from SortUserList
content = content.replace('SendMessage(g_hUserList, WM_SETREDRAW, FALSE, 0);\n', '')
content = content.replace('SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);\n', '')

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Removed WM_SETREDRAW from SortUserList")
