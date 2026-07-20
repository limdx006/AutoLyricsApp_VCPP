#pragma once
#include <windows.h>
#include <commctrl.h>
#include "config.h"


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateHeaderControls(HWND parent, HINSTANCE hInstance);
void CreateLanguageBarControls(HWND parent, HINSTANCE hInstance);
void CreateLyricsAreaControls(HWND parent, HINSTANCE hInstance);
void CreateBottomControls(HWND parent, HINSTANCE hInstance);
void TogglePlayPause(HWND hwnd);