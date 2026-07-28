#include "local_timer.h"
#include "timeline_tracker.h"
#include "time_formatter.h"
#include "lyrics_display.h"
#include "gui.h"
#include <algorithm>

namespace local_timer {
    static HWND g_hwnd = nullptr;
    static HANDLE g_hTimerQueue = nullptr;
    static HANDLE g_hTimer = nullptr;
    static double g_current_position_seconds = 0.0;
    static double g_duration_seconds = 0.0;
    static bool g_is_playing = false;
    static bool g_has_valid_position = false;
    static ULONGLONG g_last_update_tick = 0;

    // Called from the thread-pool timer queue — posts to the UI thread so
    // the interpolate / update logic runs on the message loop.
    static void CALLBACK timer_queue_callback(PVOID, BOOLEAN)
    {
        if (g_hwnd)
            PostMessageW(g_hwnd, WM_APP_TIMELINE_TICK, 0, 0);
    }

    // Updates the time-display controls (current time, end time, title,
    // artist) and invalidates the window for progress-bar redraw.
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
        bool titleChanged = setIfChanged(s_hSong, timeline_tracker::get_current_title());
        setIfChanged(s_hArtist, timeline_tracker::get_current_artist());

        // New song -> re-measure and resize the header box for it.
        if (titleChanged)
            RefreshHeaderText(g_hwnd, timeline_tracker::get_current_title());

        if (g_hwnd)
            InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    void initialize(HWND hwnd, HINSTANCE hInstance)
    {
        g_hwnd = hwnd;
        g_has_valid_position = false;
        g_last_update_tick = GetTickCount64();

        // Create a high-resolution timer queue (runs callback on thread pool,
        // avoiding WM_TIMER's low-priority coalescing and ~16ms granularity).
        g_hTimerQueue = CreateTimerQueue();
        if (g_hTimerQueue)
        {
            CreateTimerQueueTimer(&g_hTimer, g_hTimerQueue,
                                  timer_queue_callback, nullptr,
                                  TIMELINE_UPDATE_INTERVAL_MS,
                                  TIMELINE_UPDATE_INTERVAL_MS,
                                  WT_EXECUTELONGFUNCTION);
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
        g_hwnd = nullptr;
    }

    void interpolate()
    {
        if (!g_has_valid_position)
            return;

        const ULONGLONG now = GetTickCount64();
        const double elapsed = static_cast<double>(now - g_last_update_tick) / 1000.0;
        if (elapsed <= 0.0)
            return;

        if (g_is_playing)
        {
            g_current_position_seconds += elapsed;
            if (g_duration_seconds > 0.0)
                g_current_position_seconds = (std::min)(g_current_position_seconds, g_duration_seconds);
        }

        g_last_update_tick = now;
    }

    void apply_session(double pos, double dur, bool playing)
    {
        if (pos >= 0.0)
            g_current_position_seconds = pos;
        g_duration_seconds = dur;
        g_is_playing = playing;
        g_has_valid_position = true;
        g_last_update_tick = GetTickCount64();
    }

    void set_valid_position(bool valid)
    {
        g_has_valid_position = valid;
    }

    bool has_valid_position()
    {
        return g_has_valid_position;
    }

    double get_position_seconds()
    {
        return g_current_position_seconds;
    }

    double get_duration_seconds()
    {
        return g_duration_seconds;
    }

    bool is_playing()
    {
        return g_is_playing;
    }

    void update_display()
    {
        if (!g_has_valid_position)
            return;

        update_controls();
        lyrics_display::sync(g_current_position_seconds);
    }
}
