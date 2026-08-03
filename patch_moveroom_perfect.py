import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

create_old = r'''    case WM_CREATE: {
        int margin = Scale(10);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        int totalWidth = margin * 2 + Scale(260); // 280
        
        int y = margin;
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            margin, y, totalWidth - margin * 2, inputHeight, hWnd, (HMENU)101, GetModuleHandle(0), 0);
        y += inputHeight + margin;
        
        hList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
            margin, y, totalWidth - margin * 2, listHeight, hWnd, (HMENU)102, GetModuleHandle(0), 0);
        y += listHeight + margin;

        int cancelX = totalWidth - margin - btnWidth;
        int okX = cancelX - margin - btnWidth;
        
        hChk = CreateWindow(L"BUTTON", GetString(IDS_PRIVATE).c_str(), 
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            margin, y, chkWidth, btnHeight, hWnd, (HMENU)103, GetModuleHandle(0), 0);
            
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            okX, y, btnWidth, btnHeight, hWnd, (HMENU)1, GetModuleHandle(0), 0);
            
        hBtnCancel = CreateWindow(L"BUTTON", GetString(IDS_CANCEL).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            cancelX, y, btnWidth, btnHeight, hWnd, (HMENU)2, GetModuleHandle(0), 0);'''

create_new = r'''    case WM_CREATE: {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        
        int margin = Scale(10);
        int inputHeight = Scale(24);
        int listHeight = Scale(100);
        int btnHeight = Scale(24);
        int btnWidth = Scale(80);
        int chkWidth = Scale(80);
        
        int desiredClientW = Scale(280);
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
        int okX = cancelX - margin - btnWidth;
        
        hChk = CreateWindow(L"BUTTON", GetString(IDS_PRIVATE).c_str(), 
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            margin, y, chkWidth, btnHeight, hWnd, (HMENU)103, GetModuleHandle(0), 0);
            
        hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            okX, y, btnWidth, btnHeight, hWnd, (HMENU)1, GetModuleHandle(0), 0);
            
        hBtnCancel = CreateWindow(L"BUTTON", GetString(IDS_CANCEL).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            cancelX, y, btnWidth, btnHeight, hWnd, (HMENU)2, GetModuleHandle(0), 0);'''

if create_old in content:
    content = content.replace(create_old, create_new)
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully patched MoveRoomProc margins.")
else:
    print("Could not find create_old block in Source.cpp.")
