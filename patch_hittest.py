import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

hit_old = r'''        if (bTop && bLeft) { *pResult = HTTOPLEFT; return true; }
        if (bTop && bRight) { *pResult = HTTOPRIGHT; return true; }
        if (bBottom && bLeft) { *pResult = HTBOTTOMLEFT; return true; }
        if (bBottom && bRight) { *pResult = HTBOTTOMRIGHT; return true; }
        if (bLeft) { *pResult = HTLEFT; return true; }
        if (bRight) { *pResult = HTRIGHT; return true; }
        if (bTop) { *pResult = HTTOP; return true; }
        if (bBottom) { *pResult = HTBOTTOM; return true; }'''

hit_new = r'''        DWORD wStyle = GetWindowLong(hWnd, GWL_STYLE);
        bool hasSizeBox = (wStyle & WS_THICKFRAME) != 0;
        if (hasSizeBox) {
            if (bTop && bLeft) { *pResult = HTTOPLEFT; return true; }
            if (bTop && bRight) { *pResult = HTTOPRIGHT; return true; }
            if (bBottom && bLeft) { *pResult = HTBOTTOMLEFT; return true; }
            if (bBottom && bRight) { *pResult = HTBOTTOMRIGHT; return true; }
            if (bLeft) { *pResult = HTLEFT; return true; }
            if (bRight) { *pResult = HTRIGHT; return true; }
            if (bTop) { *pResult = HTTOP; return true; }
            if (bBottom) { *pResult = HTBOTTOM; return true; }
        } else {
            if (bTop || bBottom || bLeft || bRight) {
                *pResult = HTBORDER;
                return true;
            }
        }'''

if hit_old in content:
    content = content.replace(hit_old, hit_new)
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully patched WM_NCHITTEST in HandleClassicStyle.")
else:
    print("Could not find hit_old block in Source.cpp.")
