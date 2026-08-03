import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update WM_CREATE of MoveRoomProc
create_old = r'''    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260); // 270'''
        
create_new = r'''    case WM_CREATE: {
        int margin = Scale(10);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260); // 280'''

content = content.replace(create_old, create_new)

# 2. Update ShowMoveRoomDialog
show_old = r'''    int clientW = Scale(270);
    int clientH = Scale(168);'''
show_new = r'''    int clientW = Scale(280);
    int clientH = Scale(188);'''

content = content.replace(show_old, show_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated MoveRoomProc margins")
