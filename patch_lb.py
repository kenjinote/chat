import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # In UpdateFonts, we need to add LB_SETITEMHEIGHT for g_hUserList
    old_update_fonts = "if (g_hUserList) SendMessage(g_hUserList, WM_SETFONT, (WPARAM)g_hFont, TRUE);"
    new_update_fonts = old_update_fonts + "\n    if (g_hUserList) SendMessage(g_hUserList, LB_SETITEMHEIGHT, 0, (LPARAM)Scale(28));"
    
    content = content.replace(old_update_fonts, new_update_fonts)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched LB_SETITEMHEIGHT")

if __name__ == '__main__':
    patch()
