#pragma once
#include "common.h"

// Control IDs. These are used in switch/case and as HMENU literals, both
// of which require compile-time constants, so they stay as #define rather
// than becoming config.cpp variables like everything else here.
#define ID_BTN_PTT          101   // Pin-To-Top
#define ID_BTN_REFRESH       102
#define ID_BTN_SETTINGS      103
#define ID_STATIC_SONG       104
#define ID_STATIC_ARTIST     105
#define ID_STATIC_OFFSET_LBL 106
#define ID_BTN_OFFSET_MINUS  107
#define ID_EDIT_OFFSET       108
#define ID_BTN_OFFSET_PLUS   109

// Bottom card controls
#define ID_STATIC_CURR_TIME  110
#define ID_STATIC_END_TIME   111
#define ID_STATIC_STATUS     112
#define ID_BTN_PREV          113
#define ID_BTN_PLAY_PAUSE    114
#define ID_BTN_NEXT          115

// Language bar controls
#define ID_STATIC_LANG_LABEL     116
#define ID_STATIC_LANG_VALUE     117
#define ID_STATIC_CURRENT_LABEL  118
#define ID_STATIC_CURRENT_VALUE  119
#define ID_STATIC_MODE_LABEL     120
#define ID_STATIC_MODE_VALUE     121

// Lyrics area controls (reserved for future use)
#define ID_LYRICS_CONTAINER      122
#define ID_LYRICS_LINE_1         123
#define ID_LYRICS_LINE_2         124
#define ID_LYRICS_LINE_3         125

// Window size
extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

// Header card layout
extern const int CARD_MARGIN;   // gap between window edge and card
extern const int CARD_LEFT;
extern const int CARD_TOP;
extern const int CARD_WIDTH;
extern const int CARD_HEIGHT;
extern const int CARD_RADIUS;   // corner rounding

// Pin-To-Top sizing. PTT sits at the top-right of the card; SIDE_RESERVED
// is kept clear on the left too so the song text block stays centered.
extern const int PTT_SIZE;
extern const int PTT_MARGIN;      // gap from card edge to PTT box
extern const int SIDE_RESERVED;   // PTT_SIZE + PTT_MARGIN
extern const int SONG_TOP_OFFSET; // gap from card top to the song/PTT row
extern const int ARTIST_GAP;      // gap between song text block and artist line

// Footer row: REF | Offset: - value + | ST
extern const int FOOTER_ROW_OFFSET_FROM_BOTTOM; // how far above the card bottom the row sits
extern const int FOOTER_ICON_SIZE;               // REF / ST box size
extern const int FOOTER_ICON_MARGIN;             // gap from card edge to REF / ST

// Offset cluster (the "Offset: - 0.3 +" group)
extern const int OFFSET_LABEL_WIDTH;
extern const int OFFSET_LABEL_HEIGHT;
extern const int OFFSET_BTN_SIZE;
extern const int OFFSET_EDIT_WIDTH;
extern const int OFFSET_EDIT_HEIGHT;
extern const int OFFSET_GAP_LABEL_TO_MINUS;
extern const int OFFSET_GAP_MINUS_TO_EDIT;
extern const int OFFSET_GAP_EDIT_TO_PLUS;

// Language bar layout (2-row info bar below header card)
extern const int LANG_BAR_TOP;
extern const int LANG_BAR_HEIGHT;
extern const int LANG_BAR_MARGIN;       // horizontal inset from card edges
extern const int LANG_BAR_ROW1_OFFSET;  // vertical offset of top row from bar top
extern const int LANG_BAR_ROW2_OFFSET;  // vertical offset of bottom row from bar top

// Lyrics area layout (reserved space between language bar and bottom card)
extern const int LYRICS_GAP;            // gap above and below the lyrics area
extern const int LYRICS_AREA_TOP;       // top of the lyrics display area
extern const int LYRICS_AREA_HEIGHT;    // height of the lyrics display area
extern const int LYRICS_AREA_BOTTOM;    // bottom of the lyrics display area

// Bottom card layout
extern const int BOTTOM_CARD_HEIGHT;
extern const int BOTTOM_CARD_TOP;
extern const int PROGRESS_BAR_HEIGHT;
extern const int PROGRESS_BAR_MARGIN;      // vertical margin (top)
extern const int PROGRESS_BAR_H_MARGIN;    // horizontal margin (left/right) - adjust this to make bar narrower/wider
extern const int PROGRESS_BAR_FILL_PERCENT;
extern const int TIME_TEXT_HEIGHT;
extern const int TIME_TEXT_MARGIN;
extern const int PLAY_ICON_SIZE;
extern const int PLAY_ICON_GAP;
extern const int STATUS_TEXT_HEIGHT;
extern const int STATUS_MARGIN_BOTTOM;

// Fonts
extern const int FONT_SIZE_SONG;
extern const int FONT_SIZE_ARTIST;
extern const int FONT_SIZE_ICON;
extern const int FONT_SIZE_ICON_LARGE;
extern const int FONT_SIZE_LABEL;
extern const int FONT_SIZE_TIME;
extern const int FONT_SIZE_STATUS;
extern const int FONT_SIZE_LANG;    // language bar (slightly smaller than artist)
extern const int FONT_SIZE_LYRICS;  // far lines (2+ away from the highlighted line)
extern const int FONT_SIZE_LYRICS_NEAR;    // line immediately before/after the highlighted one
extern const int FONT_SIZE_LYRICS_CURRENT; // active (center) lyric line -- bigger + bold
extern const int LYRICS_LINE_SPACING;      // vertical distance between stacked lyric lines
extern const UINT LYRICS_ANIM_DURATION_MS; // how long the slide-to-next-line animation takes
extern const wchar_t* const FONT_FACE_UI;
extern const wchar_t* const FONT_FACE_SYMBOL;

// Colors
extern const COLORREF APP_COLOR_BACKGROUND;   // main window bg
extern const COLORREF APP_COLOR_CARD;         // header card bg
extern const COLORREF APP_COLOR_EDIT_BG;      // offset value box bg
extern const COLORREF APP_COLOR_SONG_TEXT;    // song name accent color
extern const COLORREF APP_COLOR_ARTIST_TEXT;  // artist name color
extern const COLORREF APP_COLOR_LIGHT_TEXT;   // icon labels / offset label color
extern const COLORREF APP_COLOR_LYRICS_NEAR;  // lyric line immediately before/after the highlighted one
extern const COLORREF APP_COLOR_LYRICS_FAR;   // lyric lines two or more away from the highlighted one
extern const COLORREF APP_COLOR_EDIT_TEXT;    // offset value text color
extern const COLORREF APP_COLOR_ICON_HOVER;   // PTT/Refresh/Settings hover color
extern const COLORREF APP_COLOR_PROGRESS_BAR; // progress bar fill color

// Hardcoded display text (temporary, until real song data is wired up)
extern const wchar_t* const SONG_NAME;
extern const wchar_t* const ARTIST_NAME;
extern const wchar_t* const OFFSET_VALUE;
extern std::wstring CURRENT_TIME;
extern std::wstring END_TIME;
extern const wchar_t* const STATUS_TEXT;

// Language bar display text
extern const wchar_t* const LANG_LABEL_TEXT;
extern const wchar_t* const LANG_VALUE_TEXT;
extern const wchar_t* const CURRENT_LABEL_TEXT;
extern const wchar_t* const CURRENT_VALUE_TEXT;
extern const wchar_t* const MODE_LABEL_TEXT;
extern const wchar_t* const MODE_VALUE_TEXT;