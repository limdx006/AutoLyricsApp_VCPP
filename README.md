# AutoLyricsApp_VCPP

Windows desktop C++ app for synced lyrics display using the Win32 API and Windows Media Session API.

## Current project status

This project is currently in a prototype / archived state. It can work for some songs and some sessions, but it is not stable enough for general daily use.

As of the current codebase, the project is effectively considered abandoned for further development because the underlying data source and session detection are too unreliable in real-world use.

## What the app tries to do

- Real-time synchronized lyrics display with smooth slide animation
- Auto-detect language (English, Chinese, Japanese, Korean) from lyric text
- Language bar with Original / Translated toggle
- Multi-session media detection across active Windows media sessions
- Offset adjustment and playback controls
- Embedded Python helpers for lyrics fetch + romanization
- Dark theme desktop UI

## Current architecture

The project is organized around:

- Win32 UI in `components/gui.*`
- WinRT media session polling in `components/media_selector.*` and `components/timeline_tracker.*`
- embedded Python helper executables for lyrics lookup and translation
- local lyric rendering and timing logic in `components/lyrics_display.*` and `components/local_timer.*`

## Known blocking issues

The main reasons this project is effectively given up are:

1. Media session detection is flaky
   - Chrome / Edge / browser-based media sessions frequently shift title/artist metadata, or present noisy placeholder values.
   - Multiple active sessions can compete, and the selector may switch between them unpredictably.
   - Background probes can temporarily score a wrong session as the current song and then poison later lyrics attempts.

2. Cache poisoning / stale negative results
   - A failed lookup can be cached as a negative result and later reused even when a different query would succeed.
   - This produces the exact symptom of “No synced lyrics found” appearing repeatedly for the same song even after rerun or refresh.
   - Clearing stale results helps, but the underlying data source remains fragile enough that the app still gets stuck often.

3. No reliable fallback provider
   - The project depends on a third-party lyrics service which is inconsistent and not controllable.
   - There is no stable offline or local fallback for synced lyrics.
   - The PyInstaller bundle approach adds build fragility without solving the data-quality problem.

4. Browser metadata / song identity drift
   - Many songs appear with incomplete or shifting artist/title strings.
   - A song can look like a valid track one moment and a placeholder/stream title the next.
   - This makes matching lyrics reliably difficult without a much stronger metadata normalization layer.

## Practical conclusion

This project is not currently production-ready and should be treated as a research prototype or a learning project rather than an app that can be depended on for real-world use.

The code is still useful as a reference implementation for:

- Win32 UI patterns
- WinRT media session polling
- background lyrics fetch orchestration
- UI and timing synchronization for lyrics display

But the overall feature goal is blocked by live metadata instability and inconsistent upstream lyrics results.

## Build (two-step)

Requires Visual Studio 2022 Build Tools, vcpkg (`nlohmann/json`), and PyInstaller.

```batch
python components\build_py_exe.py   # Step 1: bundle lyrics_fetcher.py + lyrics_translator.py → py_helper.exe + py_translator.exe
build.bat                            # Step 2: embed both exes + compile C++
```

The resulting `main.exe` is fully self-contained (~90 MB due to PyInstaller bundles), but this does not eliminate the underlying lyrics-source reliability issues.

## Project structure

```text
main.cpp                     Entry point
app.rc                       Resources (icon + embedded scripts via RCDATA)
build.bat                    Build script (C++ step)
components/
  config.h / config.cpp      Layout constants and colors
  gui.h / gui.cpp            Main window, rendering, event loop
  timeline_tracker.h/.cpp    WinRT session polling, song detection, lyrics-fetch dispatch
  local_timer.h/.cpp         High-res timer queue, position interpolation, display sync
  media_selector.h/.cpp      Multi-session scoring engine (picks the best media source)
  media_session.h/.cpp       WinRT media session queries
  lyrics_fetcher.h/.cpp      Helper extraction + LRC parsing + romanization dispatch
  lyrics_fetcher.py          Python lyrics lookup helper (dev fallback)
  lyrics_translator.py      Romanization/batch conversion helper
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

## Recommended future direction

If this project is resumed in the future, the main areas to fix first are:

- replace the unstable lyrics backend with a more deterministic provider
- normalize browser media metadata aggressively before fetch
- remove negative-cache poisoning from failed lyric lookups
- redesign song identity matching around a stronger canonical key
- add a real fallback strategy for no-result cases instead of cached false negatives

Until then, the code should be treated as a prototype and not as a dependable lyrics app.
