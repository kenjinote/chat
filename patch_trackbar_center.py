import os

file_path = 'Source.cpp'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

target = """                int tick1_Y1 = rcChannel.bottom + Scale(3);
                int tick1_Y2 = tick1_Y1 + Scale(3);
                
                int tick2_Y2 = rcChannel.top - Scale(3);
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
                RestoreDC(hdc, savedDC);
                
                return CDRF_NOTIFYITEMDRAW;
            } else if (pcd->dwDrawStage == CDDS_ITEMPREPAINT) {
                if (pcd->dwItemSpec == TBCD_THUMB) {
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
                } else if (pcd->dwItemSpec == TBCD_CHANNEL) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    rc.top += (rc.bottom - rc.top) / 2 - Scale(2);
                    rc.bottom = rc.top + Scale(4);
                    DrawScaledButtonBorder(hdc, rc, EDGE_SUNKEN);
                    return CDRF_SKIPDEFAULT;
                } else if (pcd->dwItemSpec == TBCD_TICS) {
                    return CDRF_SKIPDEFAULT;
                }
            }"""

replacement = """                RECT clientRc;
                GetClientRect(nmhdr->hwndFrom, &clientRc);
                int cy = (clientRc.top + clientRc.bottom) / 2;
                
                int tick1_Y1 = cy + Scale(2) + Scale(2);
                int tick1_Y2 = tick1_Y1 + Scale(3);
                
                int tick2_Y2 = cy - Scale(2) - Scale(2);
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
                RestoreDC(hdc, savedDC);
                
                return CDRF_NOTIFYITEMDRAW;
            } else if (pcd->dwDrawStage == CDDS_ITEMPREPAINT) {
                if (pcd->dwItemSpec == TBCD_THUMB) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    RECT clientRc;
                    GetClientRect(nmhdr->hwndFrom, &clientRc);
                    int cy = (clientRc.top + clientRc.bottom) / 2;
                    int cx = (rc.left + rc.right) / 2;
                    int w = Scale(10);
                    int h = Scale(16);
                    rc.left = cx - w / 2;
                    rc.right = rc.left + w;
                    rc.top = cy - h / 2;
                    rc.bottom = cy + h / 2;
                    HBRUSH hBg = CreateSolidBrush(RGB(192, 192, 192));
                    FillRect(hdc, &rc, hBg);
                    DeleteObject(hBg);
                    DrawScaledButtonBorder(hdc, rc, EDGE_RAISED);
                    return CDRF_SKIPDEFAULT;
                } else if (pcd->dwItemSpec == TBCD_CHANNEL) {
                    HDC hdc = pcd->hdc;
                    RECT rc = pcd->rc;
                    RECT clientRc;
                    GetClientRect(nmhdr->hwndFrom, &clientRc);
                    int cy = (clientRc.top + clientRc.bottom) / 2;
                    rc.top = cy - Scale(2);
                    rc.bottom = cy + Scale(2);
                    DrawScaledButtonBorder(hdc, rc, EDGE_SUNKEN);
                    return CDRF_SKIPDEFAULT;
                } else if (pcd->dwItemSpec == TBCD_TICS) {
                    return CDRF_SKIPDEFAULT;
                }
            }"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Center fix applied")
else:
    print("Target not found")
