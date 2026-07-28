import sys

lines = open('app.rc', encoding='utf-8').read().splitlines()

version_info = """
VS_VERSION_INFO VERSIONINFO
 FILEVERSION 1,0,0,0
 PRODUCTVERSION 1,0,0,0
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0x0L
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "FileVersion", "1.0.0.0"
            VALUE "ProductVersion", "1.0.0.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
"""

if 'VS_VERSION_INFO VERSIONINFO' not in '\n'.join(lines):
    lines.append(version_info)
    with open('app.rc', 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
