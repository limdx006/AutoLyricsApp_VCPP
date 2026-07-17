@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

cl /EHsc /std:c++20 version_1.cpp /link windowsapp.lib

pause
