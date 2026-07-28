#include "components/common.h"
#include "components/media_session.h" // To get media session info
#include "components/gui.h" // To launch the Win32 GUI window
#include "components/auto_nudge.h"
#include "components/timeline_tracker.h"
#include "components/log_viewer.h"

#include <windows.h>


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    log_viewer::log("**********************************************\n");
    log_viewer::log("*            AutoLyrics App v1.0             *\n");
    log_viewer::log("**********************************************\n");

    auto_nudge();

    MediaSessionInfo media = get_media_session_info();

    if (!media.is_success)
        log_viewer::log("Warning: No media session active.\n");

    log_viewer::log("Launching GUI...\n");
    int guiExitCode = RunGui(GetModuleHandle(nullptr), SW_SHOWNORMAL);
    log_viewer::log("GUI exit code: %d\n", guiExitCode);

    log_viewer::log("**********************************************\n");
    log_viewer::log("*                     End                    *\n");
    log_viewer::log("**********************************************\n");

    return 0;
}