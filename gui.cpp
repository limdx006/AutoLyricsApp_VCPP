#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include "components/config.h"

// Globals
static HBRUSH g_hbrBackground = nullptr;
static HBRUSH g_hbrCard        = nullptr;
static HBRUSH g_hbrEditBg      = nullptr;
static HFONT  g_hFontSong      = nullptr;
static HFONT  g_hFontArtist    = nullptr;
static HFONT  g_hFontIcon      = nullptr;
static HFONT  g_hFontIconLarge = nullptr;
static HFONT  g_hFontLabel     = nullptr;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateHeaderControls(HWND parent, HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"AutoLyricsWindowClass";

    g_hbrBackground = CreateSolidBrush(APP_COLOR_BACKGROUND);
    g_hbrCard       = CreateSolidBrush(APP_COLOR_CARD);
    g_hbrEditBg     = CreateSolidBrush(APP_COLOR_EDIT_BG);

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

    // Song name area is inset by SIDE_RESERVED on both sides: the right
    // inset leaves room for the PTT icon, the left inset is left blank
    // to match, so the text block stays visually centered on the card.
    int songX = CARD_LEFT + SIDE_RESERVED;
    int songY = CARD_TOP + SONG_TOP_OFFSET;
    int songWidth = CARD_WIDTH - (SIDE_RESERVED * 2);

    // Measure how tall the song text actually needs to be once wrapped
    // to songWidth, so a short title takes one line and a long title
    // takes two without overlapping the artist line below it.
    RECT calcRect = { 0, 0, songWidth, 0 };
    HDC hdc = GetDC(parent);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSong);
    DrawTextW(hdc, SONG_NAME, -1, &calcRect, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
    SelectObject(hdc, oldFont);
    ReleaseDC(parent, hdc);
    int songHeight = calcRect.bottom - calcRect.top;

    HWND hStaticSong = CreateWindowW(
        L"STATIC", SONG_NAME,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        songX, songY, songWidth, songHeight,
        parent, (HMENU)ID_STATIC_SONG, hInstance, nullptr);
    SendMessageW(hStaticSong, WM_SETFONT, (WPARAM)g_hFontSong, TRUE);

    // Artist name starts right where the (possibly wrapped) song text
    // actually ends, so it never overlaps regardless of song length.
    int artistY = songY + songHeight + ARTIST_GAP;
    HWND hStaticArtist = CreateWindowW(
        L"STATIC", ARTIST_NAME,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
        CARD_LEFT + 20, artistY, CARD_WIDTH - 40, 22,
        parent, (HMENU)ID_STATIC_ARTIST, hInstance, nullptr);
    SendMessageW(hStaticArtist, WM_SETFONT, (WPARAM)g_hFontArtist, TRUE);

    // Pin-To-Top: created after the song/artist controls so it's on top
    // in z-order, and its box sits entirely inside the space reserved by
    // SIDE_RESERVED so the song text can never paint over it.
    HWND hBtnPTT = CreateWindowW(
        L"STATIC", L"\u2606",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - PTT_MARGIN - PTT_SIZE, songY, PTT_SIZE, PTT_SIZE,
        parent, (HMENU)ID_BTN_PTT, hInstance, nullptr);
    SendMessageW(hBtnPTT, WM_SETFONT, (WPARAM)g_hFontIcon, TRUE);

    // Footer row: REF | Offset: - 0.3 + | ST
    // Every control in this row is a different size, so instead of giving
    // them all the same y, we pick one horizontal center line for the row
    // and place each control so its own vertical center sits on that line.
    int rowCenterY = CARD_TOP + CARD_HEIGHT - FOOTER_ROW_OFFSET_FROM_BOTTOM;
    int refY = rowCenterY - FOOTER_ICON_SIZE / 2;

    HWND hBtnRefresh = CreateWindowW(
        L"STATIC", L"\u21BB",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + FOOTER_ICON_MARGIN, refY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE,
        parent, (HMENU)ID_BTN_REFRESH, hInstance, nullptr);
    SendMessageW(hBtnRefresh, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);

    HWND hBtnSettings = CreateWindowW(
        L"STATIC", L"\u2699",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY | SS_NOPREFIX,
        CARD_LEFT + CARD_WIDTH - FOOTER_ICON_MARGIN - FOOTER_ICON_SIZE, refY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE,
        parent, (HMENU)ID_BTN_SETTINGS, hInstance, nullptr);
    SendMessageW(hBtnSettings, WM_SETFONT, (WPARAM)g_hFontIconLarge, TRUE);

    // Offset cluster, centered within the card. Buttons stay as real
    // BUTTON controls so the visible border stays, matching the reference.
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

        // Paint the rounded header card near the top of the window.
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
                SetTextColor(hdcStatic, APP_COLOR_SONG_TEXT);
            else if (ctrlId == ID_STATIC_ARTIST)
                SetTextColor(hdcStatic, APP_COLOR_ARTIST_TEXT);
            else
                SetTextColor(hdcStatic, APP_COLOR_LIGHT_TEXT);

            return (LRESULT)g_hbrCard;
        }

        // EDIT control (the offset value box): white background,
        // dark text, matching the reference screenshot's white pill.
        case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, APP_COLOR_EDIT_TEXT);
            SetBkColor(hdcEdit, APP_COLOR_EDIT_BG);
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
            if (g_hbrBackground)  { DeleteObject(g_hbrBackground);  g_hbrBackground = nullptr; }
            if (g_hbrCard)        { DeleteObject(g_hbrCard);        g_hbrCard = nullptr; }
            if (g_hbrEditBg)      { DeleteObject(g_hbrEditBg);      g_hbrEditBg = nullptr; }
            if (g_hFontSong)      { DeleteObject(g_hFontSong);      g_hFontSong = nullptr; }
            if (g_hFontArtist)    { DeleteObject(g_hFontArtist);    g_hFontArtist = nullptr; }
            if (g_hFontIcon)      { DeleteObject(g_hFontIcon);      g_hFontIcon = nullptr; }
            if (g_hFontIconLarge) { DeleteObject(g_hFontIconLarge); g_hFontIconLarge = nullptr; }
            if (g_hFontLabel)     { DeleteObject(g_hFontLabel);     g_hFontLabel = nullptr; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}