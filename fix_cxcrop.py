import re
with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace all cxCrop logic that has - bw3
content = re.sub(r'int cxCrop = -rcFrame\.left - bw3;', r'int cxCrop = -rcFrame.left;', content)
content = re.sub(r'int cyCropTop = -rcFrame\.top - GetSystemMetricsForDpi\(SM_CYCAPTION, (.*?)\) - bw3;', r'int cyCropTop = 0;', content)
content = re.sub(r'int cyCropBottom = rcFrame\.bottom - bw3;', r'int cyCropBottom = rcFrame.bottom;', content)

# Remove edge shifting logic in WM_WINDOWPOSCHANGING
content = re.sub(r'bool isTopEdge = \(pwp->y == mi\.rcWork\.top\);\s*bool isBottomEdge = \(pwp->y \+ pwp->cy == mi\.rcWork\.bottom\);\s*if \(isLeftEdge \|\| isRightEdge \|\| isTopEdge \|\| isBottomEdge\) \{.*?pwp->cy \+= cyCropTop \+ cyCropBottom;\s*\}', r'// Edge logic removed (fixed crops handle it natively)', content, flags=re.DOTALL)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Replaced cxCrop logic!")
