with open('Source.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Line 1516 is "    return (int)msg.wParam;\n"
# Line 1517 is "}#define _WINSOCK_DEPRECATED_NO_WARNINGS"
# We just want to keep everything up to the "}"
for i in range(len(lines)):
    if "}#define" in lines[i]:
        idx = lines[i].find("}#define")
        lines[i] = lines[i][:idx+1] + "\n"
        lines = lines[:i+1]
        break

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)
print("Truncated Source.cpp perfectly")
