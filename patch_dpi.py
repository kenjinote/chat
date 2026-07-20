import re

def patch_source():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add SetProcessDpiAwarenessContext to WinMain
    winmain_pattern = r'(int WINAPI wWinMain\(HINSTANCE hInstance, HINSTANCE, LPWSTR, int\) \{\n\s*timeBeginPeriod\(1\);)'
    winmain_repl = r'\1\n    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);'
    content = re.sub(winmain_pattern, winmain_repl, content)

    # 2. Add WM_DPICHANGED to WndProc
    # We'll put it right before WM_DESTROY
    wm_destroy_pattern = r'(\s*case WM_DESTROY:\n\s*\{\n\s*PostQuitMessage\(0\);)'
    wm_dpichanged_code = r'''
    case WM_DPICHANGED:
    {
        g_dpi = HIWORD(wParam);
        RECT* const prcNewWindow = (RECT*)lParam;
        SetWindowPos(hWnd,
                     NULL,
                     prcNewWindow->left,
                     prcNewWindow->top,
                     prcNewWindow->right - prcNewWindow->left,
                     prcNewWindow->bottom - prcNewWindow->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
                     
        if (pRT) {
            pRT->SetDpi((float)g_dpi, (float)g_dpi);
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }
\1'''
    content = re.sub(wm_destroy_pattern, wm_dpichanged_code, content)
    
    # 3. Replace GetDeviceCaps(hdc, LOGPIXELSY) and LOGPIXELSX with GetDpiForWindow(hWnd)
    # Wait, some places use `hdc` but they might not have `hWnd` in scope.
    # In WM_PAINT, `hWnd` is in scope (it's the WndProc parameter).
    # What about other functions? Let's check if `hWnd` is in scope where GetDeviceCaps is used.
    # If `hWnd` is not available, we can use `g_dpi` which is now dynamically updated.
    
    # Actually, if we just use `g_dpi` everywhere instead of querying GetDeviceCaps, it's globally updated by WM_DPICHANGED!
    # Let's replace `GetDeviceCaps(hdc, LOGPIXELSY)` with `g_dpi`.
    # And `GetDeviceCaps(hdc, LOGPIXELSX)` with `g_dpi`.
    content = re.sub(r'GetDeviceCaps\([^,]+,\s*LOGPIXELSY\)', 'g_dpi', content)
    content = re.sub(r'GetDeviceCaps\([^,]+,\s*LOGPIXELSX\)', 'g_dpi', content)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched Source.cpp")

if __name__ == '__main__':
    patch_source()
