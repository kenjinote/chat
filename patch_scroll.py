import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update SortUserList
sort_old = r'''    SendMessage(g_hUserList, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
    for (const auto& u : users) {
        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)u.c_str());
    }
    SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hUserList, NULL, TRUE);
}'''

sort_new = r'''    int topIndex = (int)SendMessage(g_hUserList, LB_GETTOPINDEX, 0, 0);
    int curSel = (int)SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
    std::wstring selString = L"";
    if (curSel != LB_ERR) {
        int len = (int)SendMessage(g_hUserList, LB_GETTEXTLEN, curSel, 0);
        if (len > 0) {
            std::vector<WCHAR> buf(len + 1, 0);
            SendMessage(g_hUserList, LB_GETTEXT, curSel, (LPARAM)buf.data());
            selString = buf.data();
        }
    }

    SendMessage(g_hUserList, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
    for (const auto& u : users) {
        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)u.c_str());
    }
    
    if (curSel != LB_ERR && !selString.empty()) {
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

content = content.replace(sort_old, sort_new)

# 2. Update type == "users" block
users_old = r'''                            else if (type == "users") {
                                SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
                                auto arr = json["users"].array_items();'''

users_new = r'''                            else if (type == "users") {
                                int topIndex = (int)SendMessage(g_hUserList, LB_GETTOPINDEX, 0, 0);
                                int curSel = (int)SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
                                std::wstring selString = L"";
                                if (curSel != LB_ERR) {
                                    int len = (int)SendMessage(g_hUserList, LB_GETTEXTLEN, curSel, 0);
                                    if (len > 0) {
                                        std::vector<WCHAR> buf(len + 1, 0);
                                        SendMessage(g_hUserList, LB_GETTEXT, curSel, (LPARAM)buf.data());
                                        selString = buf.data();
                                    }
                                }
                                SendMessage(g_hUserList, WM_SETREDRAW, FALSE, 0);
                                SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
                                auto arr = json["users"].array_items();'''

content = content.replace(users_old, users_new)

# 3. Update the end of type == "users" block
# We need to find where type == "users" ends. 
# It ends around UpdateSendBtnState() and some logic. Let's find SortUserList() call inside it.
users_end_old = r'''                                SortUserList();
                                
                                bool wasJoined = g_isRoomJoined;'''

users_end_new = r'''                                SortUserList();
                                
                                if (curSel != LB_ERR && !selString.empty()) {
                                    int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
                                    if (newIdx != LB_ERR) SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
                                }
                                if (topIndex != LB_ERR) SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
                                SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
                                InvalidateRect(g_hUserList, NULL, FALSE);
                                
                                bool wasJoined = g_isRoomJoined;'''

content = content.replace(users_end_old, users_end_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated Source.cpp")
