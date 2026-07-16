#include <iostream>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;

// Window default settings
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 700;
const string BG_COLOR = "#1a1a2e";
const string ACCENT_COLOR = "#16213e";

// Global variables
string current_title = "";
string current_artist = "";
double song_duration = 0;
double last_system_position = 0;  // Position at time of last sync
double last_sync_time = 0;  // perf_counter() at time of last sync
double max_seen_position = 0;  // What our local timer calculated at last sync
double last_accepted_system_pos = 0;  // Track last position we actually accepted
// Pause tracking
bool is_paused = false;
double paused_position = 0;
double pause_start_time = 0;
bool is_initialized = false;
// Lyrics tracking
vector<pair<double, string>> lyrics_lines;  // List of (timestamp_seconds, text) tuples
int current_lyric_index = -1;


int main() {
    cout << "**********************************************" << endl;
    cout << "*            AutoLyrics App v1.0             *" << endl;
    cout << "**********************************************" << endl;

    winrt::init_apartment();

    // Equivalent of: sessions = await MediaManager.request_async()
    auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

    // Equivalent of: current_session = sessions.get_current_session()
    auto session = manager.GetCurrentSession();

    if (session)
    {
        // Equivalent of: info = await current_session.try_get_media_properties_async()
        auto info = session.TryGetMediaPropertiesAsync().get();

        std::wcout << L"Now Playing: " << info.Title().c_str()
                    << L" by " << info.Artist().c_str() << std::endl;
    }
    else
    {
        std::wcout << L"No media session active." << std::endl;
    }

    return 0;
}