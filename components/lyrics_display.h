#pragma once
#include "common.h"
#include "lyrics_fetcher.h"

// Tracks which lyric line is "current" for the given playback position,
// animates the slide to the next line, and draws the visible window of
// lines (current line bold/larger, neighbors dimmer/smaller).
namespace lyrics_display {
    constexpr UINT_PTR TIMER_ID_LYRICS_ANIM = 2;

    void initialize(HWND hwnd);
    void cleanup();

    // Replaces the loaded lyrics (e.g. once the background fetch completes).
    void set_lines(vector<LyricLine> lines);

    // Call on every timeline position update (e.g. from the existing 500ms
    // timer) to re-check which line should be current and, if it advanced
    // by exactly one line, kick off the slide animation.
    void sync(double position_seconds);

    // Call when TIMER_ID_LYRICS_ANIM fires; advances/ends the animation.
    void handle_anim_timer();

    // Draws the currently visible lyric lines into the given area of hdc.
    void draw(HDC hdc, const RECT& area);
}
