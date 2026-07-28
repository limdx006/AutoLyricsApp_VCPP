#pragma once
#include "common.h"
#include "lyrics_fetcher.h"

// Status messages shown in the lyrics area when no lines are available.
enum class DisplayStatus {
    None,       // normal lyrics display
    Searching,  // "Searching lyrics......"
    NoLyrics,   // "No lyrics was found maybe try another song"
    NoMedia,    // "No detected media"
    Translating // "Translating lyrics..."
};

// Tracks which lyric line is "current" for the given playback position
namespace lyrics_display {
    constexpr UINT_PTR TIMER_ID_LYRICS_ANIM = 2;

    void initialize(HWND hwnd);
    void cleanup();

    // Replaces the loaded lyrics (e.g. once the background fetch completes).
    // If lines is non-empty, resets the display status to None.
    // Also clears any cached translations and resets to original display mode.
    void set_lines(vector<LyricLine> lines);

    // Store translated (romanized/pinyin) texts for the current song.
    // The timestamps match the original lines — sync() uses originals.
    void set_translated_texts(vector<wstring> texts);
    void clear_translated_texts();
    bool has_translated_texts();

    // Switch between showing original lyrics and translated texts.
    // When show is true but no cached translations exist, the status
    // message (Translating, if set via set_status) will be displayed.
    void set_show_translated(bool show);
    bool is_showing_translated();

    // Show a status message in the lyrics area (displayed only when no lines
    // are available). Pass DisplayStatus::None to clear it.
    void set_status(DisplayStatus status);

    // Call on every timeline position update (e.g. from the existing 500ms
    // timer) to re-check which line should be current and, if it advanced
    // by exactly one line, kick off the slide animation.
    void sync(double position_seconds);

    // Call when TIMER_ID_LYRICS_ANIM fires; advances/ends the animation.
    void handle_anim_timer();

    // Seconds added to the playback position before matching a line; positive = lyrics appear earlier.
    float get_offset();
    void set_offset(float offset_seconds);
    void reset_offset();

    // Returns the text of all currently loaded lyric lines (for the
    // translation thread to capture before dispatching).
    vector<wstring> get_lyric_texts();

    // Draws the currently visible lyric lines into the given area of hdc.
    void draw(HDC hdc, const RECT& area);
}