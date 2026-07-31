import io
import sys

with io.open("Source.cpp", "r", encoding="utf-8-sig") as f:
    text = f.read()

start_idx = text.find("LRESULT CALLBACK ScrollbarColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {")
if start_idx == -1:
    print("Cannot find ScrollbarColorProc")
    sys.exit(1)

end_idx = text.find("\nstd::wstring g_moveBoxResult;", start_idx)
if end_idx == -1:
    print("Cannot find end of ScrollbarColorProc")
    sys.exit(1)

new_func = """LRESULT CALLBACK ScrollbarColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    auto getSbRects = [&](RECT& rcTop, RECT& rcBot, RECT& rcThumb, RECT& rcPgUp, RECT& rcPgDn) {
        RECT rcWin; GetWindowRect(hWnd, &rcWin);
        int width = rcWin.right - rcWin.left;
        int height = rcWin.bottom - rcWin.top;
        int dpiY = g_dpi;
        int bw = (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) ? max(2, MulDiv(2, dpiY, 96)) : 0;
        int cxVScroll = GetSystemMetricsForDpi(SM_CXVSCROLL, dpiY);
        int offsetX = width - bw - cxVScroll;
        int offsetY = bw;
        int sbHeight = height - bw * 2;
        int arrS = cxVScroll;
        rcTop = { offsetX, offsetY, offsetX + cxVScroll, offsetY + arrS };
        rcBot = { offsetX, offsetY + sbHeight - arrS, offsetX + cxVScroll, offsetY + sbHeight };
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        GetScrollInfo(hWnd, SB_VERT, &si);
        int scrollMax = si.nMax - max((int)si.nPage - 1, 0);
        int trackLen = sbHeight - arrS * 2;
        int thumbLen = trackLen;
        int thumbTop = offsetY + arrS;
        if (scrollMax > 0 && si.nPage > 0) {
            long long page = si.nPage;
            long long range = (long long)si.nMax - si.nMin + 1;
            thumbLen = (int)(trackLen * page / range);
            int minThumb = MulDiv(8, dpiY, 96);
            if (thumbLen < minThumb) thumbLen = minThumb;
            if (thumbLen > trackLen) thumbLen = trackLen;
            if (thumbLen < trackLen) {
                thumbTop = offsetY + arrS + (si.nPos - si.nMin) * (trackLen - thumbLen) / scrollMax;
            }
        }
        rcThumb = { offsetX, thumbTop, offsetX + cxVScroll, thumbTop + thumbLen };
        rcPgUp = { offsetX, rcTop.bottom, offsetX + cxVScroll, rcThumb.top };
        rcPgDn = { offsetX, rcThumb.bottom, offsetX + cxVScroll, rcBot.top };
    };

    if (msg == WM_NCPAINT) {
        if ((GetWindowLong(hWnd, GWL_STYLE) & WS_VSCROLL) == 0) {
            return DefSubclassProc(hWnd, msg, wParam, lParam);
        }

        SCROLLBARINFO sbiClip = { sizeof(sbiClip) };
        if (!GetScrollBarInfo(hWnd, OBJID_VSCROLL, &sbiClip) || (sbiClip.rgstate[0] & STATE_SYSTEM_INVISIBLE)) {
            return DefSubclassProc(hWnd, msg, wParam, lParam);
        }

        HRGN hUpdateRgn = (HRGN)wParam;
        HRGN hClippedRgn = NULL;
        if (hUpdateRgn == (HRGN)1 || hUpdateRgn == NULL) {
            RECT rcWinClip; GetWindowRect(hWnd, &rcWinClip);
            hClippedRgn = CreateRectRgnIndirect(&rcWinClip);
        } else {
            hClippedRgn = CreateRectRgn(0,0,0,0);
            CombineRgn(hClippedRgn, hUpdateRgn, NULL, RGN_COPY);
        }
        HRGN hExclude = CreateRectRgnIndirect(&sbiClip.rcScrollBar);
        CombineRgn(hClippedRgn, hClippedRgn, hExclude, RGN_DIFF);
        DeleteObject(hExclude);

        LRESULT res = DefSubclassProc(hWnd, msg, (WPARAM)hClippedRgn, lParam);
        DeleteObject(hClippedRgn);
        
        HDC hdc = GetWindowDC(hWnd);
        RECT rcTop, rcBot, rcThumb, rcPgUp, rcPgDn;
        getSbRects(rcTop, rcBot, rcThumb, rcPgUp, rcPgDn);

        HBRUSH hFace = CreateSolidBrush(RGB(192, 192, 192));
        HPEN hBlack = (HPEN)GetStockObject(BLACK_PEN);

        WORD pat[8] = { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555 };
        HBITMAP hBmp = CreateBitmap(8, 8, 1, 1, pat);
        HBRUSH hTrackBrush = CreatePatternBrush(hBmp);

        auto drawArrow = [&](RECT rc, bool isUp, bool isPressed) {
            FillRect(hdc, &rc, hFace);
            DrawScaledButtonBorder(hdc, rc, isPressed ? EDGE_SUNKEN : EDGE_RAISED);
            
            int dpiY = g_dpi;
            int s = MulDiv(1, dpiY, 96); if (s < 1) s = 1;

            int cx = rc.left + (rc.right - rc.left) / 2;
            int cy = rc.top + (rc.bottom - rc.top) / 2;
            if (isPressed) { cx += s; cy += s; }
            SelectObject(hdc, hBlack);
            HBRUSH hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBlackBrush);
            if (isUp) {
                POINT pts[3] = { {cx, cy - 2*s}, {cx - 3*s, cy + 1*s}, {cx + 3*s, cy + 1*s} };
                Polygon(hdc, pts, 3);
            } else {
                POINT pts[3] = { {cx - 3*s, cy - 1*s}, {cx + 3*s, cy - 1*s}, {cx, cy + 2*s} };
                Polygon(hdc, pts, 3);
            }
            SelectObject(hdc, hOldB);
        };

        bool isTopPressed = (g_sbPressedWnd == hWnd && g_sbPressedHit == 1);
        drawArrow(rcTop, true, isTopPressed);

        bool isBotPressed = (g_sbPressedWnd == hWnd && g_sbPressedHit == 2);
        drawArrow(rcBot, false, isBotPressed);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(192, 192, 192));

        if (rcThumb.bottom > rcThumb.top && !(sbiClip.rgstate[0] & STATE_SYSTEM_UNAVAILABLE)) {
            FillRect(hdc, &rcPgUp, hTrackBrush);
            FillRect(hdc, &rcPgDn, hTrackBrush);
            FillRect(hdc, &rcThumb, hFace);
            DrawScaledButtonBorder(hdc, rcThumb, EDGE_RAISED);
        } else {
            RECT rcTrack = { rcTop.left, rcTop.bottom, rcBot.right, rcBot.top };
            FillRect(hdc, &rcTrack, hTrackBrush);
        }

        DeleteObject(hFace);
        DeleteObject(hTrackBrush);
        DeleteObject(hBmp);
        ReleaseDC(hWnd, hdc);
        return res;
    }
    else if (msg == WM_NCMOUSEMOVE || msg == WM_NCMOUSELEAVE) {
        if (wParam == HTVSCROLL || wParam == HTHSCROLL) return 0;
    }
    else if (msg == WM_NCLBUTTONDBLCLK || msg == WM_NCLBUTTONDOWN) {
        if (wParam == HTVSCROLL) {
            SCROLLBARINFO sbiClip = { sizeof(sbiClip) };
            if (!GetScrollBarInfo(hWnd, OBJID_VSCROLL, &sbiClip) || (sbiClip.rgstate[0] & STATE_SYSTEM_INVISIBLE)) return 0;

            RECT rcTop, rcBot, rcThumb, rcPgUp, rcPgDn;
            getSbRects(rcTop, rcBot, rcThumb, rcPgUp, rcPgDn);

            POINT pt; GetCursorPos(&pt);
            RECT rcWin; GetWindowRect(hWnd, &rcWin);
            POINT localPt = { pt.x - rcWin.left, pt.y - rcWin.top };

            int hit = 0;
            if (PtInRect(&rcTop, localPt)) hit = 1;
            else if (PtInRect(&rcBot, localPt)) hit = 2;
            else if (PtInRect(&rcThumb, localPt)) hit = 3;
            else if (PtInRect(&rcPgUp, localPt)) hit = 4;
            else if (PtInRect(&rcPgDn, localPt)) hit = 5;

            if (hit != 0) {
                SetCapture(hWnd);
                int startY = pt.y;
                SCROLLINFO si = { sizeof(si), SIF_ALL };
                GetScrollInfo(hWnd, SB_VERT, &si);
                int startPos = si.nPos;

                auto sendScroll = [&](int action) { SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(action, 0), 0); };
                if (hit == 1) sendScroll(SB_LINEUP);
                if (hit == 2) sendScroll(SB_LINEDOWN);
                if (hit == 4) sendScroll(SB_PAGEUP);
                if (hit == 5) sendScroll(SB_PAGEDOWN);

                if (hit != 3) SetTimer(hWnd, 9998, 250, NULL);
                
                g_sbPressedWnd = hWnd;
                g_sbPressedHit = hit;
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);

                MSG msgLoop;
                while (GetCapture() == hWnd && GetMessage(&msgLoop, NULL, 0, 0)) {
                    if (msgLoop.message == WM_LBUTTONUP) {
                        ReleaseCapture();
                        break;
                    } else if (msgLoop.message == WM_MOUSEMOVE) {
                        if (hit == 3) {
                            POINT scrPt; GetCursorPos(&scrPt);
                            int trackLen = rcBot.top - rcTop.bottom;
                            int thumbLen = rcThumb.bottom - rcThumb.top;
                            SCROLLINFO si2 = { sizeof(si2), SIF_ALL };
                            GetScrollInfo(hWnd, SB_VERT, &si2);
                            int scrollMax = si2.nMax - max((int)si2.nPage - 1, 0);
                            if (trackLen > thumbLen && scrollMax > 0) {
                                int dy = scrPt.y - startY;
                                int newPos = startPos + dy * scrollMax / (trackLen - thumbLen);
                                if (newPos < si2.nMin) newPos = si2.nMin;
                                if (newPos > scrollMax) newPos = scrollMax;
                                SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, newPos), 0);
                                SetScrollPos(hWnd, SB_VERT, newPos, TRUE);
                                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                            }
                        } else if (hit == 1 || hit == 2) {
                            POINT scrPt; GetCursorPos(&scrPt);
                            POINT scrLocal = { scrPt.x - rcWin.left, scrPt.y - rcWin.top };
                            bool isInside = (hit == 1 && PtInRect(&rcTop, scrLocal)) || (hit == 2 && PtInRect(&rcBot, scrLocal));
                            int newHit = isInside ? hit : 0;
                            if (g_sbPressedHit != newHit) {
                                g_sbPressedHit = newHit;
                                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                            }
                        }
                    } else if (msgLoop.message == WM_TIMER && msgLoop.wParam == 9998) {
                        SetTimer(hWnd, 9998, 50, NULL);
                        POINT scrPt; GetCursorPos(&scrPt);
                        POINT scrLocal = { scrPt.x - rcWin.left, scrPt.y - rcWin.top };
                        if (hit == 1 && PtInRect(&rcTop, scrLocal)) sendScroll(SB_LINEUP);
                        if (hit == 2 && PtInRect(&rcBot, scrLocal)) sendScroll(SB_LINEDOWN);
                        if (hit == 4 && PtInRect(&rcPgUp, scrLocal)) sendScroll(SB_PAGEUP);
                        if (hit == 5 && PtInRect(&rcPgDn, scrLocal)) sendScroll(SB_PAGEDOWN);
                    } else {
                        TranslateMessage(&msgLoop);
                        DispatchMessage(&msgLoop);
                    }
                }
                g_sbPressedWnd = NULL;
                g_sbPressedHit = 0;
                if (hit != 3) KillTimer(hWnd, 9998);
                sendScroll(SB_ENDSCROLL);
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                return 0;
            }
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }
    else if (msg == WM_MOUSEWHEEL || msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_VSCROLL) {
        wchar_t className[256];
        GetClassNameW(hWnd, className, 256);
        if (wcscmp(className, L"RICHEDIT50W") == 0) {
            if (msg == WM_MOUSEWHEEL) {
                int delta = (short)HIWORD(wParam);
                int lines = (delta > 0 ? 3 : -3);
                if (lines > 0) {
                    for (int i = 0; i < lines; i++) SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
                } else {
                    for (int i = 0; i < -lines; i++) SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                }
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                return 0;
            } else if (msg == WM_KEYDOWN) {
                int action = -1;
                if (wParam == VK_UP) action = SB_LINEUP;
                else if (wParam == VK_DOWN) action = SB_LINEDOWN;
                else if (wParam == VK_PRIOR) action = SB_PAGEUP;
                else if (wParam == VK_NEXT) action = SB_PAGEDOWN;
                
                if (action != -1) {
                    SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(action, 0), 0);
                    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                    return 0;
                }
            }
        }
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        return res;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
"""

new_text = text[:start_idx] + new_func + text[end_idx:]
with io.open("Source.cpp", "w", encoding="utf-8-sig") as f:
    f.write(new_text)

print("Done")

