import re

for filename in ['app/src/main/res/drawable/ic_smile_win.xml', 'app/src/main/res/drawable/ic_tv_win.xml']:
    with open(filename, 'r') as f:
        content = f.read()
    
    content = re.sub(r'android:width="32dp"', 'android:width="21dp"', content)
    content = re.sub(r'android:height="32dp"', 'android:height="29dp"', content)
    
    with open(filename, 'w') as f:
        f.write(content)

print("Fixed VectorDrawable intrinsic sizes.")
