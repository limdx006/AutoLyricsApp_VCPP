#include "timeline_tracker.h"
#include "media_session.h"
#include "time_formatter.h"
#include "gui.h"
#include "lyrics_display.h"
#include <algorithm>
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

    static bool apply_media_state(const MediaSessionInfo& media)
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
            std::thread([title, artist, hwnd]() {
                cout << "Fetching lyrics for: " << title << " - " << artist << "\n";
                LyricsResult result = fetch_lyrics(title, artist);
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

        if (g_has_window_position && g_last_window_position_seconds >= 0.0)
        {
            const double position_delta = std::abs(window_position - g_last_window_position_seconds);
            if (position_delta < 0.001)
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

        apply_media_state(media);
        updateTimelineDisplay();
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

        // 1. Local interpolation — advance the clock by wall-clock time so
        //    the displayed time ticks forward smoothly each tick (~500 ms).
        if (g_is_playing)
        {
            g_current_position_seconds += elapsed;
            if (g_duration_seconds > 0.0)
                g_current_position_seconds = (std::min)(g_current_position_seconds, g_duration_seconds);
        }

        // 2. Query WinRT every tick — authoritative correction for seeks.
        //    No throttling: seeks are detected within ~500 ms.
        MediaSessionInfo media = get_media_session_info();
        if (media.is_success)
        {
            apply_media_state(media);

            // Ensure title/artist always match the current session.
            g_current_title = utf8_to_wide(media.title.empty() ? string("Unknown Title") : media.title);
            g_current_artist = utf8_to_wide(media.artist.empty() ? string("Unknown Artist") : media.artist);
        }

        // 3. Reset tick clock NOW (after apply_media_state may have written
        //    to it) so the next tick always uses a clean wall-clock delta.
        g_last_update_tick = GetTickCount64();
        update_controls();
    }
}