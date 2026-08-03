import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix WM_NCHITTEST
nchit = r'''        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectExForDpi(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE), g_dpi);
        int cxCrop = -rcFrame.left - bw3;
        int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - bw3;
        if (cxCrop < 0) cxCrop = 0;
        if (cyCropTop < 0) cyCropTop = 0;

        int visLeft = cxCrop;
        int visRight = (rc.right - rc.left) - cxCrop;
        int visTop = cyCropTop;
        int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
        RECT rcCaption = { visLeft + 3, visTop + 3, visRight - 3, visTop + 3 + captionHeight };
        if (rcCaption.bottom > -rcFrame.top) rcCaption.bottom = -rcFrame.top;'''
nchit_new = r'''        int visLeft = 0;
        int visRight = (rc.right - rc.left);
        int visTop = 0;
        int captionHeight = GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
        RECT rcCaption = { visLeft + bw3, visTop + bw3, visRight - bw3, visTop + bw3 + captionHeight };'''
content = content.replace(nchit, nchit_new)


with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated NCHITTEST")
