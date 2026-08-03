import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update SortUserList
sort_old = r'''    if (curSel != LB_ERR && !selString.empty()) {
        int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
        if (newIdx != LB_ERR) {
            SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
        }
    }
    if (topIndex != LB_ERR) {
        SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
    }
    
    SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hUserList, NULL, FALSE);
}'''

sort_new = r'''    SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);

    if (curSel != LB_ERR && !selString.empty()) {
        int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
        if (newIdx != LB_ERR) {
            SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
        }
    }
    if (topIndex != LB_ERR) {
        SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
    }
    
    InvalidateRect(g_hUserList, NULL, FALSE);
}'''

content = content.replace(sort_old, sort_new)

# 2. Update type == "users"
users_old = r'''                                if (curSel != LB_ERR && !selString.empty()) {
                                    int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
                                    if (newIdx != LB_ERR) SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
                                }
                                if (topIndex != LB_ERR) SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
                                SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
                                InvalidateRect(g_hUserList, NULL, FALSE);'''

users_new = r'''                                SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
                                if (curSel != LB_ERR && !selString.empty()) {
                                    int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
                                    if (newIdx != LB_ERR) SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
                                }
                                if (topIndex != LB_ERR) SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
                                InvalidateRect(g_hUserList, NULL, FALSE);'''

content = content.replace(users_old, users_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated Source.cpp order")
