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

        std::string currentTimeText = format_display_time(static_cast<float>(g_current_position_seconds));
        std::string endTimeText = format_display_time(static_cast<float>(g_duration_seconds));
        CURRENT_TIME = std::wstring(currentTimeText.begin(), currentTimeText.end());
        END_TIME = std::wstring(endTimeText.begin(), endTimeText.end());

        HWND hCurrTimeCtrl = GetDlgItem(g_hwnd, ID_STATIC_CURR_TIME);
        HWND hEndTimeCtrl = GetDlgItem(g_hwnd, ID_STATIC_END_TIME);
        HWND hSongCtrl = GetDlgItem(g_hwnd, ID_STATIC_SONG);
        HWND hArtistCtrl = GetDlgItem(g_hwnd, ID_STATIC_ARTIST);

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

        setIfChanged(hCurrTimeCtrl, CURRENT_TIME);
        setIfChanged(hEndTimeCtrl, END_TIME);
        bool titleChanged = setIfChanged(hSongCtrl, g_current_title);
        setIfChanged(hArtistCtrl, g_current_artist);

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

            // Clear the previous song's lyrics right away.
            lyrics_display::set_lines({});

            string title = media.title;
            string artist = media.artist;
            std::thread([title, artist]() {
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
                }
            }).detach();
        }

        const double window_position = (std::max)(0.0, static_cast<double>(media.position));
        const double window_duration = (std::max)(0.0, static_cast<double>(media.duration));

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
            return;

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
        const double elapsed_seconds = static_cast<double>(now - g_last_update_tick) / 1000.0;
        if (elapsed_seconds <= 0.0)
            return;

        if (g_is_playing)
        {
            g_current_position_seconds += elapsed_seconds;
            if (g_duration_seconds > 0.0)
                g_current_position_seconds = (std::min)(g_current_position_seconds, g_duration_seconds);
        }

        g_last_update_tick = now;
        update_controls();

        MediaSessionInfo media = get_media_session_info();
        if (!media.is_success)
            return;

        if (apply_media_state(media))
            update_controls();
    }
}