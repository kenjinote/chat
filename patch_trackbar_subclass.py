import os

file_path = 'Source.cpp'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add VolumeSliderSubclassProc before MainWndProc
proc_code = """LRESULT CALLBACK VolumeSliderSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT clientRc;
        GetClientRect(hWnd, &clientRc);
        
        // Background
        HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
        FillRect(hdc, &clientRc, hBg);
        DeleteObject(hBg);
        
        int cy = (clientRc.top + clientRc.bottom) / 2;
        int thumbW = Scale(10);
        int thumbH = Scale(16);
        
        // Channel range
        int xStart = clientRc.left + thumbW / 2 + Scale(2);
        int xEnd = clientRc.right - thumbW / 2 - Scale(2);
        
        // Draw Ticks
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        int tick1_Y1 = cy + Scale(4);
        int tick1_Y2 = tick1_Y1 + Scale(3);
        int tick2_Y2 = cy - Scale(4);
        int tick2_Y1 = tick2_Y2 - Scale(3);
        
        for (int i = 0; i <= 10; i++) {
            int x = xStart + (xEnd - xStart) * i / 10;
            MoveToEx(hdc, x, tick1_Y1, NULL);
            LineTo(hdc, x, tick1_Y2);
            MoveToEx(hdc, x, tick2_Y1, NULL);
            LineTo(hdc, x, tick2_Y2);
        }
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw Channel
        RECT rcChannel = { xStart, cy - Scale(2), xEnd, cy + Scale(2) };
        DrawScaledButtonBorder(hdc, rcChannel, EDGE_SUNKEN);
        
        // Calculate Thumb X
        int pos = (int)SendMessage(hWnd, TBM_GETPOS, 0, 0);
        int minPos = (int)SendMessage(hWnd, TBM_GETRANGEMIN, 0, 0);
        int maxPos = (int)SendMessage(hWnd, TBM_GETRANGEMAX, 0, 0);
        if (maxPos <= minPos) maxPos = minPos + 1; // Prevent div by 0
        
        int cx = xStart + (xEnd - xStart) * (pos - minPos) / (maxPos - minPos);
        
        // Draw Thumb
        RECT rcThumb = { cx - thumbW / 2, cy - thumbH / 2, cx + thumbW / 2, cy + thumbH / 2 };
        hBg = CreateSolidBrush(RGB(192, 192, 192));
        FillRect(hdc, &rcThumb, hBg);
        DeleteObject(hBg);
        DrawScaledButtonBorder(hdc, rcThumb, EDGE_RAISED);
        
        EndPaint(hWnd, &ps);
        return 0; // Handled
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
"""
if "VolumeSliderSubclassProc" not in content:
    target_proc = "LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {"
    content = content.replace(target_proc, proc_code + "\n" + target_proc)

# 2. Add subclassing in WM_CREATE
target_create = """        SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);
        SendMessage(g_hVolumeSlider, TBM_SETTICFREQ, 10, 0);
        SetWindowTheme(g_hVolumeSlider, L" ", L" ");"""
replacement_create = target_create + """\n        SetWindowSubclass(g_hVolumeSlider, VolumeSliderSubclassProc, 0, 0);"""
if "VolumeSliderSubclassProc, 0, 0);" not in content:
    content = content.replace(target_create, replacement_create)

# 3. Remove NM_CUSTOMDRAW for g_hVolumeSlider
import re
pattern = r"        if \(nmhdr->hwndFrom == g_hVolumeSlider && nmhdr->code == NM_CUSTOMDRAW\) \{.*?\n        \}\n        break;"
content = re.sub(pattern, "        break;", content, flags=re.DOTALL)

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Subclass fix applied")
