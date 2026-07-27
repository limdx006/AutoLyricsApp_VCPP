#include "timeline_tracker.h"
#include "media_session.h"
#include "time_formatter.h"
#include "gui.h"
#include "lyrics_display.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace timeline_tracker {
    static HWND g_hwnd = nullptr;
    static HINSTANCE g_hInstance = nullptr;
    static double g_current_position_seconds = 0.0;
    static double g_duration_seconds = 0.0;
    static bool g_is_playing = false;
    static bool g_has_valid_position = false;
    static ULONGLONG g_last_update_tick = 0;
    static double g_last_window_position_seconds = -1.0;
    static bool g_has_window_position = false;
    static wstring g_current_title;
    static wstring g_current_artist;
    static string g_last_lyrics_title;  // title/artist of the last song lyrics were fetched for
    static string g_last_lyrics_artist;
    static std::atomic<bool> g_lyrics_fetching{false};  // guard against concurrent Python subprocesses
    static HANDLE g_hTimerQueue = nullptr;  // high-resolution timer queue
    static HANDLE g_hTimer = nullptr;

    // Callback from the high-resolution timer queue — runs on a thread pool
    // thread, not the UI thread.  Post a message so handle_timer runs on the
    // UI thread.
    static void CALLBACK timer_queue_callback(PVOID, BOOLEAN)
    {
        if (g_hwnd)
            PostMessageW(g_hwnd, WM_APP_TIMELINE_TICK, 0, 0);
    }

    static void update_controls()
    {
        if (g_hwnd == nullptr)
            return;

        std::string currentTimeText = format_display_time(g_current_position_seconds);
        std::string endTimeText = format_display_time(g_duration_seconds);
        CURRENT_TIME = std::wstring(currentTimeText.begin(), currentTimeText.end());
        END_TIME = std::wstring(endTimeText.begin(), endTimeText.end());

        // Cache child HWNDs on first use so GetDlgItem (O(n) in child count)
        // doesn't run every tick.
        static HWND s_hCurrTime = nullptr;
        static HWND s_hEndTime  = nullptr;
        static HWND s_hSong     = nullptr;
        static HWND s_hArtist   = nullptr;
        if (!s_hCurrTime)
        {
            s_hCurrTime = GetDlgItem(g_hwnd, ID_STATIC_CURR_TIME);
            s_hEndTime  = GetDlgItem(g_hwnd, ID_STATIC_END_TIME);
            s_hSong     = GetDlgItem(g_hwnd, ID_STATIC_SONG);
            s_hArtist   = GetDlgItem(g_hwnd, ID_STATIC_ARTIST);
        }

        // Only touch the control (and repaint) if its text actually changed
        auto setIfChanged = [](HWND ctrl, const wstring& newText) -> bool
        {
            if (!ctrl) return false;
            int len = GetWindowTextLengthW(ctrl);
            wstring current(len, L'\0');
            if (len > 0)
                GetWindowTextW(ctrl, &current[0], len + 1);
            if (current != newText)
            {
                SetWindowTextW(ctrl, newText.c_str());
                return true;
            }
            return false;
        };

        setIfChanged(s_hCurrTime, CURRENT_TIME);
        setIfChanged(s_hEndTime, END_TIME);
        bool titleChanged = setIfChanged(s_hSong, g_current_title);
        setIfChanged(s_hArtist, g_current_artist);

        // New song -> re-measure and resize the header box for it
        if (titleChanged)
            RefreshHeaderText(g_hwnd, g_current_title);

        // Invalidate (not RDW_UPDATENOW) so the progress bar updates without forcing a synchronous repaint
        if (g_hwnd)
            InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    void initialize(HWND hwnd, HINSTANCE hInstance)
    {
        g_hwnd = hwnd;
        g_hInstance = hInstance;
        g_has_valid_position = false;
        g_last_update_tick = GetTickCount64();
        g_last_window_position_seconds = -1.0;
        g_has_window_position = false;
        g_current_title.clear();
        g_current_artist.clear();

        refresh_from_media();

        // Create a high-resolution timer queue (runs callback on thread pool,
        // avoiding WM_TIMER's low-priority coalescing and ~16ms granularity).
        g_hTimerQueue = CreateTimerQueue();
        if (g_hTimerQueue)
        {
            CreateTimerQueueTimer(&g_hTimer, g_hTimerQueue,
                                  timer_queue_callback, nullptr,
                                  TIMELINE_UPDATE_INTERVAL_MS,  // due time
                                  TIMELINE_UPDATE_INTERVAL_MS,  // period
                                  WT_EXECUTELONGFUNCTION);      // allow >10ms callbacks
        }
    }

    void cleanup()
    {
        if (g_hTimer)
        {
            DeleteTimerQueueTimer(g_hTimerQueue, g_hTimer, INVALID_HANDLE_VALUE);
            g_hTimer = nullptr;
        }
        if (g_hTimerQueue)
        {
            DeleteTimerQueueEx(g_hTimerQueue, INVALID_HANDLE_VALUE);
            g_hTimerQueue = nullptr;
        }

        if (g_hwnd != nullptr)
        {
            g_hwnd = nullptr;
            g_hInstance = nullptr;
        }
        g_lyrics_fetching = false;
    }

    // Returns true when the position actually changed (seek / new song) so the
    // caller can decide whether a full display update is needed.
    static bool apply_media_state(const MediaSessionInfo& media, bool force_position = true)
    {
        // Detect a song change by title/artist rather than position
        if (!media.title.empty() && (media.title != g_last_lyrics_title || media.artist != g_last_lyrics_artist))
        {
            g_last_lyrics_title = media.title;
            g_last_lyrics_artist = media.artist;

            // Clear the previous song's lyrics and reset offset right away.
            lyrics_display::set_lines({});
            lyrics_display::set_status(DisplayStatus::Searching);
            lyrics_display::reset_offset();
            ResetModeToOriginal(g_hwnd);

            // Update the offset edit box to show the reset value.
            if (g_hwnd)
            {
                HWND hEdit = GetDlgItem(g_hwnd, ID_EDIT_OFFSET);
                if (hEdit)
                {
                    wchar_t buf[16];
                    swprintf(buf, 16, L"%.1f", lyrics_display::get_offset());
                    SetWindowTextW(hEdit, buf);
                }
            }

            string title = media.title;
            string artist = media.artist;
            HWND hwnd = g_hwnd;

            // Atomically claim the fetch slot to prevent multiple concurrent
            // Python subprocesses (each ~30-50 MB RSS) when the user skips
            // songs rapidly.  If a fetch is already in flight, skip this
            // song change; the next one will pick up the new title.
            if (g_lyrics_fetching.exchange(true))
                return false;

            std::thread([title, artist, hwnd]() {
                cout << "Fetching lyrics for: " << title << " - " << artist << "\n";
                LyricsResult result = fetch_lyrics(title, artist);
                g_lyrics_fetching = false;
                if (result.success)
                {
                    cout << "Lyrics found (" << result.lines.size() << " lines)\n";
                    SubmitLyrics(result.lines);
                }
                else
                {
                    cout << "No synced lyrics found.\n";
                    if (hwnd)
                        PostMessageW(hwnd, WM_APP_LYRICS_STATUS, (WPARAM)DisplayStatus::NoLyrics, 0);
                }
            }).detach();
        }

        const double window_position = (std::max)(0.0, media.position);
        const double window_duration = (std::max)(0.0, media.duration);

        // Print when the WinRT session returns a genuinely new position value
        static double s_last_winrt_position = -1.0;
        if (std::abs(window_position - s_last_winrt_position) > 0.01)
        {
            printf("[SESSION] window_pos=%.3f duration=%.3f\n", window_position, window_duration);
            s_last_winrt_position = window_position;
        }

        // When not forced, keep the interpolated position during normal
        // playback and only snap to WinRT on a genuine seek (>2 s drift
        // from the last applied WinRT anchor).
        if (!force_position && g_has_window_position && g_last_window_position_seconds >= 0.0)
        {
            const double seek_delta = std::abs(window_position - g_last_window_position_seconds);
            if (seek_delta < 1.0)  // normal inter-tick drift (~0.5 s) + small margin
            {
                g_duration_seconds = window_duration;
                g_is_playing = media.is_playing;
                g_has_valid_position = true;
                return false;
            }
        }

        g_current_position_seconds = window_position;
        g_duration_seconds = window_duration;
        g_is_playing = media.is_playing;
        g_has_valid_position = true;
        g_last_window_position_seconds = window_position;
        g_has_window_position = true;
        g_last_update_tick = GetTickCount64();
        g_current_title = media.title.empty() ? wstring(L"Unknown Title") : utf8_to_wide(media.title);
        g_current_artist = media.artist.empty() ? wstring(L"Unknown Artist") : utf8_to_wide(media.artist);
        return true;
    }

    void refresh_from_media()
    {
        MediaSessionInfo media = get_media_session_info();
        if (!media.is_success)
        {
            lyrics_display::set_status(DisplayStatus::NoMedia);
            return;
        }

        // Clear the no-media banner now that a session exists.
        lyrics_display::set_status(DisplayStatus::None);

        apply_media_state(media, true);  // force_position = from WinRT
        updateTimelineDisplay();
        lyrics_display::sync(g_current_position_seconds);
    }

    double get_current_position_seconds()
    {
        return g_current_position_seconds;
    }

    double get_duration_seconds()
    {
        return g_duration_seconds;
    }

    wstring get_current_title()
    {
        if (!g_current_title.empty())
            return g_current_title;

        return wstring(SONG_NAME ? SONG_NAME : L"Unknown Title");
    }

    wstring get_current_artist()
    {
        if (!g_current_artist.empty())
            return g_current_artist;

        return wstring(ARTIST_NAME ? ARTIST_NAME : L"Unknown Artist");
    }

    void updateTimelineDisplay()
    {
        if (!g_has_valid_position)
            return;

        update_controls();
    }

    void handle_timer()
    {
        if (!g_has_valid_position)
            return;

        const ULONGLONG now = GetTickCount64();
        const double elapsed = static_cast<double>(now - g_last_update_tick) / 1000.0;
        if (elapsed <= 0.0)
            return;

        // Diagnostic: log the actual interval between handle_timer calls
        static ULONGLONG s_last_call_tick = 0;
        if (s_last_call_tick == 0)
            s_last_call_tick = now;
        const double call_interval = static_cast<double>(now - s_last_call_tick) / 1000.0;
        s_last_call_tick = now;
        // printf("[TICK] call_interval=%.3fs elapsed=%.3fs pos=%.3f\n", call_interval, elapsed, g_current_position_seconds);

        // 1. Local interpolation — advance the clock by wall-clock time so
        //    the displayed time ticks forward smoothly each tick (~500 ms).
        if (g_is_playing)
        {
            g_current_position_seconds += elapsed;
            if (g_duration_seconds > 0.0)
                g_current_position_seconds = (std::min)(g_current_position_seconds, g_duration_seconds);
        }

        // 2. Local interpolation alone is accurate enough for the display
        //    between WinRT queries, so throttle the blocking WinRT call
        //    (~10-50 ms) to every 4th tick (~2 s) instead of every tick.
        //    The call is still responsive enough for seek/song detection.
        static int s_skip = 0;
        if (++s_skip >= 4)
        {
            s_skip = 0;

            MediaSessionInfo media = get_media_session_info();
            if (media.is_success)
            {
                // force_position = false — the interpolated position
                // survives during normal playback; only genuine seeks
                // trigger a WinRT overwrite.
                apply_media_state(media, false);

                // Regardless of whether the position was overwritten,
                // keep g_last_window_position_seconds in sync with the
                // latest WinRT sample so the seek-detection delta doesn't
                // accumulate across multiple throttled ticks.
                g_last_window_position_seconds = (std::max)(0.0, media.position);

                // Ensure title/artist always match the current session.
                g_current_title = utf8_to_wide(media.title.empty() ? string("Unknown Title") : media.title);
                g_current_artist = utf8_to_wide(media.artist.empty() ? string("Unknown Artist") : media.artist);
            }
        }

        // 3. Reset tick clock using the start-of-tick time so the next
        //    call's elapsed measures the full wall-clock interval — not
        //    minus the processing time of this call — avoiding cumulative
        //    drift.
        g_last_update_tick = now;
        update_controls();

        // Sync lyrics with the final (interpolated or seek-corrected)
        // position immediately, so the lyric line change is as tight as
        // possible to the audio.
        lyrics_display::sync(g_current_position_seconds);
    }
}