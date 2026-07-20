#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/lyrics_fetcher.h" // To fetch lyrics using the Python script
#include "components/gui.h" // To launch the Win32 GUI window

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
        cout << "Now Playing: " << media.title << " by " << media.artist << endl;
        cout << "Fetching lyrics..." << "\n";

        cout << "Position : " << media.position << endl;
        cout << "Duration : " << media.duration << endl;
        
        cout << "Playing  : " << media.is_playing << endl;

        string response = get_lyrics(media.title, media.artist);
        json j = json::parse(response);
        if (j["success"]) {
            string lyrics = j["lyrics"];
            cout << "Lyrics:\n" << lyrics << std::endl;
        }
    } else {
        cout << "No media session active." << std::endl;
    }

    cout << "Launching GUI..." << endl;
    int guiExitCode = RunGui(GetModuleHandle(nullptr), SW_SHOWNORMAL);

    cout << "GUI exit code: " << guiExitCode << endl;
    cout << "**********************************************" << "\n";
    cout << "*                     End                    *" << "\n";
    cout << "**********************************************" << "\n";
    return 0;
}
