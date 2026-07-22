#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "gui.h"
#include "timeline_tracker.h"

// Globals
static HBRUSH g_hbrBackground = nullptr;
static HBRUSH g_hbrCard        = nullptr;
static HBRUSH g_hbrEditBg      = nullptr;
static HBRUSH g_hbrProgressBar = nullptr;
static HFONT  g_hFontSong      = nullptr;
static HFONT  g_hFontArtist    = nullptr;
static HFONT  g_hFontIcon      = nullptr;
static HFONT  g_hFontIconLarge = nullptr;
static HFONT  g_hFontLabel     = nullptr;
static bool   g_isPinned       = false;
static HFONT  g_hFontTime      = nullptr;
static HFONT  g_hFontStatus    = nullptr;
static HFONT  g_hFontLang      = nullptr;   // language bar font

// Hover state for header icon labels
static bool g_hoverPTT      = false;
static bool g_hoverRefresh  = false;
static bool g_hoverSettings = false;
// Hover state for playback control icons
static bool g_hoverPrev     = false;
static bool g_hoverPlayPause = false;
static bool g_hoverNext     = false;

// true = playing (shows pause button), false = paused (shows play button)
static bool g_isPlaying = true;

static void UpdatePlayPauseButton(HWND hwnd)
{
    HWND hBtn = GetDlgItem(hwnd, ID_BTN_PLAY_PAUSE);
    if (hBtn)
    {
        const wchar_t* newText = g_isPlaying ? L"\u23F8" : L"\u25B6";
        SetWindowTextW(hBtn, newText);
    }

    HWND hStatus = GetDlgItem(hwnd, ID_STATIC_STATUS);
    if (hStatus)
    {
        const wchar_t* newStatus = g_isPlaying ? L"Playing" : L"Paused";
        SetWindowTextW(hStatus, newStatus);
    }
}

// Shared subclass procedure for icon labels; dwRefData is that control's hover flag
LRESULT CALLBACK IconHoverSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    bool* hoverFlag = (bool*)dwRefData;

    switch (msg)
    {
        case WM_MOUSEMOVE:
            if (!*hoverFlag)
            {
                *hoverFlag = true;
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;

        case WM_MOUSELEAVE:
            *hoverFlag = false;
            InvalidateRect(hwnd, nullptr, TRUE);
            break;

        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, IconHoverSubclassProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

int RunGui(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"AutoLyricsWindowClass";

    g_hbrBackground = CreateSolidBrush(APP_COLOR_BACKGROUND);
    g_hbrCard       = CreateSolidBrush(APP_COLOR_CARD);
    g_hbrEditBg     = CreateSolidBrush(APP_COLOR_EDIT_BG);
    g_hbrProgressBar = CreateSolidBrush(APP_COLOR_PROGRESS_BAR);

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

    // AdjustWindowRect expands the desired client size to account for the window frame
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    // WS_CLIPCHILDREN stops the parent painting over child controls; no WS_THICKFRAME/WS_MAXIMIZEBOX keeps the window a fixed size
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
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

    // Re-set the title; some Windows 11 builds don't pick it up from CreateWindowExW alone
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

// Re-measures the song text and resizes/repositions the song + artist boxes to fit
void RefreshHeaderText(HWND parent, const wstring& songTitle)
{
    HWND hStaticSong = GetDlgItem(parent, ID_STATIC_SONG);
    HWND hStaticArtist = GetDlgItem(parent, ID_STATIC_ARTIST);
    if (!hStaticSong || !hStaticArtist)
        return;

    int songX = CARD_LEFT + SIDE_RESERVED;
    int songY = CARD_TOP + SONG_TOP_OFFSET;
    int songWidth = CARD_WIDTH - (SIDE_RESERVED * 2);

    RECT calcRect = { 0, 0, songWidth, 0 };
    HDC hdc = GetDC(parent);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSong);
    DrawTextW(hdc, songTitle.c_str(), -1, &calcRect, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
    SelectObject(hdc, oldFont);
    ReleaseDC(parent, hdc);
    int songHeight = calcRect.bottom - calcRect.top;

    MoveWindow(hStaticSong, songX, songY, songWidth, songHeight, TRUE);

    int artistY = songY + songHeight + ARTIST_GAP;
    MoveWindow(hStaticArtist, CARD_LEFT + 20, artistY, CARD_WIDTH - 40, 22, TRUE);
}

void CreateHeaderControls(HWND parent, HINSTANCE hInstance)
{
    g_hFontSong = CreateFontW(
        FONT_SIZE_SONG, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    g_hFontArtist = CreateFontW(
        FONT_SIZE_ARTIST, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    g_hFontIcon = CreateFontW(
        FONT_SIZE_ICON, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_SYMBOL);

    g_hFontIconLarge = CreateFontW(
        FONT_SIZE_ICON_LARGE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_SYMBOL);

    g_hFontLabel = CreateFontW(
        FONT_SIZE_LABEL, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    // Inset by SIDE_RESERVED on both sides so the PTT icon has room and the text stays centered
    int songX = CARD_LEFT + SIDE_RESERVED;
    int songY = CARD_TOP + SONG_TOP_OFFSET;
    int songWidth = CARD_WIDTH - (SIDE_RESERVED * 2);

    wstring songTitle = timeline_tracker::get_current_title();
    wstring artistName = timeline_tracker::get_current_artist();

    // Measure the wrapped song text height so short/long titles both fit without overlapping the artist line
    RECT calcRect = { 0, 0, songWidth, 0 };
    HDC hdc = GetDC(parent);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSong);
    DrawTextW(hdc, songTitle.c_str(), -1, &calcRect, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
    SelectObject(hdc, oldFont);
    ReleaseDC(parent, hdc);
    int songHeight = calcRect.bottom - calcRect.top;

    HWND hStaticSong = CreateWindowW(
        L"STATIC", songTitle.c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        songX, songY, songWidth, songHeight,
        parent, (HMENU)ID_STATIC_SONG, hInstance, nullptr);
    SendMessageW(hStaticSong, WM_SETFONT, (WPARAM)g_hFontSong, TRUE);

    // Artist name starts right where the wrapped song text ends
    int artistY = songY + songHeight + ARTIST_GAP;
    HWND hStaticArtist = CreateWindowW(
        L"STATIC", artistName.c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        CARD_LEFT + 20, artistY, CARD_WIDTH - 40, 22,
        parent, (HMENU)ID_STATIC_ARTIST, hInstance, nullptr);
    SendMessageW(hStaticArtist, WM_SETFONT, (WPARAM)g_hFontArtist, TRUE);

    // Pin-to-top star, sits in the space reserved by SIDE_RESERVED
    HWND hBtnPTT = CreateWindowW(
        L"STATIC", L"\u2606",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - PTT_MARGIN - PTT_SIZE, songY, PTT_SIZE, PTT_SIZE,
        parent, (HMENU)ID_BTN_PTT, hInstance, nullptr);
    SendMessageW(hBtnPTT, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);
    SetWindowSubclass(hBtnPTT, IconHoverSubclassProc, 1, (DWORD_PTR)&g_hoverPTT);

    // Footer row (REF | Offset | ST): each control centers on this shared row line
    int rowCenterY = CARD_TOP + CARD_HEIGHT - FOOTER_ROW_OFFSET_FROM_BOTTOM;
    int refY = rowCenterY - FOOTER_ICON_SIZE / 2;

    HWND hBtnRefresh = CreateWindowW(
        L"STATIC", L"\u21BB",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + FOOTER_ICON_MARGIN, refY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE,
        parent, (HMENU)ID_BTN_REFRESH, hInstance, nullptr);
    SendMessageW(hBtnRefresh, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);
    SetWindowSubclass(hBtnRefresh, IconHoverSubclassProc, 2, (DWORD_PTR)&g_hoverRefresh);

    HWND hBtnSettings = CreateWindowW(
        L"STATIC", L"\u2699",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - FOOTER_ICON_MARGIN - FOOTER_ICON_SIZE, refY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE,
        parent, (HMENU)ID_BTN_SETTINGS, hInstance, nullptr);
    SendMessageW(hBtnSettings, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);
    SetWindowSubclass(hBtnSettings, IconHoverSubclassProc, 3, (DWORD_PTR)&g_hoverSettings);

    // Offset cluster, centered within the card
    int clusterWidth = OFFSET_LABEL_WIDTH + OFFSET_GAP_LABEL_TO_MINUS
                      + OFFSET_BTN_SIZE + OFFSET_GAP_MINUS_TO_EDIT
                      + OFFSET_EDIT_WIDTH + OFFSET_GAP_EDIT_TO_PLUS
                      + OFFSET_BTN_SIZE;
    int cx = CARD_LEFT + (CARD_WIDTH - clusterWidth) / 2;

    HWND hLblOffset = CreateWindowW(
        L"STATIC", L"Offset:",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        cx, rowCenterY - OFFSET_LABEL_HEIGHT / 2, OFFSET_LABEL_WIDTH, OFFSET_LABEL_HEIGHT,
        parent, (HMENU)ID_STATIC_OFFSET_LBL, hInstance, nullptr);
    SendMessageW(hLblOffset, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += OFFSET_LABEL_WIDTH + OFFSET_GAP_LABEL_TO_MINUS;

    HWND hBtnMinus = CreateWindowW(
        L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        cx, rowCenterY - OFFSET_BTN_SIZE / 2, OFFSET_BTN_SIZE, OFFSET_BTN_SIZE,
        parent, (HMENU)ID_BTN_OFFSET_MINUS, hInstance, nullptr);
    SendMessageW(hBtnMinus, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += OFFSET_BTN_SIZE + OFFSET_GAP_MINUS_TO_EDIT;

    HWND hEditOffset = CreateWindowW(
        L"EDIT", OFFSET_VALUE,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_READONLY,
        cx, rowCenterY - OFFSET_EDIT_HEIGHT / 2, OFFSET_EDIT_WIDTH, OFFSET_EDIT_HEIGHT,
        parent, (HMENU)ID_EDIT_OFFSET, hInstance, nullptr);
    SendMessageW(hEditOffset, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
    cx += OFFSET_EDIT_WIDTH + OFFSET_GAP_EDIT_TO_PLUS;

    HWND hBtnPlus = CreateWindowW(
        L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        cx, rowCenterY - OFFSET_BTN_SIZE / 2, OFFSET_BTN_SIZE, OFFSET_BTN_SIZE,
        parent, (HMENU)ID_BTN_OFFSET_PLUS, hInstance, nullptr);
    SendMessageW(hBtnPlus, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);
}

void CreateLanguageBarControls(HWND parent, HINSTANCE hInstance)
{
    // Language bar font: slightly smaller than artist name
    g_hFontLang = CreateFontW(
        FONT_SIZE_LANG, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    // 3 equal columns, each centered: Language | Current | Mode
    int colWidth = CARD_WIDTH / 3;

    int row1Y = LANG_BAR_TOP + LANG_BAR_ROW1_OFFSET;
    int row2Y = LANG_BAR_TOP + LANG_BAR_ROW2_OFFSET;
    int textHeight = FONT_SIZE_LANG + 4;  // small padding for clean vertical centering

    // --- Column 1: Language ---
    int col1X = CARD_LEFT;
    HWND hLangLabel = CreateWindowW(
        L"STATIC", LANG_LABEL_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col1X, row1Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_LANG_LABEL, hInstance, nullptr);
    SendMessageW(hLangLabel, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);

    HWND hLangValue = CreateWindowW(
        L"STATIC", LANG_VALUE_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col1X, row2Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_LANG_VALUE, hInstance, nullptr);
    SendMessageW(hLangValue, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);

    // --- Column 2: Current ---
    int col2X = CARD_LEFT + colWidth;
    HWND hCurrentLabel = CreateWindowW(
        L"STATIC", CURRENT_LABEL_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col2X, row1Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_CURRENT_LABEL, hInstance, nullptr);
    SendMessageW(hCurrentLabel, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);

    HWND hCurrentValue = CreateWindowW(
        L"STATIC", CURRENT_VALUE_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col2X, row2Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_CURRENT_VALUE, hInstance, nullptr);
    SendMessageW(hCurrentValue, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);

    // --- Column 3: Mode ---
    int col3X = CARD_LEFT + (colWidth * 2);
    HWND hModeLabel = CreateWindowW(
        L"STATIC", MODE_LABEL_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col3X, row1Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_MODE_LABEL, hInstance, nullptr);
    SendMessageW(hModeLabel, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);

    HWND hModeValue = CreateWindowW(
        L"STATIC", MODE_VALUE_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        col3X, row2Y, colWidth, textHeight,
        parent, (HMENU)ID_STATIC_MODE_VALUE, hInstance, nullptr);
    SendMessageW(hModeValue, WM_SETFONT, (WPARAM)g_hFontLang, TRUE);
}

void CreateLyricsAreaControls(HWND parent, HINSTANCE hInstance)
{
    // Placeholder: lyrics area bounds are set (LYRICS_AREA_TOP/HEIGHT), no controls yet
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(hInstance);
}

void CreateBottomControls(HWND parent, HINSTANCE hInstance)
{
    g_hFontTime = CreateFontW(
        FONT_SIZE_TIME, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    g_hFontStatus = CreateFontW(
        FONT_SIZE_STATUS, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

    // Bottom card uses the main window background so it blends with the lyrics area above
    int cardBottom = BOTTOM_CARD_TOP + BOTTOM_CARD_HEIGHT;

    // Time row: Current time (left), playback controls (center), End time (right)
    int timeRowY = BOTTOM_CARD_TOP + PROGRESS_BAR_HEIGHT + TIME_TEXT_MARGIN;
    int timeRowCenterY = timeRowY + TIME_TEXT_HEIGHT / 2;

    // Current time - left aligned
    HWND hStaticCurrTime = CreateWindowW(
        L"STATIC", CURRENT_TIME.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        CARD_LEFT + TIME_TEXT_MARGIN, timeRowY, 60, TIME_TEXT_HEIGHT,
        parent, (HMENU)ID_STATIC_CURR_TIME, hInstance, nullptr);
    SendMessageW(hStaticCurrTime, WM_SETFONT, (WPARAM)g_hFontTime, TRUE);

    // End time - right aligned
    HWND hStaticEndTime = CreateWindowW(
        L"STATIC", END_TIME.c_str(),
        WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - 60 - TIME_TEXT_MARGIN, timeRowY, 60, TIME_TEXT_HEIGHT,
        parent, (HMENU)ID_STATIC_END_TIME, hInstance, nullptr);
    SendMessageW(hStaticEndTime, WM_SETFONT, (WPARAM)g_hFontTime, TRUE);

    // Previous / Play-Pause / Next, centered as a group (3 icons + 2 gaps)
    int controlsTotalWidth = (PLAY_ICON_SIZE * 3) + (PLAY_ICON_GAP * 2);
    int controlsX = CARD_LEFT + (CARD_WIDTH - controlsTotalWidth) / 2;
    int controlsY = timeRowCenterY - PLAY_ICON_SIZE / 2;

    // Previous: \u23EE (skip backward / previous track)
    HWND hBtnPrev = CreateWindowW(
        L"STATIC", L"\u23EE",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        controlsX, controlsY, PLAY_ICON_SIZE, PLAY_ICON_SIZE,
        parent, (HMENU)ID_BTN_PREV, hInstance, nullptr);
    SendMessageW(hBtnPrev, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);
    SetWindowSubclass(hBtnPrev, IconHoverSubclassProc, 4, (DWORD_PTR)&g_hoverPrev);
    controlsX += PLAY_ICON_SIZE + PLAY_ICON_GAP;

    // Starts as pause icon since g_isPlaying defaults to true; updated via TogglePlayPause()
    HWND hBtnPlayPause = CreateWindowW(
        L"STATIC", L"\u23F8",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        controlsX, controlsY, PLAY_ICON_SIZE, PLAY_ICON_SIZE,
        parent, (HMENU)ID_BTN_PLAY_PAUSE, hInstance, nullptr);
    SendMessageW(hBtnPlayPause, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);
    SetWindowSubclass(hBtnPlayPause, IconHoverSubclassProc, 5, (DWORD_PTR)&g_hoverPlayPause);
    controlsX += PLAY_ICON_SIZE + PLAY_ICON_GAP;

    // Next: \u23ED (skip forward / next track)
    HWND hBtnNext = CreateWindowW(
        L"STATIC", L"\u23ED",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        controlsX, controlsY, PLAY_ICON_SIZE, PLAY_ICON_SIZE,
        parent, (HMENU)ID_BTN_NEXT, hInstance, nullptr);
    SendMessageW(hBtnNext, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);
    SetWindowSubclass(hBtnNext, IconHoverSubclassProc, 6, (DWORD_PTR)&g_hoverNext);

    // Status text: "Playing" - centered at bottom of card
    int statusY = cardBottom - STATUS_TEXT_HEIGHT - STATUS_MARGIN_BOTTOM;
    HWND hStaticStatus = CreateWindowW(
        L"STATIC", STATUS_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        CARD_LEFT, statusY, CARD_WIDTH, STATUS_TEXT_HEIGHT,
        parent, (HMENU)ID_STATIC_STATUS, hInstance, nullptr);
    SendMessageW(hStaticStatus, WM_SETFONT, (WPARAM)g_hFontStatus, TRUE);
}

// Toggles playing state and updates the play/pause button icon accordingly
void TogglePlayPause(HWND hwnd)
{
    g_isPlaying = !g_isPlaying;
    UpdatePlayPauseButton(hwnd);
}

void HandlePlaybackAction(HWND hwnd, int controlId)
{
    using playback_controls::PlaybackAction;

    switch (controlId)
    {
    case ID_BTN_PLAY_PAUSE:
        if (playback_controls::send_action(PlaybackAction::PlayPause))
        {
            TogglePlayPause(hwnd);
        }
        break;

    case ID_BTN_NEXT:
        playback_controls::send_action(PlaybackAction::Next);
        break;

    case ID_BTN_PREV:
        playback_controls::send_action(PlaybackAction::Previous);
        break;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HINSTANCE hInstance = ((LPCREATESTRUCTW)lParam)->hInstance;
            timeline_tracker::initialize(hwnd, hInstance);
            CreateHeaderControls(hwnd, hInstance);
            CreateLanguageBarControls(hwnd, hInstance);
            CreateLyricsAreaControls(hwnd, hInstance);
            CreateBottomControls(hwnd, hInstance);
            return 0;
        }

        // No-op: WM_PAINT below fills the whole client area itself (double-buffered), so skip the default erase to avoid flicker
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC screenDc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            HDC hdc = CreateCompatibleDC(screenDc);
            HBITMAP memBitmap = CreateCompatibleBitmap(screenDc, clientRect.right, clientRect.bottom);
            HBITMAP oldMemBitmap = (HBITMAP)SelectObject(hdc, memBitmap);

            // Fill the client area since WM_ERASEBKGND is now a no-op
            FillRect(hdc, &clientRect, g_hbrBackground);

            // --- Header card ---
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, g_hbrCard);
            HPEN   nullPen  = (HPEN)GetStockObject(NULL_PEN);
            HPEN   oldPen   = (HPEN)SelectObject(hdc, nullPen);

            RoundRect(hdc,
                CARD_LEFT, CARD_TOP,
                CARD_LEFT + CARD_WIDTH, CARD_TOP + CARD_HEIGHT,
                CARD_RADIUS, CARD_RADIUS);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            // --- Language bar (same style as header card) ---
            oldBrush = (HBRUSH)SelectObject(hdc, g_hbrCard);
            oldPen   = (HPEN)SelectObject(hdc, nullPen);

            RoundRect(hdc,
                CARD_LEFT, LANG_BAR_TOP,
                CARD_LEFT + CARD_WIDTH, LANG_BAR_TOP + LANG_BAR_HEIGHT,
                CARD_RADIUS, CARD_RADIUS);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            // --- Lyrics area placeholder: dim dotted outline, remove once real controls are added ---
            HPEN hpenOutline = CreatePen(PS_DOT, 1, RGB(0x30, 0x30, 0x45));
            oldPen = (HPEN)SelectObject(hdc, hpenOutline);
            oldBrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
            RoundRect(hdc,
                CARD_LEFT + 2, LYRICS_AREA_TOP,
                CARD_LEFT + CARD_WIDTH - 2, LYRICS_AREA_TOP + LYRICS_AREA_HEIGHT,
                CARD_RADIUS, CARD_RADIUS);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(hpenOutline);

            // --- Bottom card (blends with main window background) ---
            oldBrush = (HBRUSH)SelectObject(hdc, g_hbrBackground);
            oldPen   = (HPEN)SelectObject(hdc, nullPen);

            RoundRect(hdc,
                CARD_LEFT, BOTTOM_CARD_TOP,
                CARD_LEFT + CARD_WIDTH, BOTTOM_CARD_TOP + BOTTOM_CARD_HEIGHT,
                CARD_RADIUS, CARD_RADIUS);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            // --- Progress bar, filled based on the tracked timeline position ---
            int barLeft   = CARD_LEFT + PROGRESS_BAR_H_MARGIN;
            int barTop    = BOTTOM_CARD_TOP + PROGRESS_BAR_MARGIN;
            int barRight  = CARD_LEFT + CARD_WIDTH - PROGRESS_BAR_H_MARGIN;
            int barBottom = barTop + PROGRESS_BAR_HEIGHT;
            int barWidth  = barRight - barLeft;

            double duration = timeline_tracker::get_duration_seconds();
            double position = timeline_tracker::get_current_position_seconds();
            int fillWidth = 0;
            if (duration > 0.0)
            {
                double ratio = position / duration;
                if (ratio < 0.0) ratio = 0.0;
                if (ratio > 1.0) ratio = 1.0;
                fillWidth = static_cast<int>(barWidth * ratio);
            }
            else
            {
                fillWidth = (barWidth * PROGRESS_BAR_FILL_PERCENT) / 100;
            }

            // Background track (slightly lighter than card bg)
            HBRUSH hbrTrack = CreateSolidBrush(RGB(0x30, 0x30, 0x45));
            oldBrush = (HBRUSH)SelectObject(hdc, hbrTrack);
            RoundRect(hdc, barLeft, barTop, barRight, barBottom, 3, 3);
            SelectObject(hdc, oldBrush);
            DeleteObject(hbrTrack);

            // Filled portion (red)
            oldBrush = (HBRUSH)SelectObject(hdc, g_hbrProgressBar);
            if (fillWidth > 0)
            {
                RoundRect(hdc, barLeft, barTop, barLeft + fillWidth, barBottom, 3, 3);
            }
            SelectObject(hdc, oldBrush);

            // Blit the fully composed frame to the screen in one go.
            BitBlt(screenDc, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

            SelectObject(hdc, oldMemBitmap);
            DeleteObject(memBitmap);
            DeleteDC(hdc);

            EndPaint(hwnd, &ps);
            return 0;
        }

        // Per-control text color/background for STATIC controls
        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            int ctrlId = GetDlgCtrlID((HWND)lParam);

            SetBkMode(hdcStatic, TRANSPARENT);

            if (ctrlId == ID_STATIC_SONG)
                SetTextColor(hdcStatic, APP_COLOR_SONG_TEXT);
            else if (ctrlId == ID_STATIC_ARTIST)
                SetTextColor(hdcStatic, APP_COLOR_ARTIST_TEXT);
            else if (ctrlId == ID_BTN_PTT && g_hoverPTT)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_BTN_REFRESH && g_hoverRefresh)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_BTN_SETTINGS && g_hoverSettings)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_BTN_PREV && g_hoverPrev)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_BTN_PLAY_PAUSE && g_hoverPlayPause)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_BTN_NEXT && g_hoverNext)
                SetTextColor(hdcStatic, APP_COLOR_ICON_HOVER);
            else if (ctrlId == ID_STATIC_CURR_TIME || ctrlId == ID_STATIC_END_TIME)
                SetTextColor(hdcStatic, APP_COLOR_LIGHT_TEXT);
            else if (ctrlId == ID_STATIC_STATUS)
                SetTextColor(hdcStatic, APP_COLOR_ARTIST_TEXT);
            else if (ctrlId == ID_BTN_PREV || ctrlId == ID_BTN_PLAY_PAUSE || ctrlId == ID_BTN_NEXT)
                SetTextColor(hdcStatic, APP_COLOR_LIGHT_TEXT);
            // Language bar: labels use light text, values use artist (grey) text
            else if (ctrlId == ID_STATIC_LANG_LABEL ||
                     ctrlId == ID_STATIC_CURRENT_LABEL ||
                     ctrlId == ID_STATIC_MODE_LABEL)
                SetTextColor(hdcStatic, APP_COLOR_LIGHT_TEXT);
            else if (ctrlId == ID_STATIC_LANG_VALUE ||
                     ctrlId == ID_STATIC_CURRENT_VALUE ||
                     ctrlId == ID_STATIC_MODE_VALUE)
                SetTextColor(hdcStatic, APP_COLOR_ARTIST_TEXT);
            else
                SetTextColor(hdcStatic, APP_COLOR_LIGHT_TEXT);

            // Bottom card controls use background brush; header card and language bar controls use card brush
            if (ctrlId == ID_STATIC_CURR_TIME || ctrlId == ID_STATIC_END_TIME ||
                ctrlId == ID_STATIC_STATUS || ctrlId == ID_BTN_PREV ||
                ctrlId == ID_BTN_PLAY_PAUSE || ctrlId == ID_BTN_NEXT)
            {
                return (LRESULT)g_hbrBackground;
            }

            // Language bar controls sit on the same card-colored background
            if (ctrlId == ID_STATIC_LANG_LABEL || ctrlId == ID_STATIC_LANG_VALUE ||
                ctrlId == ID_STATIC_CURRENT_LABEL || ctrlId == ID_STATIC_CURRENT_VALUE ||
                ctrlId == ID_STATIC_MODE_LABEL || ctrlId == ID_STATIC_MODE_VALUE)
            {
                return (LRESULT)g_hbrCard;
            }

            return (LRESULT)g_hbrCard;
        }

        // Offset value box: white background, dark text
        case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, APP_COLOR_EDIT_TEXT);
            SetBkColor(hdcEdit, APP_COLOR_EDIT_BG);
            return (LRESULT)g_hbrEditBg;
        }

        case WM_TIMER:
        {
            if (wParam == timeline_tracker::TIMER_ID_TIMELINE_UPDATE)
            {
                timeline_tracker::handle_timer();
            }
            return 0;
        }

        case WM_COMMAND:
        {
            // STN_CLICKED and BN_CLICKED are both 0, so this catches clicks from either
            switch (LOWORD(wParam))
            {
                case ID_BTN_PTT:
                {
                    g_isPinned = !g_isPinned;
                    SetWindowPos(hwnd, g_isPinned ? HWND_TOPMOST : HWND_NOTOPMOST,
                        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

                    HWND hBtnPTT = GetDlgItem(hwnd, ID_BTN_PTT);
                    if (hBtnPTT)
                        SetWindowTextW(hBtnPTT, g_isPinned ? L"\u2605" : L"\u2606"); // filled vs outline star
                    break;
                }

                case ID_BTN_SETTINGS:
                case ID_BTN_OFFSET_MINUS:
                case ID_BTN_OFFSET_PLUS:
                    // TODO: hook up functionality later
                    break;

                case ID_BTN_REFRESH:
                    auto_nudge(0.5);
                    break;

                case ID_BTN_PREV:
                case ID_BTN_NEXT:
                case ID_BTN_PLAY_PAUSE:
                    HandlePlaybackAction(hwnd, LOWORD(wParam));
                    break;
            }
            return 0;
        }

        case WM_DESTROY:
            timeline_tracker::cleanup();
            if (g_hbrBackground)  { DeleteObject(g_hbrBackground);  g_hbrBackground = nullptr; }
            if (g_hbrCard)        { DeleteObject(g_hbrCard);        g_hbrCard = nullptr; }
            if (g_hbrEditBg)      { DeleteObject(g_hbrEditBg);      g_hbrEditBg = nullptr; }
            if (g_hbrProgressBar) { DeleteObject(g_hbrProgressBar); g_hbrProgressBar = nullptr; }
            if (g_hFontSong)      { DeleteObject(g_hFontSong);      g_hFontSong = nullptr; }
            if (g_hFontArtist)    { DeleteObject(g_hFontArtist);    g_hFontArtist = nullptr; }
            if (g_hFontIcon)      { DeleteObject(g_hFontIcon);      g_hFontIcon = nullptr; }
            if (g_hFontIconLarge) { DeleteObject(g_hFontIconLarge); g_hFontIconLarge = nullptr; }
            if (g_hFontLabel)     { DeleteObject(g_hFontLabel);     g_hFontLabel = nullptr; }
            if (g_hFontTime)      { DeleteObject(g_hFontTime);      g_hFontTime = nullptr; }
            if (g_hFontStatus)    { DeleteObject(g_hFontStatus);    g_hFontStatus = nullptr; }
            if (g_hFontLang)      { DeleteObject(g_hFontLang);      g_hFontLang = nullptr; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}