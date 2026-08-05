import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove ApplyItalicWithoutClipping definition
start_idx = content.find('void ApplyItalicWithoutClipping')
if start_idx != -1:
    end_idx = content.find('std::string GetAppVersion()')
    content = content[:start_idx] + content[end_idx:]

paint_old = r'''                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                            if (entry.data.i) ApplyItalicWithoutClipping(entry.pLayout, fullText, msgStart, (UINT32)entry.data.msg.length());
                            if (entry.data.u) entry.pLayout->SetUnderline(TRUE, range);'''
paint_new = r'''                            if (entry.data.b) entry.pLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                            if (entry.data.i) entry.pLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, range);
                            if (entry.data.u) entry.pLayout->SetUnderline(TRUE, range);'''
content = content.replace(paint_old, paint_new)

# Add trailing space to fullText to avoid right-side clipping due to italic overhang
append_old = r'''                        if (fullText.empty()) fullText = L" ";
                        
                        HRESULT hr = g_pDWriteFactory->CreateTextLayout('''
append_new = r'''                        if (fullText.empty()) fullText = L" ";
                        if (entry.data.i) fullText += L"  "; // Add spaces to accommodate italic overhang
                        
                        HRESULT hr = g_pDWriteFactory->CreateTextLayout('''
content = content.replace(append_old, append_new)

input_old = r'''        DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        // Always create normal style font format, apply italics manually to text layout to avoid emoji clipping
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        g_pDWriteFactory->CreateTextFormat(
            g_systemFontName.c_str(), NULL, weight, style, DWRITE_FONT_STRETCH_NORMAL,
            (float)tsize * 1.5f, g_systemLocaleName.c_str(), &g_pInputTextFormat);'''
input_new = r'''        DWRITE_FONT_WEIGHT weight = b ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
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
            
        if (pLayout && i) {
            ApplyItalicWithoutClipping(pLayout, safeText, 0, (UINT32)safeText.length());
        }
    }'''
input_new_2 = r'''    IDWriteTextLayout* pLayout = nullptr;
    if (g_pInputTextFormat) {
        std::wstring safeText = g_inputText.empty() ? L" " : g_inputText;
        if (i) safeText += L"  "; // Add spaces to accommodate italic overhang
        g_pDWriteFactory->CreateTextLayout(
            safeText.c_str(), (UINT32)safeText.length(), g_pInputTextFormat, width, height, &pLayout);
    }'''
content = content.replace(input_old_2, input_new_2)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patched successfully v4")
