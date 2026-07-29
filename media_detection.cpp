#include <iostream>
#include <string>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <windows.h>

using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    init_apartment();

    auto manager =
        GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

    auto sessions = manager.GetSessions();

    cout << "Found " << sessions.Size() << " media session(s).\n\n";

    int index = 1;

    for (auto const& session : sessions)
    {
        cout << "=============================\n";
        cout << "Session #" << index++ << "\n";
        cout << "=============================\n";

        auto info = session.TryGetMediaPropertiesAsync().get();
        auto playback = session.GetPlaybackInfo();
        auto timeline = session.GetTimelineProperties();

        cout << "App      : "
             << to_string(session.SourceAppUserModelId()) << '\n';

        cout << "Title    : "
             << to_string(info.Title()) << '\n';

        cout << "Artist   : "
             << to_string(info.Artist()) << '\n';

        cout << "Album    : "
             << to_string(info.AlbumTitle()) << '\n';

        cout << "Position : "
             << timeline.Position().count() / 10000000.0
             << " sec\n";

        cout << "Duration : "
             << timeline.EndTime().count() / 10000000.0
             << " sec\n";

        cout << "Status   : ";

        switch (playback.PlaybackStatus())
        {
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
            cout << "Playing";
            break;

        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
            cout << "Paused";
            break;

        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
            cout << "Stopped";
            break;

        default:
            cout << "Other";
            break;
        }

        cout << "\n\n";
    }

    system("pause");
    return 0;
}