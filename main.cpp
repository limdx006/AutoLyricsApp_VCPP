#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/gui.h" // To launch the Win32 GUI window
#include "components/auto_nudge.h"
#include "components/timeline_tracker.h"

#include <windows.h>


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
        // Lyrics fetching is handled inside the GUI itself
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