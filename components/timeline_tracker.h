#pragma once

#include "common.h"
#include "config.h"

namespace timeline_tracker {
    constexpr UINT_PTR TIMER_ID_TIMELINE_UPDATE = 1;
    constexpr UINT TIMELINE_UPDATE_INTERVAL_MS = 500;

    void initialize(HWND hwnd, HINSTANCE hInstance);
    void cleanup();
    void refresh_from_media();
    void updateTimelineDisplay();
    void handle_timer();
    double get_current_position_seconds();
    double get_duration_seconds();
    wstring get_current_title();
    wstring get_current_artist();
}