import re

def shift_path(path_data, dx, dy):
    # This is a bit tricky for all SVG commands, but we only have: M, L, A, h, v, z
    # Since our commands are simple, let's just parse tokens.
    tokens = re.findall(r'[A-Za-z]+|-?\d+(?:\.\d+)?', path_data.replace(',', ' '))
    new_tokens = []
    i = 0
    cmd = ''
    while i < len(tokens):
        t = tokens[i]
        if t.isalpha():
            cmd = t
            new_tokens.append(cmd)
            i += 1
        else:
            if cmd == 'M' or cmd == 'L':
                x = float(tokens[i])
                y = float(tokens[i+1])
                new_tokens.append(f"{x+dx:g},{y+dy:g}")
                i += 2
            elif cmd == 'A':
                rx = float(tokens[i])
                ry = float(tokens[i+1])
                rot = float(tokens[i+2])
                large = tokens[i+3]
                sweep = tokens[i+4]
                x = float(tokens[i+5])
                y = float(tokens[i+6])
                new_tokens.append(f"{rx:g},{ry:g} {rot:g} {large},{sweep} {x+dx:g},{y+dy:g}")
                i += 7
            elif cmd == 'h':
                # horizontal relative, no shift needed
                new_tokens.append(tokens[i])
                i += 1
            elif cmd == 'v':
                # vertical relative, no shift needed
                new_tokens.append(tokens[i])
                i += 1
            else:
                new_tokens.append(tokens[i])
                i += 1
    return ' '.join(new_tokens)

for filename in ['app/src/main/res/drawable/ic_smile_win.xml', 'app/src/main/res/drawable/ic_tv_win.xml']:
    with open(filename, 'r') as f:
        content = f.read()
    
    # replace viewport
    content = re.sub(r'android:viewportWidth="32"', 'android:viewportWidth="21"', content)
    content = re.sub(r'android:viewportHeight="32"', 'android:viewportHeight="29"', content)
    
    # replace paths
    def replace_path(match):
        return 'android:pathData="' + shift_path(match.group(1), -4, 0) + '"'
    
    content = re.sub(r'android:pathData="(.*?)"', replace_path, content)
    
    with open(filename, 'w') as f:
        f.write(content)

print("Shifted paths and cropped viewport to remove internal padding.")
