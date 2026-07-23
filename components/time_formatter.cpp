#include "time_formatter.h"


string format_display_time(float total_seconds){
    int round_time = static_cast<int>(round(total_seconds));
    int mins =  round_time / 60;
    int secs =  round_time % 60;
    if(secs < 10)
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