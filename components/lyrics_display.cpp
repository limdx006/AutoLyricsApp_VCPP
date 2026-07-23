#include "lyrics_display.h"
#include "config.h"
#include <algorithm>
#include <cmath>

namespace lyrics_display {
    static HWND g_hwnd = nullptr;
    static vector<LyricLine> g_lines;
    static int g_currentIndex = -1;
    static bool g_animating = false;
    static ULONGLONG g_animStartTick = 0;
    static HFONT g_hFontNormal = nullptr;
    static HFONT g_hFontCurrent = nullptr;

    static void ensure_fonts()
    {
        if (g_hFontNormal)
            return;

        g_hFontNormal = CreateFontW(
            FONT_SIZE_LYRICS, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

        g_hFontCurrent = CreateFontW(
            FONT_SIZE_LYRICS_CURRENT, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);
    }

    void initialize(HWND hwnd)
    {
        g_hwnd = hwnd;
        g_lines.clear();
        g_currentIndex = -1;
        g_animating = false;
        ensure_fonts();
    }

    void cleanup()
    {
        if (g_hwnd)
            KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
        g_hwnd = nullptr;

        if (g_hFontNormal)  { DeleteObject(g_hFontNormal);  g_hFontNormal = nullptr; }
        if (g_hFontCurrent) { DeleteObject(g_hFontCurrent); g_hFontCurrent = nullptr; }
    }

    void set_lines(vector<LyricLine> lines)
    {
        g_lines = std::move(lines);
        g_currentIndex = -1;
        g_animating = false;

        if (g_hwnd)
        {
            KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
            InvalidateRect(g_hwnd, nullptr, FALSE);
        }
    }

    // Last line whose timestamp <= position_seconds, or -1 if before the first line.
    static int find_active_index(double position_seconds)
    {
        int lo = 0, hi = (int)g_lines.size() - 1, result = -1;
        while (lo <= hi)
        {
            int mid = (lo + hi) / 2;
            if (g_lines[mid].timestamp <= position_seconds)
            {
                result = mid;
                lo = mid + 1;
            }
            else
            {
                hi = mid - 1;
            }
        }
        return result;
    }

    void sync(double position_seconds)
    {
        if (g_lines.empty() || !g_hwnd)
            return;

        int newIndex = find_active_index(position_seconds);
        if (newIndex == g_currentIndex)
            return;

        // Only animate the normal "advanced to the next line" case; seeks/jumps snap instantly.
        bool animate = (g_currentIndex >= 0 && newIndex == g_currentIndex + 1);
        g_currentIndex = newIndex;

        if (animate)
        {
            g_animating = true;
            g_animStartTick = GetTickCount64();
            SetTimer(g_hwnd, TIMER_ID_LYRICS_ANIM, 16, nullptr);
        }
        else
        {
            g_animating = false;
            KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
        }

        InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    void handle_anim_timer()
    {
        if (!g_animating || !g_hwnd)
            return;

        if (GetTickCount64() - g_animStartTick >= LYRICS_ANIM_DURATION_MS)
        {
            g_animating = false;
            KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
        }

        InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    static COLORREF lerp_color(COLORREF a, COLORREF b, double t)
    {
        auto lerp = [t](BYTE ca, BYTE cb) { return (BYTE)(ca + (cb - ca) * t); };
        return RGB(lerp(GetRValue(a), GetRValue(b)),
                    lerp(GetGValue(a), GetGValue(b)),
                    lerp(GetBValue(a), GetBValue(b)));
    }

    void draw(HDC hdc, const RECT& area)
    {
        if (g_lines.empty() || g_currentIndex < 0)
            return;

        // 0 = just started sliding, 1 = settled on the new current line. Ease-out cubic for a smooth finish.
        double t = 1.0;
        if (g_animating)
        {
            double raw = (std::min)(1.0, (double)(GetTickCount64() - g_animStartTick) / LYRICS_ANIM_DURATION_MS);
            t = 1.0 - std::pow(1.0 - raw, 3.0);
        }

        int centerY = (area.top + area.bottom) / 2;
        int areaHeight = area.bottom - area.top;
        // Calculate line spacing so that 5 lines (current +/-2) fill the height of the area.
        float lineSpacing = static_cast<float>(areaHeight) / 5.0f;

        HRGN oldClip = CreateRectRgn(0, 0, 0, 0);
        int hadClip = GetClipRgn(hdc, oldClip);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);

        for (int i = g_currentIndex - 2; i <= g_currentIndex + 2; ++i)
        {
            if (i < 0 || i >= (int)g_lines.size())
                continue;

            // finalOffset: -1 = one slot above center, 0 = center, +1 = one slot below.
            // Mid-animation it's offset by (1 - t) extra slots, so the whole
            // stack visibly slides up by one slot as t goes 0 -> 1.
            double newOffset = i - g_currentIndex;
            double finalOffset = g_animating ? (newOffset + (1.0 - t)) : newOffset;
            if (finalOffset < -2.5 || finalOffset > 2.5)
                continue;

            double closeness = (std::max)(0.0, 1.0 - std::abs(finalOffset)); // 1 at center, 0 a slot away
            HFONT font = (closeness > 0.5) ? g_hFontCurrent : g_hFontNormal;
            COLORREF color = lerp_color(APP_COLOR_ARTIST_TEXT, APP_COLOR_LIGHT_TEXT, closeness);

            int y = centerY + (int)std::lround(finalOffset * lineSpacing);
            const int horizontalMargin = 10; // pixels of padding left/right
            RECT lineRect = { area.left + horizontalMargin, 
                              static_cast<LONG>(y - lineSpacing / 2.0f), 
                              area.right - horizontalMargin, 
                              static_cast<LONG>(y + lineSpacing / 2.0f) };

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetTextColor(hdc, color);
            DrawTextW(hdc, g_lines[i].text.c_str(), -1, &lineRect,
                DT_CENTER | DT_WORDBREAK | DT_VCENTER | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
        }

        SetBkMode(hdc, oldBkMode);
        SelectClipRgn(hdc, hadClip == 1 ? oldClip : nullptr);
        DeleteObject(oldClip);
    }
}
