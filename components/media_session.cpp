#include "media_session.h"

// Fetch media session information using Windows Media Control API
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt;
using namespace winrt::Windows::Media::Control;

MediaSessionInfo get_media_session_info() {
    MediaSessionInfo result; // result.success defaults to false

    winrt::init_apartment();

    auto session = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();

    if (session)
    {
        auto info = session.TryGetMediaPropertiesAsync().get();
        result.title = to_string(info.Title());
        result.artist = to_string(info.Artist());
        result.success = true;
    }
    else
    {
        cout << "No media session active." << std::endl;
    }
    return result;
}   