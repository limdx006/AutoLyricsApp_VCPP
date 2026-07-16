#include <iostream>
#include <vector>
#include <windows.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;



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
        cout << "Now    Playing: " << title << " by " << artist << std::endl;
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