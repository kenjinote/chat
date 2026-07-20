import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Find the top of the file after the includes and add `extern HFONT g_hFont;`
    # Let's just insert it before `bool HandleMenuMeasureItem`
    content = content.replace(
        "bool HandleMenuMeasureItem(HWND hWnd, LPMEASUREITEMSTRUCT lpmis) {",
        "extern HFONT g_hFont;\nextern inline int Scale(int x);\nbool HandleMenuMeasureItem(HWND hWnd, LPMEASUREITEMSTRUCT lpmis) {"
    )
    
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched extern HFONT g_hFont")

if __name__ == '__main__':
    patch()
