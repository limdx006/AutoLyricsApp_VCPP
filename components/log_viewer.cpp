#include "log_viewer.h"
#include <cstdio>
#include <cstdarg>
#include <mutex>

namespace log_viewer {
    // Ring buffer
    static const int MAX_LOG = 50;
    static string s_buffer[MAX_LOG];
    static int s_writePos = 0;
    static int s_count = 0;
    static std::mutex s_mutex;

    // Window state
    static HWND s_hwndLog = nullptr;   // the log viewer frame window
    static HWND s_hwndEdit = nullptr;  // the read-only edit control inside it
    static HINSTANCE s_hInstance = nullptr;

    static const wchar_t* const CLASS_NAME = L"AutoLyricsLogViewer";

    // Font used for the edit control (monospace).
    static HFONT s_hFont = nullptr;

    // Internal helpers

    // Append a single line to the edit control (called on the UI thread).
    static void append_to_edit(const char* text)
    {
        if (!s_hwndEdit) return;

        int len = GetWindowTextLengthW(s_hwndEdit);
        wstring wtext = utf8_to_wide(text);

        SendMessageW(s_hwndEdit, EM_SETSEL, len, len);
        SendMessageW(s_hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)wtext.c_str());

        // Scroll to the bottom so newest text is always visible.
        SendMessageW(s_hwndEdit, EM_SCROLLCARET, 0, 0);
    }

    // Repopulate the edit control from the ring buffer.
    static void populate_edit()
    {
        if (!s_hwndEdit) return;

        SetWindowTextW(s_hwndEdit, L"");

        std::lock_guard<std::mutex> lock(s_mutex);

        // Read oldest-to-newest.
        int start = s_count < MAX_LOG ? 0 : s_writePos;
        for (int i = 0; i < s_count; ++i)
        {
            int idx = (start + i) % MAX_LOG;
            wstring wline = utf8_to_wide(s_buffer[idx]);
            SendMessageW(s_hwndEdit, EM_SETSEL, -1, -1);
            SendMessageW(s_hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)wline.c_str());
        }
    }

    // Window procedure

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
            case WM_CREATE:
            {
                // Create a read-only multi-line edit control that fills the client area.
                s_hwndEdit = CreateWindowW(
                    L"EDIT", nullptr,
                    WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
                        ES_AUTOVSCROLL | WS_VSCROLL | ES_LEFT,
                    0, 0, 0, 0,
                    hwnd, nullptr, s_hInstance, nullptr);

                if (!s_hFont)
                {
                    s_hFont = CreateFontW(
                        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
                }
                SendMessageW(s_hwndEdit, WM_SETFONT, (WPARAM)s_hFont, TRUE);

                // Load existing log messages.
                populate_edit();
                return 0;
            }

            case WM_SIZE:
            {
                if (s_hwndEdit)
                {
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    SetWindowPos(s_hwndEdit, nullptr, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
                }
                return 0;
            }

            case WM_CLOSE:
                // Hide instead of destroy — preserve content across toggles.
                ShowWindow(hwnd, SW_HIDE);
                return 0;

            case WM_DESTROY:
                s_hwndLog = nullptr;
                s_hwndEdit = nullptr;
                return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // Public API 

    void initialize(HINSTANCE hInstance)
    {
        s_hInstance = hInstance;

        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;
        RegisterClassW(&wc);
    }

    void cleanup()
    {
        if (s_hwndLog)
        {
            DestroyWindow(s_hwndLog);
            s_hwndLog = nullptr;
            s_hwndEdit = nullptr;
        }
        if (s_hFont)
        {
            DeleteObject(s_hFont);
            s_hFont = nullptr;
        }
    }

    void log(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        char buf[4096];
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);

        // Always go to the terminal so the console user sees it.
        fprintf(stdout, "%s", buf);
        fflush(stdout);

        // Store in the ring buffer.
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_buffer[s_writePos] = buf;
            s_writePos = (s_writePos + 1) % MAX_LOG;
            if (s_count < MAX_LOG)
                ++s_count;
        }

        // If the log window is open, append immediately.
        if (s_hwndLog && IsWindowVisible(s_hwndLog))
            append_to_edit(buf);
    }

    void toggle_window(HWND parent)
    {
        if (s_hwndLog && IsWindowVisible(s_hwndLog))
        {
            ShowWindow(s_hwndLog, SW_HIDE);
            return;
        }

        if (!s_hwndLog)
        {
            s_hwndLog = CreateWindowW(
                CLASS_NAME, L"Log",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 680, 420,
                parent, nullptr, s_hInstance, nullptr);
        }
        else
        {
            ShowWindow(s_hwndLog, SW_SHOW);
            SetForegroundWindow(s_hwndLog);
            populate_edit();
        }
    }

    void refresh_display()
    {
        populate_edit();
    }
}
