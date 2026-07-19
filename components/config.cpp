#include "config.h"

const int WINDOW_WIDTH  = 400;
const int WINDOW_HEIGHT = 800;

const int CARD_MARGIN = 15;
const int CARD_LEFT   = CARD_MARGIN;
const int CARD_TOP    = CARD_MARGIN;
const int CARD_WIDTH  = WINDOW_WIDTH - (CARD_MARGIN * 2);
const int CARD_HEIGHT = 180;
const int CARD_RADIUS = 24;

const int PTT_SIZE        = 40;
const int PTT_MARGIN      = 10;
const int SIDE_RESERVED   = PTT_SIZE + PTT_MARGIN;
const int SONG_TOP_OFFSET = 10;
const int ARTIST_GAP      = 6;

const int FOOTER_ROW_OFFSET_FROM_BOTTOM = 40;
const int FOOTER_ICON_SIZE   = 44;
const int FOOTER_ICON_MARGIN = 10;

const int OFFSET_LABEL_WIDTH        = 60;
const int OFFSET_LABEL_HEIGHT       = 28;
const int OFFSET_BTN_SIZE           = 28;
const int OFFSET_EDIT_WIDTH         = 40;
const int OFFSET_EDIT_HEIGHT        = 25;
const int OFFSET_GAP_LABEL_TO_MINUS = 5;
const int OFFSET_GAP_MINUS_TO_EDIT  = 2;
const int OFFSET_GAP_EDIT_TO_PLUS   = 2;

const int FONT_SIZE_SONG       = 30;
const int FONT_SIZE_ARTIST     = 15;
const int FONT_SIZE_ICON       = 40;
const int FONT_SIZE_ICON_LARGE = 40;
const int FONT_SIZE_LABEL      = 25;
const wchar_t* const FONT_FACE_UI     = L"Segoe UI";
const wchar_t* const FONT_FACE_SYMBOL = L"Segoe UI Symbol";

const COLORREF APP_COLOR_BACKGROUND  = RGB(0x1a, 0x1a, 0x2e);
const COLORREF APP_COLOR_CARD        = RGB(0x16, 0x21, 0x3e);
const COLORREF APP_COLOR_EDIT_BG     = RGB(0xff, 0xff, 0xff);
const COLORREF APP_COLOR_SONG_TEXT   = RGB(0xe0, 0x5a, 0x6e);
const COLORREF APP_COLOR_ARTIST_TEXT = RGB(0xa0, 0xa0, 0xb0);
const COLORREF APP_COLOR_LIGHT_TEXT  = RGB(0xf0, 0xf0, 0xf0);
const COLORREF APP_COLOR_EDIT_TEXT   = RGB(0x1a, 0x1a, 0x2e);
const COLORREF APP_COLOR_ICON_HOVER  = RGB(0x4c, 0xc9, 0xf0);

const wchar_t* const SONG_NAME    = L"Song Name Goes Here";
const wchar_t* const ARTIST_NAME  = L"Unknown Artist";
const wchar_t* const OFFSET_VALUE = L"0.3";