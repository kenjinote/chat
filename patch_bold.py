import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

helper_code = r'''
void ApplyBoldToText(IDWriteTextLayout* pLayout, const std::wstring& text, UINT32 startPos, UINT32 length, bool isItalic) {
    if (!isItalic) {
        DWRITE_TEXT_RANGE r = { startPos, length };
        pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, r);
        return;
    }

    UINT32 runStart = startPos;
    UINT32 runLen = 0;
    for (UINT32 j = startPos; j < startPos + length; ++j) {
        wchar_t c = text[j];
        bool isEmoji = (c >= 0xD800 && c <= 0xDFFF) || (c >= 0x2600 && c <= 0x27BF);
        if (isEmoji) {
            if (runLen > 0) {
                DWRITE_TEXT_RANGE r = { runStart, runLen };
                pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, r);
            }
            runStart = j + 1;
            runLen = 0;
        } else {
            runLen++;
        }
    }
    if (runLen > 0) {
        DWRITE_TEXT_RANGE r = { runStart, runLen };
        pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, r);
    }
}
'''

insert_idx = content.find('std::string GetAppVersion()')
content = content[:insert_idx] + helper_code + '\n' + content[insert_idx:]

log_old = r'''                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);'''
log_new = r'''                            if (entry.data.b) ApplyBoldToText(entry.pLayout, fullText, msgStart, (UINT32)entry.data.msg.length(), entry.data.i);'''
content = content.replace(log_old, log_new)

input_old = r'''        DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = i ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
        g_pDWriteFactory->CreateTextFormat(
            g_systemFontName.c_str(), NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
            (float)tsize * 1.5f, g_systemLocaleName.c_str(), &g_pInputTextFormat);'''
input_new = r'''        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = i ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
        g_pDWriteFactory->CreateTextFormat(
            g_systemFontName.c_str(), NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
            (float)tsize * 1.5f, g_systemLocaleName.c_str(), &g_pInputTextFormat);'''
content = content.replace(input_old, input_new)

input_old_2 = r'''    IDWriteTextLayout* pLayout = nullptr;
    if (g_pInputTextFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), g_pInputTextFormat, width, height, &pLayout);
    }'''
input_new_2 = r'''    IDWriteTextLayout* pLayout = nullptr;
    if (g_pInputTextFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), g_pInputTextFormat, width, height, &pLayout);
        if (pLayout && b) {
            ApplyBoldToText(pLayout, safeText, 0, (UINT32)safeText.length(), i);
        }
    }'''
content = content.replace(input_old_2, input_new_2)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patched bold italics")
