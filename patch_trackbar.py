import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    manifest = """
#pragma comment(linker,"\\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\\"")
"""
    if "Microsoft.Windows.Common-Controls" not in content:
        content = content.replace("#include <windows.h>", "#include <windows.h>\n" + manifest)

    old_prepaint = """                int dpiY = g_dpi;
                int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                RECT rcChannel;
                SendMessage(g_hVolumeSlider, TBM_GETCHANNELRECT, 0, (LPARAM)&rcChannel);
                
                int thumbW = 10 * s;
                int xStart = rcChannel.left + thumbW / 2;
                int xEnd = rcChannel.right - thumbW / 2;
                
                int tick1_Y1 = rcChannel.bottom + 3 * s;
                int tick1_Y2 = tick1_Y1 + 2 * s;
                
                int tick2_Y2 = rcChannel.top - 3 * s;
                int tick2_Y1 = tick2_Y2 - 2 * s;"""

    new_prepaint = """                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                RECT rcChannel;
                SendMessage(g_hVolumeSlider, TBM_GETCHANNELRECT, 0, (LPARAM)&rcChannel);
                
                int thumbW = Scale(10);
                int xStart = rcChannel.left + thumbW / 2;
                int xEnd = rcChannel.right - thumbW / 2;
                
                int tick1_Y1 = rcChannel.bottom + Scale(3);
                int tick1_Y2 = tick1_Y1 + Scale(3);
                
                int tick2_Y2 = rcChannel.top - Scale(3);
                int tick2_Y1 = tick2_Y2 - Scale(3);"""

    content = content.replace(old_prepaint, new_prepaint)

    old_channel = """                    int dpiY = g_dpi;
                    int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;
                    rc.top += (rc.bottom - rc.top) / 2 - 2*s;
                    rc.bottom = rc.top + 4*s;"""

    new_channel = """                    rc.top += (rc.bottom - rc.top) / 2 - Scale(2);
                    rc.bottom = rc.top + Scale(4);"""

    content = content.replace(old_channel, new_channel)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched trackbar")

if __name__ == '__main__':
    patch()
