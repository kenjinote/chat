#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

int g_dpi = 96;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCALCSIZE && wParam == TRUE) {
        NCCALCSIZE_PARAMS* pncsp = (NCCALCSIZE_PARAMS*)lParam;
        DefWindowProc(hWnd, WM_NCCALCSIZE, wParam, lParam);
        
        RECT rcFrame = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&rcFrame, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
        int bw3 = 3;
        int captionHeight = GetSystemMetrics(SM_CYCAPTION);
        
        int cxCrop = -rcFrame.left - bw3; 
        int cyCropTop = -rcFrame.top - captionHeight - bw3; 
        int cyCropBottom = rcFrame.bottom - bw3; 
        
        pncsp->rgrc[0].left -= cxCrop;
        pncsp->rgrc[0].right += cxCrop;
        pncsp->rgrc[0].top -= cyCropTop;
        pncsp->rgrc[0].bottom += cyCropBottom;
        
        return 0;
    }
    else if (msg == WM_NCPAINT) {
        HDC hdc = GetWindowDC(hWnd);
        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
        int width = rcWindow.right - rcWindow.left;
        int height = rcWindow.bottom - rcWindow.top;
        
        RECT rcCaption = { 3, 3, width - 3, 3 + GetSystemMetrics(SM_CYCAPTION) };
        HBRUSH hBr = CreateSolidBrush(RGB(255, 0, 0)); // Red title bar
        FillRect(hdc, &rcCaption, hBr);
        DeleteObject(hBr);
        
        RECT rcLeft = { 0, 0, 3, height };
        RECT rcRight = { width - 3, 0, width, height };
        RECT rcBottom = { 0, height - 3, width, height };
        RECT rcTop = { 0, 0, width, 3 };
        
        HBRUSH hBorder = CreateSolidBrush(RGB(0, 0, 255)); // Blue borders
        FillRect(hdc, &rcLeft, hBorder);
        FillRect(hdc, &rcRight, hBorder);
        FillRect(hdc, &rcBottom, hBorder);
        FillRect(hdc, &rcTop, hBorder);
        DeleteObject(hBorder);
        
        ReleaseDC(hWnd, hdc);
        return 0;
    }
    else if (msg == WM_NCHITTEST) {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            RECT rcWindow; GetWindowRect(hWnd, &rcWindow);
            POINT wpt = { pt.x - rcWindow.left, pt.y - rcWindow.top };
            if (wpt.y < 3 + GetSystemMetrics(SM_CYCAPTION)) return HTCAPTION;
        }
        return hit;
    }
    else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "TestCalcSize";
    RegisterClass(&wc);
    
    HWND hWnd = CreateWindowEx(0, "TestCalcSize", "Test", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 400, 300, NULL, NULL, hInstance, NULL);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
