#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dwmapi.lib")
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
int main() {
    WNDCLASS wc = { 0, WndProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), NULL, L"TestClass" };
    RegisterClass(&wc);
    HWND hWnd = CreateWindow(L"TestClass", L"Test DWM", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 400, 300, NULL, NULL, GetModuleHandle(NULL), NULL);
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    MessageBoxA(NULL, "Check if animations work when maximizing this window!", "Test", MB_OK);
    return 0;
}
