@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d D:\Personal_GitHub_rep\AutoLyricsApp_VCPP

cl /EHsc /std:c++20 ^
components/gui.cpp ^
components/config.cpp ^
/Fo"obj\\" ^
/Fd"obj\\" ^
/link ^
user32.lib gdi32.lib comctl32.lib

.\gui.exe

pause