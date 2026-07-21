import os
import re
import codecs

LANG_MAP = {
    "LANG_ENGLISH": "en",
    "LANG_JAPANESE": "ja",
    "LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED": "zh-Hans",
    "LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL": "zh-Hant",
    "LANG_KOREAN": "ko",
    "LANG_FRENCH": "fr",
    "LANG_GERMAN": "de",
    "LANG_SPANISH": "es",
    "LANG_HINDI": "hi",
    "LANG_PORTUGUESE": "pt",
    "LANG_RUSSIAN": "ru",
    "LANG_ARABIC": "ar",
    "LANG_BENGALI": "bn",
    "LANG_INDONESIAN": "id",
    "LANG_ITALIAN": "it",
    "LANG_TURKISH": "tr",
    "LANG_VIETNAMESE": "vi"
}

def parse_rc_file(rc_path):
    # Read as utf-16
    try:
        with codecs.open(rc_path, 'r', 'utf-16') as f:
            content = f.read()
    except Exception:
        with codecs.open(rc_path, 'r', 'utf-8', errors='ignore') as f:
            content = f.read()

    blocks = re.split(r'LANGUAGE\s+', content)
    
    strings_by_lang = {}
    
    for block in blocks[1:]: 
        lines = block.split('\n')
        lang_spec = lines[0].strip()
        
        mapped_lang = None
        for key, val in LANG_MAP.items():
            if lang_spec.startswith(key):
                mapped_lang = val
                break
        
        if not mapped_lang:
            continue
            
        in_stringtable = False
        strings_by_lang[mapped_lang] = {}
        
        for line in lines[1:]:
            line = line.strip()
            if line == "STRINGTABLE":
                in_stringtable = True
                continue
            if in_stringtable and line == "BEGIN":
                continue
            if in_stringtable and line == "END":
                in_stringtable = False
                continue
            
            if in_stringtable and line.startswith("IDS_"):
                # Handle possible whitespace after IDS_XXX and capture everything in quotes
                # Some strings might not have quotes if they are just identifiers, but RC files use quotes.
                match = re.match(r'(IDS_[A-Z0-9_]+)\s+"(.*)"', line)
                if match:
                    key = match.group(1)
                    val = match.group(2)
                    strings_by_lang[mapped_lang][key] = val
                    
    return strings_by_lang

def generate_lproj_files(strings_by_lang, base_dir):
    os.makedirs(base_dir, exist_ok=True)
    
    for lang, strings in strings_by_lang.items():
        lproj_dir = os.path.join(base_dir, f"{lang}.lproj")
        os.makedirs(lproj_dir, exist_ok=True)
        
        strings_path = os.path.join(lproj_dir, "Localizable.strings")
        with codecs.open(strings_path, 'w', 'utf-8') as f:
            for k, v in strings.items():
                v = v.replace('%s', '%@')
                # In ObjC Localizable.strings, quotes must be escaped.
                v = v.replace('""', '\\"')
                f.write(f'"{k}" = "{v}";\n')

if __name__ == "__main__":
    rc_path = "windows_chat/app.rc"
    strings = parse_rc_file(rc_path)
    generate_lproj_files(strings, "Resources")
    print(f"Generated Localizable.strings for {len(strings)} languages.")
