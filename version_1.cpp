#include <iostream>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
using namespace std;
using namespace winrt;
using namespace winrt::Windows::Media::Control;


int main() {
    wcout << L"**********************************************" << L"\n";
    wcout << L"*            AutoLyrics App v1.0             *" << L"\n";
    wcout << L"**********************************************" << L"\n";

    winrt::init_apartment();

    auto session = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();

    if (session)
    {
        auto info = session.TryGetMediaPropertiesAsync().get();

        wcout << L"Now Playing: " << info.Title().c_str() << " by " << info.Artist().c_str() << std::endl;
    }
    else
    {
        wcout << L"No media session active." << std::endl;
    }

    wcout << L"**********************************************" << L"\n";
    wcout << L"*                     End                    *" << L"\n";
    wcout << L"**********************************************" << L"\n";
    return 0;
}