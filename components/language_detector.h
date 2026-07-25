#pragma once
#include "common.h"

// Languages the detector can identify.
enum class Language {
    English,
    Chinese,
    Japanese,
    Korean,
    Unknown
};

// Analyzes Unicode character ranges across lyric text lines to determine the most likely language.
Language detect_language(const vector<wstring>& lyricLines);

// Convert detected language to its display label.
const wchar_t* language_to_wstring(Language lang);
