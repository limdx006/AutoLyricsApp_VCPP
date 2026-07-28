#pragma once

#include "common.h"
#include "config.h"
#include "local_timer.h"

namespace timeline_tracker {
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