import re
with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace WM_NCHITTEST logic
nchit_old = r'''    case WM_NCHITTEST: {
        POINT pt;
        pt.x = (int)(short)LOWORD(lParam);
        pt.y = (int)(short)HIWORD(lParam);
        RECT rc;
        GetWindowRect(hWnd, &rc);
        
        int grab = 4;
        
        bool bLeft = (pt.x >= rc.left && pt.x < rc.left + grab);
        bool bRight = (pt.x < rc.right && pt.x >= rc.right - grab);
        bool bTop = (pt.y >= rc.top && pt.y < rc.top + grab);
        bool bBottom = (pt.y < rc.bottom && pt.y >= rc.bottom - grab);'''
        
nchit_new = r'''    case WM_NCHITTEST: {
        POINT pt;
        pt.x = (int)(short)LOWORD(lParam);
        pt.y = (int)(short)HIWORD(lParam);
        RECT rc;
        GetWindowRect(hWnd, &rc);

        int dpiY = 96;
        HDC hdc = GetDC(hWnd);
        if (hdc) {
            dpiY = g_dpi;
            ReleaseDC(hWnd, hdc);
        }
        int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
        int bw3 = bw1 * 3;

        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
        int cxCrop = -rcFrame.left;
        int cyCropTop = 0;
        int cyCropBottom = rcFrame.bottom;
        if (cxCrop < 0) cxCrop = 0;
        if (cyCropTop < 0) cyCropTop = 0;
        if (cyCropBottom < 0) cyCropBottom = 0;
        
        int grab = bw3 + Scale(2); // Provide a generous grab margin (3D border + 2px)
        
        bool bLeft = (pt.x >= rc.left + cxCrop && pt.x < rc.left + cxCrop + grab);
        bool bRight = (pt.x < rc.right - cxCrop && pt.x >= rc.right - cxCrop - grab);
        bool bTop = (pt.y >= rc.top + cyCropTop && pt.y < rc.top + cyCropTop + grab);
        bool bBottom = (pt.y < rc.bottom - cyCropBottom && pt.y >= rc.bottom - cyCropBottom - grab);'''

content = content.replace(nchit_old, nchit_new)

# Since we moved the calculation up, we should remove the duplicate calculation below
nchit_dup = r'''        int dpiY = 96;
        HDC hdc = GetDC(hWnd);
        if (hdc) {
            dpiY = g_dpi;
            ReleaseDC(hWnd, hdc);
        }
        int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
        int bw3 = bw1 * 3;

        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
        int cxCrop = -rcFrame.left;
        int cyCropTop = 0;
        if (cxCrop < 0) cxCrop = 0;
        if (cyCropTop < 0) cyCropTop = 0;'''

content = content.replace(nchit_dup, "")

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated WM_NCHITTEST")
