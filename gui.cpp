#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

// Window dimensions
constexpr int WINDOW_WIDTH  = 400;
constexpr int WINDOW_HEIGHT = 800;

// Header card layout
constexpr int CARD_MARGIN   = 15;   // gap between window edge and card
constexpr int CARD_LEFT     = CARD_MARGIN;
constexpr int CARD_TOP      = CARD_MARGIN;
constexpr int CARD_WIDTH    = WINDOW_WIDTH - (CARD_MARGIN * 2);
constexpr int CARD_HEIGHT   = 180;
constexpr int CARD_RADIUS   = 24;   // corner rounding

// Control IDs
#define ID_BTN_PTT          101   // Pin-To-Top
#define ID_BTN_REFRESH       102
#define ID_BTN_SETTINGS      103
#define ID_STATIC_SONG       104
#define ID_STATIC_ARTIST     105
#define ID_STATIC_OFFSET_LBL 106
#define ID_BTN_OFFSET_MINUS  107
#define ID_EDIT_OFFSET       108
#define ID_BTN_OFFSET_PLUS   109

// Hardcoded display text (temporary)
static const wchar_t* SONG_NAME    = L"Song Name Goes Here";
static const wchar_t* ARTIST_NAME  = L"Unknown Artist";
static const wchar_t* OFFSET_VALUE = L"0.3";

// Globals
static HBRUSH g_hbrBackground = nullptr;   // main window bg: #1a1a2e
static HBRUSH g_hbrCard       = nullptr;   // card bg:        #16213e
static HBRUSH g_hbrEditBg     = nullptr;   // offset value box: white
static HFONT  g_hFontSong     = nullptr;
static HFONT  g_hFontArtist   = nullptr;
static HFONT  g_hFontIcon     = nullptr;
static HFONT  g_hFontIconLarge = nullptr;
static HFONT  g_hFontLabel    = nullptr;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateHeaderControls(HWND parent, HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"AutoLyricsWindowClass";

    g_hbrBackground = CreateSolidBrush(RGB(0x1a, 0x1a, 0x2e));
    g_hbrCard       = CreateSolidBrush(RGB(0x16, 0x21, 0x3e));
    g_hbrEditBg     = CreateSolidBrush(RGB(0xff, 0xff, 0xff));

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = g_hbrBackground;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc))
        return 0;

    // CreateWindowExW's width/height include the window frame (title bar,
    // borders). AdjustWindowRect expands a desired client-area size so the
    // inside of the window ends up exactly WINDOW_WIDTH x WINDOW_HEIGHT.
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"AutoLyrics",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 0;

    // Explicitly (re)set the title after creation. On some Windows 11
    // builds, the DWM-composited title bar doesn't reliably pick up the
    // full lpWindowName string passed to CreateWindowExW at creation time.
    SetWindowTextW(hwnd, L"AutoLyrics");

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void CreateHeaderControls(HWND parent, HINSTANCE hInstance)
{
    // Fonts
    g_hFontSong = CreateFontW(
        30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    g_hFontArtist = CreateFontW(
        15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Icon glyphs for the pin label (kept smaller, near the song name)
    g_hFontIcon = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");

    // Larger icon glyphs for REF / ST in the footer row
    g_hFontIconLarge = CreateFontW(
        40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");

    // Offset label, +/- buttons, and the value text all share this font
    g_hFontLabel = CreateFontW(
        25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Pin-To-Top: a plain SS_NOTIFY static label instead of a BUTTON.
    // STATIC controls have no border/face of their own, so this gives a
    // borderless, icon-only look while still receiving click notifications
    // (STN_CLICKED) through WM_COMMAND, same as a real button would.
    // Positioned a bit lower so it sits roughly level with the song name.
    HWND hBtnPTT = CreateWindowW(
        L"STATIC", L"\u2606",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - 40, CARD_TOP + 10, 28, 28,
        parent, (HMENU)ID_BTN_PTT, hInstance, nullptr);
    SendMessageW(hBtnPTT, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);

    // Song name, up to 2 lines, centered
    HWND hStaticSong = CreateWindowW(
        L"STATIC", SONG_NAME,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        CARD_LEFT + 20, CARD_TOP + 10, CARD_WIDTH - 40, 56,
        parent, (HMENU)ID_STATIC_SONG, hInstance, nullptr);
    SendMessageW(hStaticSong, WM_SETFONT, (WPARAM)g_hFontSong, TRUE);

    // Artist name, centered, below song block
    HWND hStaticArtist = CreateWindowW(
        L"STATIC", ARTIST_NAME,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        CARD_LEFT + 20, CARD_TOP + 56 + 6, CARD_WIDTH - 40, 22,
        parent, (HMENU)ID_STATIC_ARTIST, hInstance, nullptr);
    SendMessageW(hStaticArtist, WM_SETFONT, (WPARAM)g_hFontArtist, TRUE);

    // Footer row: REF | Offset: - 0.3 + | ST
    // Every control in this row is a different size, so instead of giving
    // them all the same y, we pick one horizontal center line for the row
    // and place each control so its own vertical center sits on that line.
    int rowCenterY = CARD_TOP + CARD_HEIGHT - 40;

    int iconSize = 44;   // REF / ST box size
    int refY = rowCenterY - iconSize / 2;

    // Refresh: borderless icon label, far left of card
    HWND hBtnRefresh = CreateWindowW(
        L"STATIC", L"\u21BB",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + 10, refY, iconSize, iconSize,
        parent, (HMENU)ID_BTN_REFRESH, hInstance, nullptr);
    SendMessageW(hBtnRefresh, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);

    // Settings: borderless icon label, far right of card
    HWND hBtnSettings = CreateWindowW(
        L"STATIC", L"\u2699",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - 10 - iconSize, refY, iconSize, iconSize,
        parent, (HMENU)ID_BTN_SETTINGS, hInstance, nullptr);
    SendMessageW(hBtnSettings, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);

    // Offset cluster, centered within the card. Buttons stay as real
    // BUTTON controls so the visible border stays, matching the reference.
    int labelW = 60, labelH = 28;
    int btnSize = 28;
    int editW = 40, editH = 25;
    int clusterWidth = labelW + 30 + btnSize + editW + btnSize;
    int clusterX = CARD_LEFT + (CARD_WIDTH - clusterWidth) / 2;
    int cx = clusterX;

    HWND hLblOffset = CreateWindowW(
        L"STATIC", L"Offset:",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        cx, rowCenterY - labelH / 2, labelW, labelH,
        parent, (HMENU)ID_STATIC_OFFSET_LBL, hInstance, nullptr);
    SendMessageW(hLblOffset, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += labelW + 5;

    HWND hBtnMinus = CreateWindowW(
        L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        cx, rowCenterY - btnSize / 2, btnSize, btnSize,
        parent, (HMENU)ID_BTN_OFFSET_MINUS, hInstance, nullptr);
    SendMessageW(hBtnMinus, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += btnSize + 2;

    HWND hEditOffset = CreateWindowW(
        L"EDIT", OFFSET_VALUE,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_READONLY,
        cx, rowCenterY - editH / 2, editW, editH,
        parent, (HMENU)ID_EDIT_OFFSET, hInstance, nullptr);
    SendMessageW(hEditOffset, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += editW + 2;

    HWND hBtnPlus = CreateWindowW(
        L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        cx, rowCenterY - btnSize / 2, btnSize, btnSize,
        parent, (HMENU)ID_BTN_OFFSET_PLUS, hInstance, nullptr);
    SendMessageW(hBtnPlus, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HINSTANCE hInstance = ((LPCREATESTRUCTW)lParam)->hInstance;
            CreateHeaderControls(hwnd, hInstance);
            return 0;
        }

        // Paint the rounded header card (#16213e) near the top of the window.
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, g_hbrCard);
            HPEN   nullPen  = (HPEN)GetStockObject(NULL_PEN);
            HPEN   oldPen   = (HPEN)SelectObject(hdc, nullPen);

            RoundRect(hdc,
                CARD_LEFT, CARD_TOP,
                CARD_LEFT + CARD_WIDTH, CARD_TOP + CARD_HEIGHT,
                CARD_RADIUS, CARD_RADIUS);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            EndPaint(hwnd, &ps);
            return 0;
        }

        // STATIC control text/background differs per control: song name
        // gets the accent color, icon labels and artist get lighter tones,
        // all sit on the card so the background is always the card brush.
        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            int ctrlId = GetDlgCtrlID((HWND)lParam);

            SetBkMode(hdcStatic, TRANSPARENT);

            if (ctrlId == ID_STATIC_SONG)
                SetTextColor(hdcStatic, RGB(0xe0, 0x5a, 0x6e)); // accent pink/red
            else if (ctrlId == ID_STATIC_ARTIST)
                SetTextColor(hdcStatic, RGB(0xa0, 0xa0, 0xb0)); // muted gray
            else
                SetTextColor(hdcStatic, RGB(0xf0, 0xf0, 0xf0)); // white icons/labels

            return (LRESULT)g_hbrCard;
        }

        // EDIT control (the "0.3" offset value box): white background,
        // dark text, matching the screenshot's white pill.
        case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(0x1a, 0x1a, 0x2e));
            SetBkColor(hdcEdit, RGB(0xff, 0xff, 0xff));
            return (LRESULT)g_hbrEditBg;
        }

        case WM_COMMAND:
        {
            // All buttons/icon labels are wired up but intentionally do
            // nothing yet. STN_CLICKED and BN_CLICKED are both notification
            // code 0, so this single switch catches clicks from either.
            switch (LOWORD(wParam))
            {
                case ID_BTN_PTT:
                case ID_BTN_REFRESH:
                case ID_BTN_SETTINGS:
                case ID_BTN_OFFSET_MINUS:
                case ID_BTN_OFFSET_PLUS:
                    // TODO: hook up functionality later
                    break;
            }
            return 0;
        }

        case WM_DESTROY:
            if (g_hbrBackground) { DeleteObject(g_hbrBackground); g_hbrBackground = nullptr; }
            if (g_hbrCard)       { DeleteObject(g_hbrCard);       g_hbrCard = nullptr; }
            if (g_hbrEditBg)     { DeleteObject(g_hbrEditBg);     g_hbrEditBg = nullptr; }
            if (g_hFontSong)     { DeleteObject(g_hFontSong);     g_hFontSong = nullptr; }
            if (g_hFontArtist)   { DeleteObject(g_hFontArtist);   g_hFontArtist = nullptr; }
            if (g_hFontIcon)     { DeleteObject(g_hFontIcon);     g_hFontIcon = nullptr; }
            if (g_hFontIconLarge) { DeleteObject(g_hFontIconLarge); g_hFontIconLarge = nullptr; }
            if (g_hFontLabel)    { DeleteObject(g_hFontLabel);    g_hFontLabel = nullptr; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}