import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    old_slider_logic = "if (g_hVolumeSlider) SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);"
    new_slider_logic = """if (g_hVolumeSlider) {
        int pos = (int)SendMessage(g_hVolumeSlider, TBM_GETPOS, 0, 0);
        DestroyWindow(g_hVolumeSlider);
        g_hVolumeSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS | TBS_FIXEDLENGTH,
            0, 0, 0, 0, hWnd, (HMENU)1052, GetModuleHandle(0), 0);
        SendMessage(g_hVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, pos);
        SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);
        SendMessage(g_hVolumeSlider, TBM_SETTICFREQ, 10, 0);
        SetWindowTheme(g_hVolumeSlider, L" ", L" ");
    }"""
    
    content = content.replace(old_slider_logic, new_slider_logic)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched UpdateFonts to recreate trackbar")

if __name__ == '__main__':
    patch()
