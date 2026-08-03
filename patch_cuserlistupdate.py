import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Insert CUserListUpdate class
class_def = r'''
class CUserListUpdate {
    HWND m_hwnd;
    int m_topIndex;
    int m_curSel;
    std::wstring m_selString;
    static int s_refCount;
public:
    CUserListUpdate(HWND hwnd) : m_hwnd(hwnd) {
        if (m_hwnd) {
            if (s_refCount == 0) {
                m_topIndex = (int)SendMessage(m_hwnd, LB_GETTOPINDEX, 0, 0);
                m_curSel = (int)SendMessage(m_hwnd, LB_GETCURSEL, 0, 0);
                if (m_curSel != LB_ERR) {
                    int len = (int)SendMessage(m_hwnd, LB_GETTEXTLEN, m_curSel, 0);
                    if (len > 0) {
                        std::vector<WCHAR> buf(len + 1, 0);
                        SendMessage(m_hwnd, LB_GETTEXT, m_curSel, (LPARAM)buf.data());
                        m_selString = buf.data();
                    }
                }
                SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0);
            }
            s_refCount++;
        }
    }
    ~CUserListUpdate() {
        if (m_hwnd) {
            s_refCount--;
            if (s_refCount == 0) {
                SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0);
                if (m_curSel != LB_ERR && !m_selString.empty()) {
                    int newIdx = (int)SendMessage(m_hwnd, LB_FINDSTRINGEXACT, -1, (LPARAM)m_selString.c_str());
                    if (newIdx != LB_ERR) SendMessage(m_hwnd, LB_SETCURSEL, newIdx, 0);
                }
                if (m_topIndex != LB_ERR) {
                    SendMessage(m_hwnd, LB_SETTOPINDEX, m_topIndex, 0);
                }
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
    }
};
int CUserListUpdate::s_refCount = 0;

void SortUserList() {'''
content = content.replace('void SortUserList() {', class_def)


# 2. Modify SortUserList to use CUserListUpdate and remove manual code
sort_old = r'''    int topIndex = (int)SendMessage(g_hUserList, LB_GETTOPINDEX, 0, 0);
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
    
    SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);

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

sort_new = r'''    CUserListUpdate updater(g_hUserList);
    SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);
    for (const auto& u : users) {
        SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)u.c_str());
    }
}'''
content = content.replace(sort_old, sort_new)


# 3. Modify type == "users" to use CUserListUpdate
users_old = r'''                            else if (type == "users") {
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
                                SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);'''

users_new = r'''                            else if (type == "users") {
                                CUserListUpdate updater(g_hUserList);
                                SendMessage(g_hUserList, LB_RESETCONTENT, 0, 0);'''
content = content.replace(users_old, users_new)


users_end_old = r'''                                SortUserList();
                                
                                SendMessage(g_hUserList, WM_SETREDRAW, TRUE, 0);
                                if (curSel != LB_ERR && !selString.empty()) {
                                    int newIdx = (int)SendMessage(g_hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)selString.c_str());
                                    if (newIdx != LB_ERR) SendMessage(g_hUserList, LB_SETCURSEL, newIdx, 0);
                                }
                                if (topIndex != LB_ERR) SendMessage(g_hUserList, LB_SETTOPINDEX, topIndex, 0);
                                InvalidateRect(g_hUserList, NULL, FALSE);
                                
                                bool wasJoined = g_isRoomJoined;'''

users_end_new = r'''                                SortUserList();
                                
                                bool wasJoined = g_isRoomJoined;'''
content = content.replace(users_end_old, users_end_new)


# 4. Modify type == "join"
join_old = r'''                            else if (type == "join") {
                                std::string uid = json["userId"].string_value();'''
join_new = r'''                            else if (type == "join") {
                                CUserListUpdate updater(g_hUserList);
                                std::string uid = json["userId"].string_value();'''
content = content.replace(join_old, join_new)


# 5. Modify type == "leave"
leave_old = r'''                            else if (type == "leave") {
                                std::string uid = json["userId"].string_value();'''
leave_new = r'''                            else if (type == "leave") {
                                CUserListUpdate updater(g_hUserList);
                                std::string uid = json["userId"].string_value();'''
content = content.replace(leave_old, leave_new)


with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated Source.cpp with CUserListUpdate")
