import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add WM_NCCALCSIZE to HandleClassicStyle
nccalcsize = r'''    case WM_NCCALCSIZE: {
        if (wParam == TRUE) {
            NCCALCSIZE_PARAMS* pncsp = (NCCALCSIZE_PARAMS*)lParam;
            DefWindowProc(hWnd, WM_NCCALCSIZE, wParam, lParam);
            
            RECT rcFrame = { 0, 0, 0, 0 };
            AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
            
            int bw3 = MulDiv(1, g_dpi, 96) * 3;
            if (bw3 < 3) bw3 = 3;
            int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
            
            int cxCrop = -rcFrame.left - bw3;
            int cyCropTop = -rcFrame.top - captionHeight - bw3;
            int cyCropBottom = rcFrame.bottom - bw3;
            
            if (cxCrop < 0) cxCrop = 0;
            if (cyCropTop < 0) cyCropTop = 0;
            if (cyCropBottom < 0) cyCropBottom = 0;
            
            pncsp->rgrc[0].left -= cxCrop;
            pncsp->rgrc[0].right += cxCrop;
            pncsp->rgrc[0].top -= cyCropTop;
            pncsp->rgrc[0].bottom += cyCropBottom;
            
            *pResult = 0;
            return true;
        }
        break;
    }
    case WM_NCPAINT:'''
content = content.replace('    case WM_NCPAINT:', nccalcsize)

# 2. Update WM_WINDOWPOSCHANGING to remove shifting
windowpos = r'''                if (isLeftEdge || isRightEdge || isTopEdge || isBottomEdge) {
                    RECT rcFrame = { 0, 0, 0, 0 };
                    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
                    int cxCrop = -rcFrame.left - 3;
                    int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - 3;
                    int cyCropBottom = rcFrame.bottom - 3;
                    if (cxCrop < 0) cxCrop = 0;
                    if (cyCropTop < 0) cyCropTop = 0;
                    if (cyCropBottom < 0) cyCropBottom = 0;

                    pwp->x -= cxCrop;
                    pwp->y -= cyCropTop;
                    pwp->cx += cxCrop * 2;
                    pwp->cy += cyCropTop + cyCropBottom;
                }'''
content = content.replace(windowpos, '                // Edge logic removed (handled by WM_NCCALCSIZE)')

# 3. ApplyClassicStyle remove SetWindowRgn
applyclassic = r'''    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
    int cxCrop = -rcFrame.left - 3;
    int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - 3;
    int cyCropBottom = rcFrame.bottom - 3;
    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;
    HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, width - cxCrop, height - cyCropBottom);
    SetWindowRgn(hWnd, hRgn, FALSE);'''
content = content.replace(applyclassic, '    // SetWindowRgn removed (handled by WM_NCCALCSIZE)')

# 4. WM_NCHITTEST logic
nchit = r'''        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
        int cxCrop = -rcFrame.left - 3;
        int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - 3;
        int cyCropBottom = rcFrame.bottom - 3;
        if (cxCrop < 0) cxCrop = 0;
        if (cyCropTop < 0) cyCropTop = 0;
        if (cyCropBottom < 0) cyCropBottom = 0;

        int visLeft = cxCrop;
        int visTop = cyCropTop;
        int visRight = width - cxCrop;
        int visBottom = height - cyCropBottom;'''
nchit_new = r'''        int visLeft = 0;
        int visTop = 0;
        int visRight = width;
        int visBottom = height;'''
content = content.replace(nchit, nchit_new)

# 5. WM_NCPAINT logic
ncpaint = r'''                RECT rcFrame = { 0, 0, 0, 0 };
                AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
                
                int cxCrop = -rcFrame.left - 3;
                int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - 3;
                int cyCropBottom = rcFrame.bottom - 3;
                if (cxCrop < 0) cxCrop = 0;
                if (cyCropTop < 0) cyCropTop = 0;
                if (cyCropBottom < 0) cyCropBottom = 0;

                int visLeft = cxCrop;
                int visTop = cyCropTop;
                int visRight = width - cxCrop;
                int visBottom = height - cyCropBottom;'''
content = content.replace(ncpaint, nchit_new)

# 6. GetWindowMargin logic
margin = r'''RECT GetWindowMargin(HWND hwnd) {
    int dpi = 96;
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        dpi = GetDpiForWindow(hwnd);
        ReleaseDC(hwnd, hdc);
    }
    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE), dpi);
    int cxCrop = -rcFrame.left - 3;
    int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, dpi) - 3;
    int cyCropBottom = rcFrame.bottom - 3;
    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;

    RECT r = { cxCrop, cyCropTop, cxCrop, cyCropBottom };
    return r;
}'''
margin_new = r'''RECT GetWindowMargin(HWND hwnd) {
    RECT r = { 0, 0, 0, 0 };
    return r;
}'''
content = content.replace(margin, margin_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated successfully")
