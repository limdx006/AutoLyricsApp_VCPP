#pragma once
#include "common.h"

struct MediaSessionInfo {
    bool is_success = false;

    string title;
    string artist;
    string album_title;       // Album title (used by scoring)
    string app_id;            // SourceAppUserModelId, e.g. "Spotify.exe", "chrome.exe"

    double position = 0;
    double duration = 0;

    bool is_playing = false;
    bool has_thumbnail = false;   // Whether the session has album art

    int score = 0;                // Confidence score from media_selector
};

// Queries the Windows Global System Media Transport Controls
// for the currently playing media session.
MediaSessionInfo get_media_session_info();