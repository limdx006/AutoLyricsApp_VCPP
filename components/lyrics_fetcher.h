#pragma once
#include "common.h"

// Calls out to lyrics_fetcher.py (via TRACK_TITLE / TRACK_ARTIST env vars)
// and returns the raw JSON response string it prints to stdout.
string get_lyrics(const string& title, const string& artist);