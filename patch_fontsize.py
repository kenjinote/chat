import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

recv_old = r'''                                pData->size = json["sz"].is_number() ? json["sz"].int_value() : 10;
                                pData->color = json["clr"].is_number() ? json["clr"].int_value() : RGB(0,0,0);'''

recv_new = r'''                                pData->size = json["sz"].is_number() ? json["sz"].int_value() : 10;
                                if (pData->size > 32) pData->size = 32;
                                if (pData->size < 6) pData->size = 6;
                                pData->color = json["clr"].is_number() ? json["clr"].int_value() : RGB(0,0,0);'''

if recv_old in content:
    content = content.replace(recv_old, recv_new)
    print("Replaced recv sz")
else:
    print("Could not find recv sz")
    
send_old = r'''        size = _wtoi(szText);
    }
    
    json11::Json json = json11::Json::object {'''

send_new = r'''        size = _wtoi(szText);
    }
    if (size > 32) size = 32;
    if (size < 6) size = 6;
    
    json11::Json json = json11::Json::object {'''

if send_old in content:
    content = content.replace(send_old, send_new)
    print("Replaced send sz")
else:
    print("Could not find send sz")

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
