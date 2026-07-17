#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/lyrics_fetcher.h" // To fetch lyrics using the Python script

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

    if (media.success)
    {
        cout << "Now Playing: " << media.title << " by " << media.artist << endl;
        cout << "Fetching lyrics..." << "\n";
 
        string response = get_lyrics(media.title, media.artist);
        json j = json::parse(response);
        if (j["success"]) {
            string lyrics = j["lyrics"];
            cout << "Lyrics:\n" << lyrics << std::endl;
        }
    } else {
        cout << "No media session active." << std::endl;
    }

    cout << "**********************************************" << "\n";
    cout << "*                     End                    *" << "\n";
    cout << "**********************************************" << "\n";
    return 0;
}
