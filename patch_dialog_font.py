import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    content = content.replace(
        "HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);",
        "HFONT hFont = g_hFont;"
    )

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched dialog fonts")

if __name__ == '__main__':
    patch()
