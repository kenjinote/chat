call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /EHsc /MD /O2 /D UNICODE /D _UNICODE /utf-8 /std:c++17 c:\Users\kenjinote\Desktop\chat\Source.cpp c:\Users\kenjinote\Desktop\chat\json11.cpp /link User32.lib Gdi32.lib Ole32.lib ws2_32.lib d2d1.lib dwrite.lib winmm.lib Advapi32.lib Comdlg32.lib /OUT:c:\Users\kenjinote\Desktop\chat\chat.exe
