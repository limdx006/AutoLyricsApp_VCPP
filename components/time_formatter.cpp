#include "time_formatter.h"


string format_display_time(float total_seconds){
    int round_time = static_cast<int>(round(total_seconds));
    int mins =  round_time / 60;
    int secs =  round_time % 60;
    if(secs < 10)
        return to_string(mins) + ":0" + to_string(secs);
    return to_string(mins) + ":" + to_string(secs);
}