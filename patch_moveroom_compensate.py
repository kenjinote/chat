import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Revert MoveRoomProc WM_CREATE to simple layout
create_old = r'''    case WM_CREATE: {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        
        int desiredClientW = Scale(270);
        int desiredClientH = margin * 4 + inputHeight + listHeight + btnHeight;
        
        if (rcClient.right != desiredClientW || rcClient.bottom != desiredClientH) {
            RECT rcWin;
            GetWindowRect(hWnd, &rcWin);
            SetWindowPos(hWnd, NULL, 0, 0, 
                (rcWin.right - rcWin.left) + (desiredClientW - rcClient.right), 
                (rcWin.bottom - rcWin.top) + (desiredClientH - rcClient.bottom), 
                SWP_NOMOVE | SWP_NOZORDER);
        }
        
        int clientW = desiredClientW;
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            margin, y, clientW - margin * 2, inputHeight, hWnd, (HMENU)101, GetModuleHandle(0), 0);
        y += inputHeight + margin;
        
        hList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
            margin, y, clientW - margin * 2, listHeight, hWnd, (HMENU)102, GetModuleHandle(0), 0);
        y += listHeight + margin;

        int cancelX = clientW - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;'''

create_new = r'''    case WM_CREATE: {
        int margin = Scale(5);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        int clientW = Scale(270);
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            margin, y, clientW - margin * 2, inputHeight, hWnd, (HMENU)101, GetModuleHandle(0), 0);
        y += inputHeight + margin;
        
        hList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
            margin, y, clientW - margin * 2, listHeight, hWnd, (HMENU)102, GetModuleHandle(0), 0);
        y += listHeight + margin;

        int cancelX = clientW - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;'''

if create_old in content:
    content = content.replace(create_old, create_new)
    print("Reverted MoveRoomProc WM_CREATE")
else:
    print("Could not find create_old")


# 2. Update ShowMoveRoomDialog to pre-compensate window size
show_old = r'''    int clientW = Scale(280);
    int clientH = Scale(188);
    RECT rcDlg = { 0, 0, clientW, clientH };
    AdjustWindowRectExForDpi(&rcDlg, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME, g_dpi);
    int winW = rcDlg.right - rcDlg.left;
    int winH = rcDlg.bottom - rcDlg.top;'''

show_new = r'''    int margin = Scale(5);
    int clientW = Scale(270);
    int clientH = margin * 4 + Scale(24) + Scale(100) + Scale(24);
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
    print("Updated ShowMoveRoomDialog")
else:
    print("Could not find show_old")


with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
