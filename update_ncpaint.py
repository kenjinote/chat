import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix WM_NCPAINT
ncpaint = r'''                int cxCrop = -rcFrame.left - bw3;
                int cyCropTop = -rcFrame.top - GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi) - bw3;
                int cyCropBottom = rcFrame.bottom - bw3;
                if (cxCrop < 0) cxCrop = 0;
                if (cyCropTop < 0) cyCropTop = 0;
                if (cyCropBottom < 0) cyCropBottom = 0;

                int visLeft = cxCrop;
                int visTop = cyCropTop;
                int visRight = width - cxCrop;
                int visBottom = height - cyCropBottom;'''
ncpaint_new = r'''                int visLeft = 0;
                int visTop = 0;
                int visRight = width;
                int visBottom = height;'''
content = content.replace(ncpaint, ncpaint_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated NCPAINT")
