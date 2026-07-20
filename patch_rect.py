import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # We want to replace AdjustWindowRectEx(arg1, arg2, arg3, arg4);
    # with AdjustWindowRectExForDpi(arg1, arg2, arg3, arg4, g_dpi);
    # Since the arguments might contain function calls or bitwise ORs (like WS_POPUP | WS_CAPTION),
    # we can just use a simple regex that matches up to the closing parenthesis.
    
    # Let's find all instances of AdjustWindowRectEx
    # We'll split the file by AdjustWindowRectEx and reconstruct it.
    parts = content.split('AdjustWindowRectEx(')
    new_content = parts[0]
    for part in parts[1:]:
        # Find the matching closing parenthesis
        paren_count = 1
        i = 0
        while i < len(part) and paren_count > 0:
            if part[i] == '(':
                paren_count += 1
            elif part[i] == ')':
                paren_count -= 1
            i += 1
        
        # Replace the closing parenthesis with `, g_dpi)`
        if i > 0 and part[i-1] == ')':
            new_content += 'AdjustWindowRectExForDpi(' + part[:i-1] + ', g_dpi)' + part[i:]
        else:
            new_content += 'AdjustWindowRectEx(' + part

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(new_content)
    print("Patched AdjustWindowRectEx")

if __name__ == '__main__':
    patch()
