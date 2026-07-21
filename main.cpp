#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/lyrics_fetcher.h" // To fetch lyrics using the Python script
#include "components/gui.h" // To launch the Win32 GUI window
#include "components/time_formatter.h" // To get formatted time from seconds to min and second

#include <windows.h>

#include <nlohmann/json.hpp> // For JSON parsing

// Namespace declarations for convenience
using json = nlohmann::json;


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "**********************************************" << "\n";
    cout << "*            AutoLyrics App v1.0             *" << "\n";
    cout << "**********************************************" << "\n";


    MediaSessionInfo media = get_media_session_info();

    if (media.is_success)
    {
        cout << "Now Playing: " << media.title << " by " << media.artist << "\n";
        cout << "Fetching lyrics..." << "\n";

        string currentTimeText = format_display_time(media.position);
        string endTimeText = format_display_time(media.duration);

        CURRENT_TIME = wstring(currentTimeText.begin(), currentTimeText.end());
        END_TIME = wstring(endTimeText.begin(), endTimeText.end());


        cout << "Playing : " << media.is_playing << "\n";

        // string response = get_lyrics(media.title, media.artist);
        // json j = json::parse(response);
        // if (j["success"]) {
        //     string lyrics = j["lyrics"];
        //     cout << "Lyrics:\n" << lyrics << std::"\n";
        // }
        // } else {
        //     cout << "No media session active." << std::"\n";
        // }

    cout << "Launching GUI..." << "\n";
    int guiExitCode = RunGui(GetModuleHandle(nullptr), SW_SHOWNORMAL);

        cout << "GUI exit code: " << guiExitCode << "\n";
        cout << "**********************************************" << "\n";
        cout << "*                     End                    *" << "\n";
        cout << "**********************************************" << "\n";
        return 0;
    }
}

