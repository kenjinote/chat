import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

code = code.replace('g_videoFrames[g_userId] = rgbOut;', 'g_videoPixelsMap[g_userId] = rgbOut;')
code = code.replace('g_videoDimensions[g_userId] = {width, height};', 'g_videoSizeMap[g_userId] = {width, height};')

code = code.replace('g_videoFrames[data->username] = rgbOut;', 'g_videoPixelsMap[data->username] = rgbOut;')
code = code.replace('g_videoDimensions[data->username] = {640, 480};', 'g_videoSizeMap[data->username] = {640, 480};\n        HWND hView = NULL;\n        auto it = g_videoWindows.find(data->username);\n        if (it != g_videoWindows.end()) hView = it->second;\n        if (hView && IsWindow(hView)) InvalidateRect(hView, NULL, FALSE);')

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Source.cpp fixed variable names")
