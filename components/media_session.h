#pragma once
#include "common.h"

struct MediaSessionInfo {
    bool is_success = false;

    string title;
    string artist;

    float position;
    float duration;

    bool is_playing = false;
};

// Queries the Windows Global System Media Transport Controls
// for the currently playing media session.
MediaSessionInfo get_media_session_info();