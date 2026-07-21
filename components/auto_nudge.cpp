#include "auto_nudge.h"
#include "playback_controls.h"

void auto_nudge(float sleep_delay)
{
    using playback_controls::PlaybackAction;

    const bool was_playing = playback_controls::is_playing();

    if (was_playing)
    {
        playback_controls::send_action(PlaybackAction::PlayPause);

        if (sleep_delay > 0.0)
            Sleep(static_cast<DWORD>(sleep_delay * 1000.0));

        playback_controls::send_action(PlaybackAction::PlayPause);
    }
    else
    {
        playback_controls::send_action(PlaybackAction::PlayPause);
    }
}