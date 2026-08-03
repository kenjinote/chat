import re
with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add DWMNCRENDERINGPOLICY in ApplyClassicStyle
classic_old = r'''void ApplyClassicStyle(HWND hWnd) {
    SetWindowTheme(hWnd, L" ", L" ");
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);'''

classic_new = r'''void ApplyClassicStyle(HWND hWnd) {
    SetWindowTheme(hWnd, L" ", L" ");
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);'''

content = content.replace(classic_old, classic_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated ApplyClassicStyle")
