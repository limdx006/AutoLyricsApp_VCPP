#include <iostream>
#include <vector>
#include <windows.h>
#include <cstdio>
#include <array>

// Fetch media session information using Windows Media Control API
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>


// Namespace declarations for convenience
using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;


// Function declarations
string get_lyrics(const string& title, const string& artist);


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "**********************************************" << "\n";
    cout << "*            AutoLyrics App v1.0             *" << "\n";
    cout << "**********************************************" << "\n";

    winrt::init_apartment();

    auto session = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();

    if (session)
    {
        auto info = session.TryGetMediaPropertiesAsync().get();
        string title = winrt::to_string(info.Title());
        string artist = winrt::to_string(info.Artist());
        cout << "Now Playing: " << title << " by " << artist << std::endl;
        cout << "Fetching lyrics..." << "\n";
        string lyrics = get_lyrics(title, artist);
        cout << "Lyrics: \n" << lyrics;
    }
    else
    {
        cout << "No media session active." << std::endl;
    }

    cout << "**********************************************" << "\n";
    cout << "*                     End                    *" << "\n";
    cout << "**********************************************" << "\n";
    return 0;
}


std::string get_lyrics(const string& title,
                       const string& artist)
{
    string command = "python lyrics_fetcher.py \"" + title + "\" \"" + artist + "\"";

    array<char, 256> buffer;
    string result;

    FILE* pipe = _popen(command.c_str(), "r");

    if (!pipe)
        return "";

    while (fgets(buffer.data(), buffer.size(), pipe))
    {
        result += buffer.data();
    }

    _pclose(pipe);

    return result;
}
