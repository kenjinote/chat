import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add GetDynamicCrops function above ApplyClassicStyle
get_dynamic_crops_code = r"""
void GetDynamicCrops(HWND hWnd, RECT rcWindow, int dpi, int captionHeight, 
                     int& cxCrop, int& cyCropTop, int& cyCropBottom) 
{
    RECT rcFrame = { 0, 0, 0, 0 };
    AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), dpi);
    int bw1 = MulDiv(1, dpi, 96); if (bw1 < 1) bw1 = 1;
    int bw3 = bw1 * 3;

    cxCrop = -rcFrame.left - bw3;
    cyCropTop = -rcFrame.top - captionHeight - bw3;
    cyCropBottom = rcFrame.bottom - bw3;

    HMONITOR hMonitor = MonitorFromRect(&rcWindow, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(hMonitor, &mi)) {
        bool isDwmSnapped = IsZoomed(hWnd) || 
            (rcWindow.top == mi.rcWork.top && rcWindow.bottom >= mi.rcWork.bottom);
            
        if (isDwmSnapped) {
            cyCropTop = 0; 
            cyCropBottom = rcFrame.bottom;
        }
    }

    if (cxCrop < 0) cxCrop = 0;
    if (cyCropTop < 0) cyCropTop = 0;
    if (cyCropBottom < 0) cyCropBottom = 0;
}

void ApplyClassicStyle(HWND hWnd, int width, int height) {
"""
content = re.sub(r'void ApplyClassicStyle\(HWND hWnd, int width, int height\) \{', get_dynamic_crops_code, content, count=1)

# 2. Update ApplyClassicStyle to use it
apply_classic = r"""    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
    int cxCrop, cyCropTop, cyCropBottom;
    GetDynamicCrops(hWnd, rcWindow, g_dpi, captionHeight, cxCrop, cyCropTop, cyCropBottom);

    HRGN hRgn = CreateRectRgn(cxCrop, cyCropTop, width - cxCrop, height - cyCropBottom);"""
content = re.sub(r'    RECT rcFrame = \{ 0, 0, 0, 0 \};\s*AdjustWindowRectExForDpi\(.*?\);\s*int cxCrop = -rcFrame\.left - 3;\s*int cyCropTop = -rcFrame\.top - GetSystemMetricsForDpi\(SM_CYCAPTION, g_dpi\) - 3;\s*int cyCropBottom = rcFrame\.bottom - 3;\s*if \(cxCrop < 0\) cxCrop = 0;\s*if \(cyCropTop < 0\) cyCropTop = 0;\s*if \(cyCropBottom < 0\) cyCropBottom = 0;\s*HRGN hRgn = CreateRectRgn\(cxCrop, cyCropTop, width - cxCrop, height - cyCropBottom\);', apply_classic, content, count=1, flags=re.DOTALL)

# 3. Update WM_WINDOWPOSCHANGING to remove the useless modification of pwp->y
windowpos_regex = r'(bool isTopEdge = \(pwp->y == mi\.rcWork\.top\);\s*bool isBottomEdge = \(pwp->y \+ pwp->cy == mi\.rcWork\.bottom\);\s*)if \(isLeftEdge \|\| isRightEdge \|\| isTopEdge \|\| isBottomEdge\) \{.*?pwp->cy \+= cyCropTop \+ cyCropBottom;\s*\}'
content = re.sub(windowpos_regex, r'\1', content, count=1, flags=re.DOTALL)

# 4. Update WM_NCHITTEST
nchittest = r"""        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
        int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
        int cxCrop, cyCropTop, cyCropBottom;
        GetDynamicCrops(hWnd, rcWindow, g_dpi, captionHeight, cxCrop, cyCropTop, cyCropBottom);

        int visLeft = cxCrop;
        int visTop = cyCropTop;
        int visRight = width - cxCrop;
        int visBottom = height - cyCropBottom;"""
content = re.sub(r'        RECT rcFrame = \{ 0, 0, 0, 0 \};\s*AdjustWindowRectExForDpi\(.*?\);\s*int cxCrop = -rcFrame\.left - 3;\s*int cyCropTop = -rcFrame\.top - GetSystemMetricsForDpi\(SM_CYCAPTION, g_dpi\) - 3;\s*int cyCropBottom = rcFrame\.bottom - 3;\s*if \(cxCrop < 0\) cxCrop = 0;\s*if \(cyCropTop < 0\) cyCropTop = 0;\s*if \(cyCropBottom < 0\) cyCropBottom = 0;\s*int visLeft = cxCrop;\s*int visTop = cyCropTop;\s*int visRight = width - cxCrop;\s*int visBottom = height - cyCropBottom;', nchittest, content, count=1, flags=re.DOTALL)

# 5. Update WM_NCPAINT
ncpaint = r"""                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
                int cxCrop, cyCropTop, cyCropBottom;
                GetDynamicCrops(hWnd, rcWindow, g_dpi, captionHeight, cxCrop, cyCropTop, cyCropBottom);

                int visLeft = cxCrop;
                int visTop = cyCropTop;
                int visRight = width - cxCrop;
                int visBottom = height - cyCropBottom;"""
content = re.sub(r'                RECT rcFrame = \{ 0, 0, 0, 0 \};\s*AdjustWindowRectExForDpi\(.*?\);\s*int cxCrop = -rcFrame\.left - 3;\s*int cyCropTop = -rcFrame\.top - GetSystemMetricsForDpi\(SM_CYCAPTION, g_dpi\) - 3;\s*int cyCropBottom = rcFrame\.bottom - 3;\s*if \(cxCrop < 0\) cxCrop = 0;\s*if \(cyCropTop < 0\) cyCropTop = 0;\s*if \(cyCropBottom < 0\) cyCropBottom = 0;\s*int visLeft = cxCrop;\s*int visTop = cyCropTop;\s*int visRight = width - cxCrop;\s*int visBottom = height - cyCropBottom;', ncpaint, content, count=1, flags=re.DOTALL)

# 6. Fill the gap in WM_NCPAINT
gap_fill = r"""                RECT rcCaption = { visLeft + bw3, visTop + bw3, visRight - bw3, visTop + bw3 + captionHeight };
                if (rcCaption.bottom > -rcFrame.top) rcCaption.bottom = -rcFrame.top;
                
                // If DWM Snapped, cyCropTop is 0. But the OS client area still starts at 31 (-rcFrame.top).
                // So there is a gap between our 23px caption and the client area! We must fill it with the window background color.
                if (cyCropTop == 0 && -rcFrame.top > rcCaption.bottom) {
                    RECT rcGap = { visLeft + bw3, rcCaption.bottom, visRight - bw3, -rcFrame.top };
                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &rcGap, hBg);
                    DeleteObject(hBg);
                }"""
content = re.sub(r'                RECT rcCaption = \{ visLeft \+ bw3, visTop \+ bw3, visRight - bw3, visTop \+ bw3 \+ captionHeight \};\s*if \(rcCaption\.bottom > -rcFrame\.top\) rcCaption\.bottom = -rcFrame\.top;', gap_fill, content, count=1)


with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Done")
