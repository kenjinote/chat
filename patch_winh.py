import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Update ShowMoveRoomDialog
move_old = r'''    int winW = (rcDlg.right - rcDlg.left) + bw3 * 2;
    int winH = (rcDlg.bottom - rcDlg.top) + bw3 + (currentTop - targetTop);'''

move_new = r'''    int winW = (rcDlg.right - rcDlg.left) + bw3 * 2;
    int winH = (rcDlg.bottom - rcDlg.top) + bw3 - (currentTop - targetTop);'''

if move_old in content:
    content = content.replace(move_old, move_new, 1) # First occurrence should be MoveRoom or ChangeName
    content = content.replace(move_old, move_new, 1) # Second occurrence
    
with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated window height compensation")
