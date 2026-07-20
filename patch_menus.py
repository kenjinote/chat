import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Menus: Replace WM_GETFONT fallback logic
    old_menu_font = r'HFONT hFont = \(HFONT\)SendMessage\(hWnd, WM_GETFONT, 0, 0\);\s*if \(!hFont\) hFont = \(HFONT\)GetStockObject\(DEFAULT_GUI_FONT\);'
    new_menu_font = r'HFONT hFont = g_hFont;'
    content = re.sub(old_menu_font, new_menu_font, content)

    # 2. Trackbar: Add TBS_FIXEDLENGTH to creation
    content = content.replace(
        "WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS",
        "WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS | TBS_FIXEDLENGTH"
    )

    # 3. Trackbar: Send TBM_SETTHUMBLENGTH in UpdateFonts
    # Also we should send it once after creation in WM_CREATE
    # UpdateFonts already exists, let's insert the message at the end
    update_fonts_end = "if (g_hBtnMic) SendMessage(g_hBtnMic, WM_SETFONT, (WPARAM)g_hFont, TRUE);\n}"
    
    # Wait, in the previous patch I removed g_hBtnMic from Source.cpp!
    # So UpdateFonts ends with `if (g_hBtnVoice) SendMessage(g_hBtnVoice, WM_SETFONT, (WPARAM)g_hFont, TRUE);\n}`
    # Let's just find `void UpdateFonts` block and append.
    
    # A safer way to append to UpdateFonts:
    old_update_fonts = "if (g_hBtnVoice) SendMessage(g_hBtnVoice, WM_SETFONT, (WPARAM)g_hFont, TRUE);\n}"
    new_update_fonts = "if (g_hBtnVoice) SendMessage(g_hBtnVoice, WM_SETFONT, (WPARAM)g_hFont, TRUE);\n    if (g_hVolumeSlider) SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);\n}"
    content = content.replace(old_update_fonts, new_update_fonts)

    # 4. We should also send TBM_SETTHUMBLENGTH right after CreateWindow for g_hVolumeSlider
    old_trackbar_create = """        SendMessage(g_hVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));"""
    new_trackbar_create = """        SendMessage(g_hVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));\n        SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);"""
    content = content.replace(old_trackbar_create, new_trackbar_create)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched menus and trackbar")

if __name__ == '__main__':
    patch()
