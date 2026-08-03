import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

create_old = r'''        int margin = Scale(10);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        
        int desiredClientW = Scale(280);'''

create_new = r'''        int margin = Scale(5);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        
        int desiredClientW = Scale(270);'''

if create_old in content:
    content = content.replace(create_old, create_new)
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully patched MoveRoomProc margins to 5.")
else:
    print("Could not find create_old block in Source.cpp.")
