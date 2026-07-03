import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Extract from #include <mmsystem.h> to the end of void DecodeJPEG()
start_marker = "#include <wincodec.h>"
end_marker = "InvalidateRect(g_hVideoView, NULL, FALSE);\n}"

start_idx = content.find(start_marker)
end_idx = content.find(end_marker) + len(end_marker)

extracted_code = content[start_idx:end_idx]
content = content[:start_idx] + content[end_idx:]

# Find where to insert (after IDWriteFactory* g_pDWriteFactory = nullptr;)
insert_marker = "IDWriteFactory* g_pDWriteFactory = nullptr;"
insert_idx = content.find(insert_marker) + len(insert_marker)

content = content[:insert_idx] + "\n\n" + extracted_code + "\n" + content[insert_idx:]

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Order fixed")
