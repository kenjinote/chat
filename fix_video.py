import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Extract EncodeJPEG and WebcamThread
match = re.search(r'bool EncodeJPEG\(.*?return 0;\n}', content, re.DOTALL)
if match:
    functions = match.group(0)
    # Remove from original position
    content = content.replace(functions, "")
    
    # Insert after MediaHeader
    mh_end = content.find("};\n#pragma pack(pop)")
    if mh_end != -1:
        insert_pos = mh_end + len("};\n#pragma pack(pop)")
        
        # fix wcsncpy_s
        functions = functions.replace('wcsncpy_s(mhdr->username, g_username.c_str(), 31);', 'wcsncpy_s(mhdr->username, 32, g_username.c_str(), _TRUNCATE);')
        
        content = content[:insert_pos] + "\n\n" + functions + "\n" + content[insert_pos:]
    
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Fixed order and wcsncpy_s")
else:
    print("Could not find WebcamThread")
