#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/lyrics_fetcher.h" // To fetch lyrics using the Python script
#include "components/gui.h" // To launch the Win32 GUI window
#include "components/time_formatter.h" // To get formatted time from seconds to min and second
#include "components/auto_nudge.h"
#include "components/timeline_tracker.h"

#include <windows.h>
#include <thread>
#include <cstdio>


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "**********************************************" << "\n";
    cout << "*            AutoLyrics App v1.0             *" << "\n";
    cout << "**********************************************" << "\n";

    auto_nudge();

    MediaSessionInfo media = get_media_session_info();

    if (media.is_success)
    {
        // Lyrics search runs on its own thread so it doesn't block the GUI
        // from opening: RunGui() below runs the message loop and doesn't
        // return until the window closes, so both happen at the same time.
        std::thread lyricsThread([media]() {
            cout << "Fetching lyrics..." << "\n";
            LyricsResult result = fetch_lyrics(media.title, media.artist);

            if (!result.success)
            {
                cout << "No synced lyrics found." << "\n";
                return;
            }

            cout << "Lyrics found (" << result.lines.size() << " lines):" << "\n";
            for (const LyricLine& line : result.lines)
            {
                int mins = (int)(line.timestamp / 60.0f);
                float secs = line.timestamp - mins * 60.0f;
                wchar_t stamp[16];
                swprintf(stamp, 16, L"%02d:%05.2f", mins, secs);
                wcout << L"[" << stamp << L"] " << line.text << L"\n";
            }

            SubmitLyrics(std::move(result.lines));
        });
        lyricsThread.detach();

        cout << "Launching GUI..." << "\n";
        int guiExitCode = RunGui(GetModuleHandle(nullptr), SW_SHOWNORMAL);
        cout << "GUI exit code: " << guiExitCode << "\n";
    }
    else
    {
        cout << "No media session active." << "\n";
    }

    cout << "**********************************************" << "\n";
    cout << "*                     End                    *" << "\n";
    cout << "**********************************************" << "\n";

    return 0;
}