#include "media_session.h"
#include "time_formatter.h"

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt;
using namespace winrt::Windows::Media::Control;

MediaSessionInfo get_media_session_info()
{
    MediaSessionInfo result;

    static bool initialized = false;
    if (!initialized)
    {
        winrt::init_apartment();
        initialized = true;
    }

    auto manager =
        GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

    auto session = manager.GetCurrentSession();

    if (!session)
        return result;

    auto info = session.TryGetMediaPropertiesAsync().get();

    result.title = to_string(info.Title());
    result.artist = to_string(info.Artist());

    // Playback information
    auto playback = session.GetPlaybackInfo();

    result.is_playing =
        playback.PlaybackStatus() ==
        GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;

    // Timeline information
    auto timeline = session.GetTimelineProperties();

    result.position =
        timeline.Position().count() / 10000000.0;

    result.duration =
        timeline.EndTime().count() / 10000000.0;

    result.is_success = true;

    return result;
}