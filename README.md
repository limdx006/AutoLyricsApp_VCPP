# AutoLyricsApp_VCPP

Windows desktop C++ app for synced lyrics display using the Win32 API and Windows Media Session API.

## Features

- Real-time synchronized lyrics display with smooth slide animation
- Auto-detect language (English, Chinese, Japanese, Korean) from lyric text
- Mode toggle (Original / Translated) — English songs hide Translated automatically
- Offset adjustment (buttons ±0.1 or direct edit, range ±9.9 s, default +1.0 s)
- Media playback controls (play/pause, next, previous)
- High-resolution timer queue for accurate timeline tracking
- Embedded Python lyrics fetcher — single `main.exe`, no external `.py` file needed
- Pin-to-top window, dark theme UI

## Build

Requires Visual Studio 2022 Build Tools and vcpkg (`nlohmann/json`).

```batch
build.bat
```

## Project structure

```
main.cpp                     Entry point
app.rc                       Resources (icon + embedded lyrics_fetcher.py)
build.bat                    Build script
lyrics_fetcher.py            Embedded Python script (via RCDATA)
components/
  config.h / config.cpp      Layout constants and colors
  gui.h / gui.cpp            Main window, rendering, event loop
  timeline_tracker.h/.cpp    Timer, WinRT polling, position interpolation
  media_session.h/.cpp       WinRT media session queries
  lyrics_fetcher.h/.cpp      Embedded script extraction + LRC parsing
  lyrics_display.h/.cpp      Animated lyrics rendering
  language_detector.h/.cpp   Unicode-range language detection
  playback_controls.h/.cpp   Media transport controls
  auto_nudge.h/.cpp          Startup session wake-up
  time_formatter.h/.cpp      Second-to-display-time formatting
  common.h                   Shared includes, utf8_to_wide helper
```

## Dependencies

- **Python 3** (system-installed, used at runtime for `syncedlyrics`)
- **vcpkg:** `nlohmann/json`
- **nuget:** None (pure Win32)
