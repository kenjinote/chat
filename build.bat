call "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
rc.exe /fo app.res app.rc
cl.exe /EHsc /MD /O2 /D UNICODE /D _UNICODE /utf-8 /std:c++17 c:\Users\kenjinote\Desktop\chat\Source.cpp c:\Users\kenjinote\Desktop\chat\json11.cpp c:\Users\kenjinote\Desktop\chat\H264Helper.cpp app.res /link User32.lib Gdi32.lib Ole32.lib OleAut32.lib ws2_32.lib d2d1.lib dwrite.lib winmm.lib Advapi32.lib Comdlg32.lib mfplat.lib mf.lib mfuuid.lib wmcodecdspuuid.lib Mfreadwrite.lib d3d11.lib windowsapp.lib /OUT:c:\Users\kenjinote\Desktop\chat\chat.exe
