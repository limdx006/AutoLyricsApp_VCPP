#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/lyrics_fetcher.h" // To fetch lyrics using the Python script
#include "components/gui.h" // To launch the Win32 GUI window
#include "components/time_formatter.h" // To get formatted time from seconds to min and second
#include "components/auto_nudge.h"
#include "components/timeline_tracker.h"

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

    auto_nudge();
    cout << "Auto nudged" << "\n";

    MediaSessionInfo media = get_media_session_info();

    if (media.is_success)
    {
        cout << "Playing : " << media.is_playing << "\n";
        cout << "Now Playing: " << media.title << " by " << media.artist << "\n";

        cout << "Launching GUI..." << "\n";
        int guiExitCode = RunGui(GetModuleHandle(nullptr), SW_SHOWNORMAL);
        cout << "GUI exit code: " << guiExitCode << "\n";

        cout << "Fetching lyrics..." << "\n";
        // string response = get_lyrics(media.title, media.artist);
        // json j = json::parse(response);
        // if (j["success"]) {
        //     string lyrics = j["lyrics"];
        //     cout << "Lyrics:\n" << lyrics << std::"\n";
        // }
        // } else {
        //     cout << "No media session active." << std::"\n";
        // }
    }
    
    cout << "**********************************************" << "\n";
    cout << "*                     End                    *" << "\n";
    cout << "**********************************************" << "\n";
    return 0;
}

