import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_code = r'''                entry.pLayout->HitTestPoint(x, y - currentY, &isTrailingHit, &isInside, &metrics);
                outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
                return true;'''

new_code = r'''                entry.pLayout->HitTestPoint(x, y - currentY, &isTrailingHit, &isInside, &metrics);
                outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
                std::wstring displayUser = entry.data.user;
                size_t hashPos = displayUser.find(L'#');
                if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);
                int actualLen = (int)displayUser.length() + (int)entry.data.msg.length() + (entry.data.isSystemEvent ? 0 : 2);
                if (actualLen == 0) actualLen = 1;
                if (outCharIndex > actualLen) outCharIndex = actualLen;
                return true;'''

content = content.replace(old_code, new_code)

old_code2 = r'''            entry.pLayout->HitTestPoint(9999.0f, 9999.0f, &isTrailingHit, &isInside, &metrics);
            outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
            return true;'''

new_code2 = r'''            entry.pLayout->HitTestPoint(9999.0f, 9999.0f, &isTrailingHit, &isInside, &metrics);
            outCharIndex = metrics.textPosition + (isTrailingHit ? metrics.length : 0);
            std::wstring displayUser = entry.data.user;
            size_t hashPos = displayUser.find(L'#');
            if (hashPos != std::wstring::npos) displayUser = displayUser.substr(0, hashPos);
            int actualLen = (int)displayUser.length() + (int)entry.data.msg.length() + (entry.data.isSystemEvent ? 0 : 2);
            if (actualLen == 0) actualLen = 1;
            if (outCharIndex > actualLen) outCharIndex = actualLen;
            return true;'''

content = content.replace(old_code2, new_code2)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patched GetTextPositionFromPoint successfully")
