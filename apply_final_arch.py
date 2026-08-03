import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add WM_NCCALCSIZE
nccalcsize = r'''    case WM_NCCALCSIZE: {
        if (wParam == TRUE) {
            NCCALCSIZE_PARAMS* pncsp = (NCCALCSIZE_PARAMS*)lParam;
            DefWindowProc(hWnd, WM_NCCALCSIZE, wParam, lParam);
            
            int dpiY = 96;
            HDC hdc = GetDC(hWnd);
            if (hdc) { dpiY = GetDpiForWindow(hWnd); ReleaseDC(hWnd, hdc); }
            int bw1 = MulDiv(1, dpiY, 96); if (bw1 < 1) bw1 = 1;
            int bw3 = bw1 * 3;
            
            pncsp->rgrc[0].left += bw3;
            pncsp->rgrc[0].right -= bw3;
            pncsp->rgrc[0].bottom -= bw3;
            
            int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, dpiY);
            RECT rcFrame = {0};
            AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), dpiY);
            int targetTop = captionHeight + bw3;
            int currentTop = -rcFrame.top;
            pncsp->rgrc[0].top -= (currentTop - targetTop);
            
            *pResult = 0;
            return true;
        }
        break;
    }
    case WM_NCPAINT:'''
content = content.replace('    case WM_NCPAINT:', nccalcsize)

# 2. Update ApplyClassicStyle fixed crops
applyclassic = r'''    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
    int cxCrop = -rcFrame.left - 3;
    int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - 3;
    int cyCropBottom = rcFrame.bottom - 3;
    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;
    HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, width - cxCrop, height - cyCropBottom);'''
applyclassic_new = r'''    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
    int cxCrop = -rcFrame.left;
    int cyCropTop = 0;
    int cyCropBottom = rcFrame.bottom;
    HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, width - cxCrop, height - cyCropBottom);'''
content = content.replace(applyclassic, applyclassic_new)

# 3. Update WM_NCHITTEST fixed crops
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
nchit_new = r'''        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
        int cxCrop = -rcFrame.left;
        int cyCropTop = 0;
        int cyCropBottom = rcFrame.bottom;

        int visLeft = cxCrop;
        int visTop = cyCropTop;
        int visRight = width - cxCrop;
        int visBottom = height - cyCropBottom;'''
content = content.replace(nchit, nchit_new)

# 4. Update WM_NCPAINT fixed crops
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

# 5. Remove Edge Shifting in WM_WINDOWPOSCHANGING
windowpos_regex = r'(bool isTopEdge = \(pwp->y == mi\.rcWork\.top\);\s*bool isBottomEdge = \(pwp->y \+ pwp->cy == mi\.rcWork\.bottom\);\s*)if \(isLeftEdge \|\| isRightEdge \|\| isTopEdge \|\| isBottomEdge\) \{.*?pwp->cy \+= cyCropTop \+ cyCropBottom;\s*\}'
content = re.sub(windowpos_regex, r'\1// Edge logic removed (fixed crops handle it natively)', content, flags=re.DOTALL)

# 6. Update GetWindowMargin fixed crops
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
    int dpi = 96;
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        dpi = GetDpiForWindow(hwnd);
        ReleaseDC(hwnd, hdc);
    }
    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE), dpi);
    int cxCrop = -rcFrame.left;
    int cyCropTop = 0;
    int cyCropBottom = rcFrame.bottom;

    RECT r = { cxCrop, cyCropTop, cxCrop, cyCropBottom };
    return r;
}'''
content = content.replace(margin, margin_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Applied final architecture!")
