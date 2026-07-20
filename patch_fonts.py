import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    update_fonts_code = """
void UpdateFonts(HWND hWnd) {
    if (g_hFont) DeleteObject(g_hFont);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hFontI_btn) DeleteObject(g_hFontI_btn);
    if (g_hFontU_btn) DeleteObject(g_hFontU_btn);
    if (g_hFontB_btn) DeleteObject(g_hFontB_btn);
    if (g_hFontCombo) DeleteObject(g_hFontCombo);

    g_hFont = CreateFontW(Scale(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS UI Gothic");
    g_hFontBold = CreateFontW(Scale(12), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS UI Gothic");
    g_hFontI_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Georgia");
    g_hFontU_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hFontB_btn = CreateFontW(Scale(15), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Georgia");
    g_hFontCombo = CreateFontW(Scale(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    if (g_hUserList) SendMessage(g_hUserList, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hUserListHeader) SendMessage(g_hUserListHeader, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hSendBtn) SendMessage(g_hSendBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hBtnB) SendMessage(g_hBtnB, WM_SETFONT, (WPARAM)g_hFontB_btn, TRUE);
    if (g_hBtnI) SendMessage(g_hBtnI, WM_SETFONT, (WPARAM)g_hFontI_btn, TRUE);
    if (g_hBtnU) SendMessage(g_hBtnU, WM_SETFONT, (WPARAM)g_hFontU_btn, TRUE);
    if (g_hBtnVideo) SendMessage(g_hBtnVideo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hComboSize) SendMessage(g_hComboSize, WM_SETFONT, (WPARAM)g_hFontCombo, TRUE);
    if (g_hBtnMoveRoom) SendMessage(g_hBtnMoveRoom, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hBtnColor) SendMessage(g_hBtnColor, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hBtnVoice) SendMessage(g_hBtnVoice, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hBtnMic) SendMessage(g_hBtnMic, WM_SETFONT, (WPARAM)g_hFont, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
"""
    content = content.replace("LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {", update_fonts_code)
    
    # Patch WM_DPICHANGED
    old_dpichanged = """    case WM_DPICHANGED: {
        g_dpi = HIWORD(wParam);
        RECT* const prcNewWindow = (RECT*)lParam;"""
        
    new_dpichanged = """    case WM_DPICHANGED: {
        g_dpi = HIWORD(wParam);
        UpdateFonts(hWnd);
        RECT* const prcNewWindow = (RECT*)lParam;"""
        
    content = content.replace(old_dpichanged, new_dpichanged)
    
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched Source.cpp with fonts")

if __name__ == '__main__':
    patch()
