import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Insert helper function before UpdateInputLayout
helper_code = r'''
void ApplyItalicWithoutClipping(IDWriteTextLayout* pLayout, const std::wstring& text, UINT32 startPos, UINT32 length) {
    UINT32 runStart = startPos;
    UINT32 runLen = 0;
    for (UINT32 i = startPos; i < startPos + length; ++i) {
        wchar_t c = text[i];
        bool isEmoji = (c >= 0xD800 && c <= 0xDFFF) || (c >= 0x2600 && c <= 0x27BF);
        if (isEmoji) {
            if (runLen > 0) {
                DWRITE_TEXT_RANGE r = { runStart, runLen };
                pLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, r);
            }
            runStart = i + 1;
            runLen = 0;
        } else {
            runLen++;
        }
    }
    if (runLen > 0) {
        DWRITE_TEXT_RANGE r = { runStart, runLen };
        pLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, r);
    }
}

void UpdateInputLayout() {'''

content = content.replace("void UpdateInputLayout() {", helper_code)

# 2. Patch WM_PAINT
paint_old = r'''                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                            if (entry.data.i) entry.pLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, range);
                            if (entry.data.u) entry.pLayout->SetUnderline(TRUE, range);'''
paint_new = r'''                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                            if (entry.data.i) ApplyItalicWithoutClipping(entry.pLayout, fullText, msgStart, (UINT32)entry.data.msg.length());
                            if (entry.data.u) entry.pLayout->SetUnderline(TRUE, range);'''
content = content.replace(paint_old, paint_new)

# 3. Patch UpdateInputLayout
input_old = r'''        DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = i ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
        g_pDWriteFactory->CreateTextFormat(
            g_systemFontName.c_str(), NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
            (float)tsize * 1.5f, g_systemLocaleName.c_str(), &g_pInputTextFormat);'''
input_new = r'''        DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        // Always create normal style font format, apply italics manually to text layout to avoid emoji clipping
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        g_pDWriteFactory->CreateTextFormat(
            g_systemFontName.c_str(), NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
            (float)tsize * 1.5f, g_systemLocaleName.c_str(), &g_pInputTextFormat);'''
content = content.replace(input_old, input_new)

input_old_2 = r'''    IDWriteTextLayout* pLayout = nullptr;
    if (g_pInputTextFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), g_pInputTextFormat, width, height, &pLayout);
    }
    
    if (g_pInputTextLayout) { g_pInputTextLayout->Release(); g_pInputTextLayout = nullptr; }
    g_pInputTextLayout = pLayout;
    
    if (g_pInputTextLayout && u) {'''
input_new_2 = r'''    IDWriteTextLayout* pLayout = nullptr;
    if (g_pInputTextFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), g_pInputTextFormat, width, height, &pLayout);
            
        if (pLayout && i) {
            ApplyItalicWithoutClipping(pLayout, safeText, 0, (UINT32)safeText.length());
        }
    }
    
    if (g_pInputTextLayout) { g_pInputTextLayout->Release(); g_pInputTextLayout = nullptr; }
    g_pInputTextLayout = pLayout;
    
    if (g_pInputTextLayout && u) {'''
content = content.replace(input_old_2, input_new_2)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patched successfully")
