#include "time_formatter.h"
#include <cmath>


string format_display_time(double total_seconds){
    // Custom rounding: round UP when fractional seconds > 0.4 (more
    // responsive display — the next minute:second shows slightly earlier).
    double frac = total_seconds - std::floor(total_seconds);
    int display_seconds;
    if (frac > 0.4)
        display_seconds = static_cast<int>(std::ceil(total_seconds));
    else
        display_seconds = static_cast<int>(std::floor(total_seconds));

    int mins  = display_seconds / 60;
    int secs  = display_seconds % 60;
    if (secs < 10)
        return to_string(mins) + ":0" + to_string(secs);
    return to_string(mins) + ":" + to_string(secs);
}

float parse_lrc_time(const string& time_str){
    size_t colon = time_str.find(':');
    if (colon == string::npos)
        return -1.0f;

    try
    {
        int minutes = std::stoi(time_str.substr(0, colon));
        float seconds = std::stof(time_str.substr(colon + 1));
        return minutes * 60.0f + seconds;
    }
    catch (...)
    {
        return -1.0f; // not a numeric mm:ss(.cc) tag
    }
}