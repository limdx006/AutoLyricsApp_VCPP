# AutoLyricsApp_VCPP

Windows desktop C++ app for synced lyrics display using the Win32 API and Windows Media Session API.

## Features

- Real-time synchronized lyrics display with smooth slide animation
- Auto-detect language (English, Chinese, Japanese, Korean) from lyric text
- Mode toggle (Original / Translated) — "Current:" shows Romaji/PinYin dynamically
- Multi-session media detection — automatically selects the best media session from all active sources (Spotify, Chrome, etc.)
- Offset adjustment (buttons ±0.1 or direct edit, range ±9.9 s, default +1.0 s)
- Media playback controls (play/pause, next, previous)
- High-resolution timer queue for accurate timeline tracking
- In-app log viewer for diagnostics
- **No Python required at runtime** — lyrics fetcher and translator are bundled as standalone `.exe` via PyInstaller
- Pin-to-top window, dark theme UI

## Build (two-step)

Requires Visual Studio 2022 Build Tools, vcpkg (`nlohmann/json`), and PyInstaller.

```batch
python components\build_py_exe.py   # Step 1: bundle lyrics_fetcher.py + lyrics_translator.py → py_helper.exe + py_translator.exe
build.bat                            # Step 2: embed both exes + compile C++
```

The resulting `main.exe` is fully self-contained (~90 MB due to PyInstaller bundles).

## Project structure

```
main.cpp                     Entry point
app.rc                       Resources (icon + embedded scripts via RCDATA)
build.bat                    Build script (C++ step)
build_py_exe.py              PyInstaller build script (produces py_helper.exe + py_translator.exe)
py_helper.exe                Standalone lyrics fetcher (embedded via RCDATA)
py_translator.exe            Standalone romanization script (embedded via RCDATA)
lyrics_fetcher.py            Source for py_helper.exe (dev fallback)
lyrics_translator.py         Romanization/batch conversion (dev fallback)
components/
  config.h / config.cpp      Layout constants and colors
  gui.h / gui.cpp            Main window, rendering, event loop
  timeline_tracker.h/.cpp    WinRT session polling, song detection, lyrics-fetch dispatch
  local_timer.h/.cpp         High-res timer queue, position interpolation, display sync
  media_selector.h/.cpp      Multi-session scoring engine (picks the best media source)
  media_session.h/.cpp       WinRT media session queries
  lyrics_fetcher.h/.cpp      Helper extraction + LRC parsing + romanization dispatch
  lyrics_display.h/.cpp      Animated lyrics rendering with font caching
  language_detector.h/.cpp   Unicode-range language detection
  playback_controls.h/.cpp   Media transport controls
  auto_nudge.h/.cpp          Startup session wake-up
  time_formatter.h/.cpp      Second-to-display-time formatting
  log_viewer.h/.cpp          In-app diagnostic log window
  common.h                   Shared includes, utf8_to_wide helper
```

## Dependencies (build only)

- **Python 3 + PyInstaller** (for building the helper executables)
- **vcpkg:** `nlohmann/json`
- **nuget:** None (pure Win32)
