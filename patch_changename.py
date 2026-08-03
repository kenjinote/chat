import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update ChangeNameProc WM_CREATE
create_old = r'''    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260);
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            margin, y, totalWidth - margin * 2, inputHeight, hWnd, 0, GetModuleHandle(0), 0);
        
        y += inputHeight + margin;
        
        int cancelX = totalWidth - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;'''

create_new = r'''    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int clientW = Scale(270);
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            margin, y, clientW - margin * 2, inputHeight, hWnd, 0, GetModuleHandle(0), 0);
        
        y += inputHeight + margin;
        
        int cancelX = clientW - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;'''

if create_old in content:
    content = content.replace(create_old, create_new)
    print("Replaced ChangeNameProc WM_CREATE")
else:
    print("Could not find ChangeNameProc WM_CREATE")


# 2. Update ShowChangeNameDialog
show_old = r'''    int margin = Scale(5);
    int totalWidth = margin * 2 + Scale(260);
    int totalHeight = margin * 3 + Scale(24) + Scale(24);
    
    RECT rcDlg = { 0, 0, totalWidth, totalHeight };
    AdjustWindowRectExForDpi(&rcDlg, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME, g_dpi);
    int winW = rcDlg.right - rcDlg.left;
    int winH = rcDlg.bottom - rcDlg.top;'''

show_new = r'''    int margin = Scale(5);
    int clientW = Scale(270);
    int clientH = margin * 3 + Scale(24) + Scale(24);
    
    RECT rcDlg = { 0, 0, clientW, clientH };
    AdjustWindowRectExForDpi(&rcDlg, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME, g_dpi);
    
    int bw1 = MulDiv(1, g_dpi, 96); if (bw1 < 1) bw1 = 1;
    int bw3 = bw1 * 3;
    int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
    int targetTop = captionHeight + bw3;
    int currentTop = -rcDlg.top;
    
    int winW = (rcDlg.right - rcDlg.left) + bw3 * 2;
    int winH = (rcDlg.bottom - rcDlg.top) + bw3 + (currentTop - targetTop);'''

if show_old in content:
    content = content.replace(show_old, show_new)
    print("Replaced ShowChangeNameDialog")
else:
    print("Could not find ShowChangeNameDialog")


with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
