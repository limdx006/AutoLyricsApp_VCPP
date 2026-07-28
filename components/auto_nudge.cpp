#include "auto_nudge.h"
#include "log_viewer.h"
#include "playback_controls.h"

void auto_nudge(float sleep_delay)
{
    int attempts = 0;

    using playback_controls::PlaybackAction;

    const bool was_playing = playback_controls::is_playing();


    // Only nudge (brief pause/resume) when media is actively playing.
    // If already paused, do nothing — otherwise we'd resume playback.
    if (!was_playing)
        return;

    log_viewer::log("[NUDGE] Attempt auto nudge\n");
    do {
        playback_controls::send_action(PlaybackAction::PlayPause);

        if (sleep_delay > 0.0)
            Sleep(static_cast<DWORD>(sleep_delay * 1000.0));

        playback_controls::send_action(PlaybackAction::PlayPause);
        Sleep(2000);
        attempts++;

    } while(!was_playing);
    log_viewer::log("[NUDGE] Auto nudge success in: %d\n", attempts);
}