@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d D:\Personal_GitHub_rep\AutoLyricsApp_VCPP

cl /EHsc /std:c++20 media_detection.cpp /link windowsapp.lib

.\media_detection.exe 

pause