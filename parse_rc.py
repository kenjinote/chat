import os
import re

rc_path = r"C:\Users\kenjinote\Desktop\chat\app.rc"
out_dir = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\res"

id_map = {
    "IDS_LOBBY": "lobby",
    "IDS_JOINED_NETWORK": "joined_network",
    "IDS_MOVED_TO": "moved_to",
    "IDS_ROOM_ENTER_FMT": "room_enter_fmt",
    "IDS_ROOM_LEAVE_FMT": "room_leave_fmt",
    "IDS_USER_LIST_FMT": "user_list_fmt",
    "IDS_TALK": "talk",
    "IDS_PRIVATE": "private_room",
    "IDS_SEND": "send",
    "IDS_MOVE_ROOM_BTN": "move_room_btn",
    "IDS_CHANGE_NAME_TITLE": "rename_btn"
}

def fix_format(text):
    parts = re.split(r'(%[sd])', text)
    count = 1
    res = ""
    for p in parts:
        if p == '%s':
            res += f"%{count}$s"
            count += 1
        elif p == '%d':
            res += f"%{count}$d"
            count += 1
        else:
            p = p.replace("'", "\\'")
            p = p.replace("&", "&amp;")
            res += p
    return res

def parse_rc(file_path):
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    
    blocks = re.split(r'LANGUAGE\s+(LANG_\w+),\s+(SUBLANG_\w+)', content)
    
    languages = []
    for i in range(1, len(blocks)-2, 3):
        lang = blocks[i].strip()
        sublang = blocks[i+1].strip()
        text = blocks[i+2]
        
        st_match = re.search(r'STRINGTABLE\s*BEGIN(.*?)\bEND\b', text, re.DOTALL | re.IGNORECASE)
        if not st_match:
            continue
            
        st_text = st_match.group(1)
        strings = {}
        for line in st_text.splitlines():
            line = line.strip()
            if not line: continue
            
            m = re.match(r'^(IDS_\w+)\s+"(.*)"$', line)
            if m:
                key = m.group(1)
                val = m.group(2)
                if key in id_map:
                    strings[id_map[key]] = fix_format(val)
        
        if strings:
            languages.append({
                "lang": lang,
                "sublang": sublang,
                "strings": strings
            })
            
    return languages

lang_to_values = {
    "LANG_ENGLISH": "values",
    "LANG_JAPANESE": "values-ja",
    "LANG_CHINESE|SUBLANG_CHINESE_SIMPLIFIED": "values-zh-rCN",
    "LANG_CHINESE|SUBLANG_CHINESE_TRADITIONAL": "values-zh-rTW",
    "LANG_KOREAN": "values-ko",
    "LANG_FRENCH": "values-fr",
    "LANG_GERMAN": "values-de",
    "LANG_SPANISH": "values-es",
    "LANG_HINDI": "values-hi",
    "LANG_PORTUGUESE": "values-pt",
    "LANG_RUSSIAN": "values-ru",
    "LANG_ARABIC": "values-ar",
    "LANG_BENGALI": "values-bn",
    "LANG_INDONESIAN": "values-in",
    "LANG_ITALIAN": "values-it",
    "LANG_TURKISH": "values-tr",
    "LANG_VIETNAMESE": "values-vi"
}

def get_dir_name(lang, sublang):
    k = f"{lang}|{sublang}"
    if k in lang_to_values: return lang_to_values[k]
    if lang in lang_to_values: return lang_to_values[lang]
    return None

langs = parse_rc(rc_path)

android_only = {
    "app_name": "chat",
    "connect": "Connect",
    "server_ip": "Server IP:",
    "name_label": "Name:",
    "vid_on": "Vid:ON",
    "vid_off": "Vid:OFF",
    "vol": "Vol:",
    "mute": "Mute"
}
android_only_ja = {
    "connect": "接続",
    "server_ip": "サーバーIP:",
    "name_label": "名前:",
    "mute": "ミュート",
    "vid_on": "ビデオ:ON",
    "vid_off": "ビデオ:OFF",
    "vol": "音量:"
}

for l in langs:
    dname = get_dir_name(l['lang'], l['sublang'])
    if not dname:
        print("Skipping", l['lang'], l['sublang'])
        continue
        
    os.makedirs(os.path.join(out_dir, dname), exist_ok=True)
    xml_path = os.path.join(out_dir, dname, "strings.xml")
    
    xml = '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n'
    
    if dname == "values":
        for k, v in android_only.items():
            if k not in l['strings']:
                l['strings'][k] = v
    elif dname == "values-ja":
        for k, v in android_only_ja.items():
            if k not in l['strings']:
                l['strings'][k] = v
                
    for k, v in l['strings'].items():
        xml += f'    <string name="{k}">{v}</string>\n'
    xml += '</resources>\n'
    
    with open(xml_path, "w", encoding="utf-8") as f:
        f.write(xml)
    print("Wrote", xml_path, len(l['strings']), "strings")
