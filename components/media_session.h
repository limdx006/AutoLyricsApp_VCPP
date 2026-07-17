#pragma once
#include "common.h"

struct MediaSessionInfo {
    bool success = false;
    string title;
    string artist;
};

// Queries the Windows Global System Media Transport Controls
// for the currently playing media session.
MediaSessionInfo get_media_session_info();