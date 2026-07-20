import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # We want to replace `pRT->BeginDraw();` with `pRT->SetDpi((float)g_dpi, (float)g_dpi);\n            pRT->BeginDraw();`
    # But only if it's not already preceded by SetDpi. Actually, it's safer to just replace all `BeginDraw()` calls for pRT.
    content = re.sub(r'pRT->BeginDraw\(\);', r'pRT->SetDpi((float)g_dpi, (float)g_dpi);\n            pRT->BeginDraw();', content)
    
    # Also, the previous patch script I wrote accidentally left `g_dpi = g_dpi;` in wWinMain.
    # Let's fix that! We should initialize g_dpi with GetDpiForSystem() in wWinMain.
    content = re.sub(r'g_dpi = g_dpi;', r'g_dpi = GetDpiForSystem();', content)
    
    # Let's clean up the redundant pRT->SetDpi inside D2DLogProc that was inside `if (pRT) { pRT->SetDpi... }` right after creation.
    # Actually, it doesn't matter if it's called twice.
    
    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched Source.cpp again")

if __name__ == '__main__':
    patch()
