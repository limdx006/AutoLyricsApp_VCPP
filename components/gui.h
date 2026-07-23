#pragma once
#include <windows.h>
#include <commctrl.h>
#include "config.h"
#include "playback_controls.h"
#include "auto_nudge.h"
#include "lyrics_fetcher.h"

// Posted to the main window once a background lyrics fetch completes;
// lParam is a heap-allocated vector<LyricLine>* (see SubmitLyrics/WindowProc).
#define WM_APP_LYRICS_READY (WM_APP + 1)

int RunGui(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateHeaderControls(HWND parent, HINSTANCE hInstance);
void RefreshHeaderText(HWND parent, const wstring& songTitle);
void CreateLanguageBarControls(HWND parent, HINSTANCE hInstance);
void CreateLyricsAreaControls(HWND parent, HINSTANCE hInstance);
void CreateBottomControls(HWND parent, HINSTANCE hInstance);
void TogglePlayPause(HWND hwnd);

// Thread-safe: hands parsed lyric lines to the GUI thread from any thread
// (e.g. the background thread doing the actual lyrics search).
void SubmitLyrics(vector<LyricLine> lines);