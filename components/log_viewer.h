#pragma once
#include "common.h"

// Captures terminal output into a ring buffer (max 50 entries) and
// provides a toggle-able child window that displays them, so the user
// can see diagnostic messages without a separate console.
namespace log_viewer {
    // Must be called once at startup (registers the window class).
    void initialize(HINSTANCE hInstance);

    // Frees window resources.
    void cleanup();

    // Logs a printf-style message: prints to the terminal AND stores it
    // in the ring buffer.  If the log window is open the message is also
    // appended there.
    void log(const char* format, ...);

    // Show / hide the log viewer window.
    void toggle_window(HWND parent);

    // Append any pending buffered messages to the edit control (called
    // from WM_SIZE or after window creation to catch up).
    void refresh_display();
}
