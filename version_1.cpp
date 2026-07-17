#include <iostream>
#include <vector>
#include <windows.h>
#include <cstdio>
#include <array>

// Fetch media session information using Windows Media Control API
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

#include <nlohmann/json.hpp> // For JSON parsing

// Namespace declarations for convenience
using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;
using json = nlohmann::json;

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
        string title = to_string(info.Title());
        string artist = to_string(info.Artist());
        cout << "Now Playing: " << title << " by " << artist << endl;
        cout << "Fetching lyrics..." << "\n";
        string response = get_lyrics(title, artist);
        json j = json::parse(response);
        if (j["success"]){
            string lyrics = j["lyrics"];
            cout << "Lyrics:\n" << lyrics << std::endl;
        }
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


std::string get_lyrics(const string& title, const string& artist)
{
    // Convert UTF-8 std::string -> UTF-16 for SetEnvironmentVariableW
    auto toWide = [](const string& s) -> wstring {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
        return w;
    };

    SetEnvironmentVariableW(L"TRACK_TITLE", toWide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", toWide(artist).c_str());

    string command = "python lyrics_fetcher.py";

    array<char, 256> buffer;
    string result;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe))
        result += buffer.data();

    _pclose(pipe);
    return result;
}
