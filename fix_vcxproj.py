import re

# 1. Modify chat.vcxproj
with open('chat.vcxproj', 'r', encoding='utf-8') as f:
    proj = f.read()

proj = proj.replace('<ClCompile Include="Source.cpp" />', '<ClCompile Include="Source.cpp" />\n    <ClCompile Include="H264Helper.cpp" />')
proj = proj.replace('<ClInclude Include="json11.hpp" />', '<ClInclude Include="json11.hpp" />\n    <ClInclude Include="H264Helper.h" />')

with open('chat.vcxproj', 'w', encoding='utf-8') as f:
    f.write(proj)

# 2. Add #pragma comment to H264Helper.h
with open('H264Helper.h', 'r', encoding='utf-8') as f:
    h264h = f.read()

pragmas = """#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "Mfreadwrite.lib")

"""

if "mfplat.lib" not in h264h:
    h264h = h264h.replace('#pragma once\n', '#pragma once\n' + pragmas)
    with open('H264Helper.h', 'w', encoding='utf-8') as f:
        f.write(h264h)

print("Fixed vcxproj and added pragma comments")
