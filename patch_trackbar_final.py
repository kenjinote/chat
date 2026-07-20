import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add TBM_SETTHUMBLENGTH to WM_SIZE
    old_size = """        EndDeferWindowPos(hdwp);

        return 0;
    }"""
    new_size = """        EndDeferWindowPos(hdwp);

        SendMessage(g_hVolumeSlider, TBM_SETTHUMBLENGTH, (WPARAM)Scale(16), 0);

        return 0;
    }"""
    content = content.replace(old_size, new_size)

    # 2. Fix thumb width in TBCD_THUMB
    old_thumb = """                if (pcd->dwItemSpec == TBCD_THUMB) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &rc, hBg);
                    DeleteObject(hBg);
                    DrawScaledButtonBorder(hdc, rc, EDGE_RAISED);
                    return CDRF_SKIPDEFAULT;
                }"""
    new_thumb = """                if (pcd->dwItemSpec == TBCD_THUMB) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    int cx = (rc.left + rc.right) / 2;
                    int w = Scale(10);
                    rc.left = cx - w / 2;
                    rc.right = rc.left + w;
                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &rc, hBg);
                    DeleteObject(hBg);
                    DrawScaledButtonBorder(hdc, rc, EDGE_RAISED);
                    return CDRF_SKIPDEFAULT;
                }"""
    content = content.replace(old_thumb, new_thumb)

    # 3. Add InvalidateRect in WM_HSCROLL
    old_hscroll = """    case WM_HSCROLL: {
        if ((HWND)lParam == g_hVolumeSlider) {
            if (g_pSimpleAudioVolume) {"""
    new_hscroll = """    case WM_HSCROLL: {
        if ((HWND)lParam == g_hVolumeSlider) {
            InvalidateRect(g_hVolumeSlider, NULL, FALSE);
            if (g_pSimpleAudioVolume) {"""
    content = content.replace(old_hscroll, new_hscroll)

    # 4. Add InvalidateRect in WM_APP_VOLUME_CHANGED
    old_vol_changed = """            if (currentPos != displayPos) {
                SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, displayPos);
            }"""
    new_vol_changed = """            if (currentPos != displayPos) {
                SendMessage(g_hVolumeSlider, TBM_SETPOS, TRUE, displayPos);
                InvalidateRect(g_hVolumeSlider, NULL, FALSE);
            }"""
    content = content.replace(old_vol_changed, new_vol_changed)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched trackbar size logic")

if __name__ == '__main__':
    patch()
