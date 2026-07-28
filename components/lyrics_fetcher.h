#pragma once
#include "common.h"

// One timestamped line of lyrics, in seconds from the start of the track.
struct LyricLine {
    float timestamp;
    wstring text;
};

struct LyricsResult {
    bool success = false;      // true only if synced (timestamped) lines were found
    vector<LyricLine> lines;
};

// Calls out to lyrics_fetcher.py (via TRACK_TITLE / TRACK_ARTIST env vars)
// and returns the raw JSON response string it prints to stdout.
string get_lyrics(const string& title, const string& artist);

// Same as get_lyrics(), but parses the JSON response and splits the LRC
// text it contains into timestamped lines ready for display.
LyricsResult fetch_lyrics(const string& title, const string& artist);

// Calls lyrics_translator.py to romanize/translate the given lyric text
// lines according to the language code ("ja", "ko", "zh", "en").
// Returns the translated texts as UTF-16 strings, or an empty vector on failure.
vector<wstring> translate_lyrics(const vector<wstring>& texts, const string& langCode); 