import os

file_path = 'Source.cpp'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

target = """                            int volume = (maxAmp * 100) / 32767;
                            volume = volume * 3;
                            if (volume > 100) volume = 100;
                            
                            bool newlySpeaking = false;"""

replacement = """                            int volume = (maxAmp * 100) / 32767;
                            volume = volume * 3;
                            if (volume > 100) volume = 100;
                            
                            LogToFile("UDP AUDIO RECV: maxAmp=" + std::to_string(maxAmp) + " volume=" + std::to_string(volume) + " wname=" + w2a(wname.c_str()));
                            
                            bool newlySpeaking = false;"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Log added")
else:
    print("Target not found")
