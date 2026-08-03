import re
with open('build.bat', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('version.lib', 'version.lib dwmapi.lib')

with open('build.bat', 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated build.bat")
