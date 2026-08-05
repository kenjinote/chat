import re

with open('chat_server/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

chat_old = r'''        } else if (type == "chat") {
            if (m_room) {
                json11::Json::object chatObj = json.object_items();
                chatObj["userId"] = m_userId;
                chatObj["userName"] = m_userName;
                chatObj["nclr"] = m_nclr;
                m_room->deliver(json11::Json(chatObj).dump() + "\n", shared_from_this());
            }'''

chat_new = r'''        } else if (type == "chat") {
            if (m_room) {
                json11::Json::object chatObj = json.object_items();
                chatObj["userId"] = m_userId;
                chatObj["userName"] = m_userName;
                chatObj["nclr"] = m_nclr;
                
                if (chatObj["sz"].is_number()) {
                    int sz = chatObj["sz"].int_value();
                    if (sz > 32) sz = 32;
                    if (sz < 6) sz = 6;
                    chatObj["sz"] = sz;
                }
                
                m_room->deliver(json11::Json(chatObj).dump() + "\n", shared_from_this());
            }'''

if chat_old in content:
    content = content.replace(chat_old, chat_new)
    print("Patched chat_server/main.cpp")
else:
    print("Could not find chat_old in chat_server/main.cpp")

with open('chat_server/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
