#pragma once
#include "common.h"

string format_display_time(float total_seconds);

// Parses an LRC timestamp like "01:23.45" (mm:ss.cc, no brackets) into total
// seconds. Returns -1 for anything that isn't a valid mm:ss(.cc) timestamp
// (e.g. LRC metadata tags like "ar:Some Artist").
float parse_lrc_time(const string& time_str);