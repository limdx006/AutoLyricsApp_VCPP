#include "timeline_tracker.h"
#include "auto_nudge.h"
#include "media_session.h"
#include "time_formatter.h"
#include <algorithm>
#include <chrono>

namespace timeline_tracker {
    static HWND g_hwnd = nullptr;
    static HINSTANCE g_hInstance = nullptr;
    static double g_current_position_seconds = 0.0;
    static double g_duration_seconds = 0.0;
    static bool g_is_playing = false;
    static bool g_has_valid_position = false;
    static ULONGLONG g_last_update_tick = 0;

    static void update_controls()
    {
        if (g_hwnd == nullptr)
            return;

        std::string currentTimeText = format_display_time(static_cast<float>(g_current_position_seconds));
        std::string endTimeText = format_display_time(static_cast<float>(g_duration_seconds));
        CURRENT_TIME = std::wstring(currentTimeText.begin(), currentTimeText.end());
        END_TIME = std::wstring(endTimeText.begin(), endTimeText.end());

        HWND hCurrTimeCtrl = GetDlgItem(g_hwnd, ID_STATIC_CURR_TIME);
        HWND hEndTimeCtrl = GetDlgItem(g_hwnd, ID_STATIC_END_TIME);

        if (hCurrTimeCtrl)
        {
            SetWindowTextW(hCurrTimeCtrl, CURRENT_TIME.c_str());
            RedrawWindow(hCurrTimeCtrl, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }

        if (hEndTimeCtrl)
        {
            SetWindowTextW(hEndTimeCtrl, END_TIME.c_str());
            RedrawWindow(hEndTimeCtrl, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }

    void initialize(HWND hwnd, HINSTANCE hInstance)
    {
        g_hwnd = hwnd;
        g_hInstance = hInstance;
        g_has_valid_position = false;
        g_last_update_tick = GetTickCount64();

        refresh_from_media();
        SetTimer(g_hwnd, TIMER_ID_TIMELINE_UPDATE, TIMELINE_UPDATE_INTERVAL_MS, nullptr);
    }

    void cleanup()
    {
        if (g_hwnd != nullptr)
        {
            KillTimer(g_hwnd, TIMER_ID_TIMELINE_UPDATE);
            g_hwnd = nullptr;
            g_hInstance = nullptr;
        }
    }

    void refresh_from_media()
    {
        auto_nudge(0.1);

        MediaSessionInfo media = get_media_session_info();
        if (!media.is_success)
            return;

        g_current_position_seconds = (std::max)(0.0, static_cast<double>(media.position));
        g_duration_seconds = (std::max)(0.0, static_cast<double>(media.duration));
        g_is_playing = media.is_playing;
        g_has_valid_position = true;
        g_last_update_tick = GetTickCount64();

        updateTimelineDisplay();
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
        const double elapsed_seconds = static_cast<double>(now - g_last_update_tick) / 1000.0;
        if (elapsed_seconds <= 0.0)
            return;

        g_current_position_seconds += elapsed_seconds;
        if (g_duration_seconds > 0.0)
            g_current_position_seconds = (std::min)(g_current_position_seconds, g_duration_seconds);

        g_last_update_tick = now;
        update_controls();
    }
}