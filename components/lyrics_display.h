#pragma once
#include "common.h"
#include "lyrics_fetcher.h"

// Status messages shown in the lyrics area when no lines are available.
enum class DisplayStatus {
    None,       // normal lyrics display
    Searching,  // "Searching lyrics......"
    NoLyrics,   // "No lyrics was found maybe try another song"
    NoMedia     // "No detected media"
};

// Tracks which lyric line is "current" for the given playback position
namespace lyrics_display {
    constexpr UINT_PTR TIMER_ID_LYRICS_ANIM = 2;

    void initialize(HWND hwnd);
    void cleanup();

    // Replaces the loaded lyrics (e.g. once the background fetch completes).
    // If lines is non-empty, resets the display status to None.
    void set_lines(vector<LyricLine> lines);

    // Show a status message in the lyrics area (displayed only when no lines
    // are available). Pass DisplayStatus::None to clear it.
    void set_status(DisplayStatus status);

    // Call on every timeline position update (e.g. from the existing 500ms
    // timer) to re-check which line should be current and, if it advanced
    // by exactly one line, kick off the slide animation.
    void sync(float position_seconds);

    // Call when TIMER_ID_LYRICS_ANIM fires; advances/ends the animation.
    void handle_anim_timer();

    // Seconds added to the playback position before matching a line; positive = lyrics appear earlier.
    float get_offset();
    void set_offset(float offset_seconds);
    void reset_offset();

    // Draws the currently visible lyric lines into the given area of hdc.
    void draw(HDC hdc, const RECT& area);
}