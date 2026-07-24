#include "config.h"

const int WINDOW_WIDTH  = 400;
const int WINDOW_HEIGHT = 800;

const int CARD_MARGIN = 5;
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
const int FOOTER_ICON_SIZE              = 44;
const int FOOTER_ICON_MARGIN            = 10;

const int OFFSET_LABEL_WIDTH        = 60;
const int OFFSET_LABEL_HEIGHT       = 28;
const int OFFSET_BTN_SIZE           = 28;
const int OFFSET_EDIT_WIDTH         = 40;
const int OFFSET_EDIT_HEIGHT        = 25;
const int OFFSET_GAP_LABEL_TO_MINUS = 5;
const int OFFSET_GAP_MINUS_TO_EDIT  = 2;
const int OFFSET_GAP_EDIT_TO_PLUS   = 2;

// Language bar: sits just below the header card with a small gap
const int LANG_BAR_TOP         = CARD_TOP + CARD_HEIGHT + 5;
const int LANG_BAR_HEIGHT      = 40;
const int LANG_BAR_MARGIN      = 5;   // horizontal inset for text
const int LANG_BAR_ROW1_OFFSET = 5;    // top row (labels) from bar top
const int LANG_BAR_ROW2_OFFSET = 22;   // bottom row (values) from bar top

// Bottom card layout (define before lyrics area so it can be referenced)
const int BOTTOM_CARD_HEIGHT        = 90;
const int BOTTOM_CARD_TOP           = WINDOW_HEIGHT - BOTTOM_CARD_HEIGHT - CARD_MARGIN;
const int PROGRESS_BAR_HEIGHT       = 7;
const int PROGRESS_BAR_MARGIN       = 0;    // vertical margin from card top
const int PROGRESS_BAR_H_MARGIN     = 18;   // horizontal margin from card edges (increase = narrower bar)
const int PROGRESS_BAR_FILL_PERCENT = 75;
const int TIME_TEXT_HEIGHT          = 15;
const int TIME_TEXT_MARGIN          = 15;
const int PLAY_ICON_SIZE            = 36;
const int PLAY_ICON_GAP             = 24;
const int STATUS_TEXT_HEIGHT        = 20;
const int STATUS_MARGIN_BOTTOM      = 5;

// Lyrics area: reserved space between language bar and bottom card
const int LYRICS_GAP         = 8;   // gap between lang bar / lyrics area, and lyrics area / bottom card
const int LYRICS_AREA_TOP    = LANG_BAR_TOP + LANG_BAR_HEIGHT + LYRICS_GAP;
const int LYRICS_AREA_BOTTOM = BOTTOM_CARD_TOP - LYRICS_GAP;
const int LYRICS_AREA_HEIGHT = LYRICS_AREA_BOTTOM - LYRICS_AREA_TOP;

const int FONT_SIZE_SONG       = 30;
const int FONT_SIZE_ARTIST     = 15;
const int FONT_SIZE_ICON       = 40;
const int FONT_SIZE_ICON_LARGE = 40;
const int FONT_SIZE_LABEL      = 25;
const int FONT_SIZE_TIME       = 14;
const int FONT_SIZE_STATUS     = 14;
const int FONT_SIZE_LANG       = 14;   // slightly smaller than artist
const int FONT_SIZE_LYRICS     = 20;   // far lines (2+ away from the highlighted line)
const int FONT_SIZE_LYRICS_NEAR    = 24; // line immediately before/after the highlighted one
const int FONT_SIZE_LYRICS_CURRENT = 32; // active (center) lyric line
const int LYRICS_LINE_SPACING      = 30;
const UINT LYRICS_ANIM_DURATION_MS = 250;

const wchar_t* const FONT_FACE_UI     = L"Segoe UI";
const wchar_t* const FONT_FACE_SYMBOL = L"Segoe UI Symbol";

const COLORREF APP_COLOR_BACKGROUND   = RGB(0x1a, 0x1a, 0x2e);
const COLORREF APP_COLOR_CARD         = RGB(0x16, 0x21, 0x3e);
const COLORREF APP_COLOR_EDIT_BG      = RGB(0xff, 0xff, 0xff);
const COLORREF APP_COLOR_SONG_TEXT    = RGB(0xe0, 0x5a, 0x6e);
const COLORREF APP_COLOR_ARTIST_TEXT  = RGB(0xa0, 0xa0, 0xb0);
const COLORREF APP_COLOR_LIGHT_TEXT   = RGB(0xf0, 0xf0, 0xf0);
const COLORREF APP_COLOR_LYRICS_NEAR  = RGB(0xc4, 0xc4, 0xd2);  // brighter than artist text -- the line right before/after the highlighted one
const COLORREF APP_COLOR_LYRICS_FAR   = RGB(0x60, 0x60, 0x72);  // dimmer than artist text -- everything further away
const COLORREF APP_COLOR_EDIT_TEXT    = RGB(0x1a, 0x1a, 0x2e);
const COLORREF APP_COLOR_ICON_HOVER   = RGB(0x4c, 0xc9, 0xf0);
const COLORREF APP_COLOR_PROGRESS_BAR = RGB(0xe0, 0x5a, 0x6e);  // red/pink

const wchar_t* const SONG_NAME    = L"Song Name Goes Here";
const wchar_t* const ARTIST_NAME  = L"Unknown Artist";
const wchar_t* const OFFSET_VALUE = L"0.3";
wstring CURRENT_TIME = L"02:24";
wstring END_TIME     = L"02:51";
const wchar_t* const STATUS_TEXT  = L"Playing";

// Language bar hardcoded text
const wchar_t* const LANG_LABEL_TEXT    = L"Language:";
const wchar_t* const LANG_VALUE_TEXT    = L"Chinese";
const wchar_t* const CURRENT_LABEL_TEXT = L"Current:";
const wchar_t* const CURRENT_VALUE_TEXT = L"PinYin";
const wchar_t* const MODE_LABEL_TEXT    = L"[  Original  ]";
const wchar_t* const MODE_VALUE_TEXT    = L"[ Translated ]";