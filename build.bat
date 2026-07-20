
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64
 
cd /d D:\Personal_GitHub_rep\AutoLyricsApp_VCPP

cl /EHsc /std:c++20 ^
main.cpp ^
components\*.cpp ^
/Fo"obj\\" ^
/Fd"obj\\" ^
/I"D:\Personal_GitHub_rep\AutoLyricsApp_VCPP\components" ^
/I"D:\Personal_GitHub_rep\AutoLyricsApp_VCPP\vcpkg\installed\x64-windows\include" ^
/link ^
windowsapp.lib

.\main.exe
 
pause
 