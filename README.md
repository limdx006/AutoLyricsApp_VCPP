# AutoLyricsApp_VCPP

Windows desktop C++ app for synced lyrics display using the Win32 API and Windows Media Session API.

## Features

- Real-time synchronized lyrics display with smooth slide animation
- Auto-detect language (English, Chinese, Japanese, Korean) from lyric text
- Mode toggle (Original / Translated) — "Current:" shows Romaji/PinYin dynamically
- Offset adjustment (buttons ±0.1 or direct edit, range ±9.9 s, default +1.0 s)
- Media playback controls (play/pause, next, previous)
- High-resolution timer queue for accurate timeline tracking
- **No Python required at runtime** — lyrics fetcher is bundled as a standalone `.exe` via PyInstaller
- Pin-to-top window, dark theme UI

## Build (two-step)

Requires Visual Studio 2022 Build Tools, vcpkg (`nlohmann/json`), and PyInstaller.

```batch
python components\build_py_exe.py   # Step 1: bundle lyrics_fetcher.py → py_helper.exe
build.bat                            # Step 2: embed py_helper.exe + compile C++
```

The resulting `main.exe` is fully self-contained (~30 MB).

## Project structure

```
main.cpp                     Entry point
app.rc                       Resources (icon + embedded scripts)
build.bat                    Build script (C++ step)
build_py_exe.py              PyInstaller build script
py_helper.exe                Standalone lyrics fetcher (embedded via RCDATA)
lyrics_fetcher.py            Source for py_helper.exe (dev fallback)
lyrics_translator.py         Romanization script (future use)
components/
  config.h / config.cpp      Layout constants and colors
  gui.h / gui.cpp            Main window, rendering, event loop
  timeline_tracker.h/.cpp    Timer, WinRT polling, position interpolation
  media_session.h/.cpp       WinRT media session queries
  lyrics_fetcher.h/.cpp      Helper extraction + LRC parsing
  lyrics_display.h/.cpp      Animated lyrics rendering
  language_detector.h/.cpp   Unicode-range language detection
  playback_controls.h/.cpp   Media transport controls
  auto_nudge.h/.cpp          Startup session wake-up
  time_formatter.h/.cpp      Second-to-display-time formatting
  common.h                   Shared includes, utf8_to_wide helper
```

## Dependencies (build only)

- **Python 3 + PyInstaller** (for building `py_helper.exe`)
- **vcpkg:** `nlohmann/json`
- **nuget:** None (pure Win32)
