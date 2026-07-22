#pragma once
#include <windows.h>
#include <commctrl.h>
#include "config.h"
#include "playback_controls.h"
#include "auto_nudge.h"

int RunGui(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateHeaderControls(HWND parent, HINSTANCE hInstance);
void RefreshHeaderText(HWND parent, const wstring& songTitle);
void CreateLanguageBarControls(HWND parent, HINSTANCE hInstance);
void CreateLyricsAreaControls(HWND parent, HINSTANCE hInstance);
void CreateBottomControls(HWND parent, HINSTANCE hInstance);
void TogglePlayPause(HWND hwnd);