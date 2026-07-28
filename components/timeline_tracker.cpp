#include "timeline_tracker.h"
#include "media_session.h"
#include "gui.h"
#include "log_viewer.h"
#include "lyrics_display.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace timeline_tracker {
    static HWND g_hwnd = nullptr;
    static double g_last_window_position_seconds = -1.0;
    static bool g_has_window_position = false;
    static wstring g_current_title;
    static wstring g_current_artist;
    static string g_last_lyrics_title;  // title/artist of the last song lyrics were fetched for
    static string g_last_lyrics_artist;
    static std::atomic<bool> g_lyrics_fetching{false};  // guard against concurrent Python subprocesses

    void initialize(HWND hwnd, HINSTANCE hInstance)
    {
        g_hwnd = hwnd;
        g_last_window_position_seconds = -1.0;
        g_has_window_position = false;
        g_current_title.clear();
        g_current_artist.clear();

        local_timer::initialize(hwnd, hInstance);
        refresh_from_media();
    }

    void cleanup()
    {
        local_timer::cleanup();
        g_hwnd = nullptr;
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
                log_viewer::log("[FETCH] Fetching lyrics for: %s - %s\n", title.c_str(), artist.c_str());
                LyricsResult result = fetch_lyrics(title, artist);
                g_lyrics_fetching = false;
                if (result.success)
                {
                    log_viewer::log("[FETCH] Lyrics found (%zu lines)\n", result.lines.size());
                    SubmitLyrics(result.lines);
                }
                else
                {
                    log_viewer::log("[FETCH] No synced lyrics found.\n");
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
            log_viewer::log("[SESSION] window_pos=%.3f duration=%.3f\n", window_position, window_duration);
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
                local_timer::apply_session(-1.0, window_duration, media.is_playing);
                return false;
            }
        }

        local_timer::apply_session(window_position, window_duration, media.is_playing);
        g_last_window_position_seconds = window_position;
        g_has_window_position = true;
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
            local_timer::set_valid_position(false);
            return;
        }

        // Clear the no-media banner now that a session exists.
        lyrics_display::set_status(DisplayStatus::None);

        apply_media_state(media, true);
        local_timer::update_display();
    }

    double get_current_position_seconds()
    {
        return local_timer::get_position_seconds();
    }

    double get_duration_seconds()
    {
        return local_timer::get_duration_seconds();
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
        local_timer::update_display();
    }

    void handle_timer()
    {
        // ── Priority 2: Local interpolation ──
        local_timer::interpolate();

        // ── Priority 1: WinRT session update ──
        MediaSessionInfo media = get_media_session_info();
        if (media.is_success)
        {
            apply_media_state(media, false);

            // Keep the seek-detection anchor in sync even when the
            // position wasn't overwritten.
            if (g_has_window_position)
                g_last_window_position_seconds = (std::max)(0.0, media.position);

            // Ensure title/artist always match the current session.
            g_current_title = utf8_to_wide(media.title.empty() ? string("Unknown Title") : media.title);
            g_current_artist = utf8_to_wide(media.artist.empty() ? string("Unknown Artist") : media.artist);
        }

        local_timer::update_display();
    }
}