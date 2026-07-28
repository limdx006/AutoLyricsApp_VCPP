#pragma once
#include "common.h"
#include "config.h"

// High-resolution timer that interpolates the playback position between
// WinRT session polls and drives the time display / lyrics sync.
namespace local_timer {
    constexpr UINT TIMELINE_UPDATE_INTERVAL_MS = 500;

    void initialize(HWND hwnd, HINSTANCE hInstance);
    void cleanup();

    // Advance g_current_position_seconds by the wall-clock elapsed time
    // since the last call.  Call this first on every timer tick.
    void interpolate();

    // Overwrite position/duration/playing with authoritative session data.
    // Pass pos < 0 to leave the current position unchanged (e.g. when
    // seek-detection decides to keep the interpolated value).
    void apply_session(double pos, double dur, bool playing);

    // Position state accessors (replaces timeline_tracker versions).
    void set_valid_position(bool valid);
    bool has_valid_position();
    double get_position_seconds();
    double get_duration_seconds();
    bool is_playing();

    // Update the time-display controls and sync lyrics with the current
    // position.  Called at the end of every timer tick.
    void update_display();
}
