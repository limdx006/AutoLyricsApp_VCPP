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

        // Measure line height using both fonts to get the maximum height needed
        int fontHeight = 0;
        {
            HFONT hOld = (HFONT)SelectObject(hdc, g_hFontNormal);
            TEXTMETRICW tm;
            GetTextMetricsW(hdc, &tm);
            fontHeight = tm.tmHeight + tm.tmExternalLeading;
            SelectObject(hdc, hOld);
        }
        {
            HFONT hOld = (HFONT)SelectObject(hdc, g_hFontCurrent);
            TEXTMETRICW tm;
            GetTextMetricsW(hdc, &tm);
            int h = tm.tmHeight + tm.tmExternalLeading;
            if (h > fontHeight)
                fontHeight = h;
            SelectObject(hdc, hOld);
        }
        if (fontHeight <= 0)
            fontHeight = 1; // avoid division by zero

        // Slot height (distance between centers of consecutive lines, includes gap)
        // Make it large enough to allow a line to wrap to two lines.
        const int slotHeight = static_cast<int>(fontHeight * 2.2f); // ~2.2 lines height

        int areaHeight = area.bottom - area.top;
        // Maximum number of lines that can fit vertically (using slotHeight)
        int maxLines = areaHeight / slotHeight;
        if (maxLines < 1)
            maxLines = 1;

        // Lines to show above and below the current line (to center current line as much as possible)
        int linesAbove = (maxLines - 1) / 2;
        int linesBelow = maxLines - 1 - linesAbove;

        // 0 = just started sliding, 1 = settled on the new current line. Ease-out cubic for a smooth finish.
        double t = 1.0;
        if (g_animating)
        {
            double raw = (std::min)(1.0, (double)(GetTickCount64() - g_animStartTick) / LYRICS_ANIM_DURATION_MS);
            t = 1.0 - std::pow(1.0 - raw, 3.0);
        }

        int centerY = (area.top + area.bottom) / 2;

        HRGN oldClip = CreateRectRgn(0, 0, 0, 0);
        int hadClip = GetClipRgn(hdc, oldClip);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);

        // Determine a safe range of indices to consider (we'll cull by offset later)
        int minIndex = g_currentIndex - maxLines - 2;
        int maxIndex = g_currentIndex + maxLines + 2;
        if (minIndex < 0) minIndex = 0;
        if (maxIndex >= (int)g_lines.size()) maxIndex = (int)g_lines.size() - 1;

        for (int i = minIndex; i <= maxIndex; ++i)
        {
            // Compute offset in lines (slots) from current index
            double lineOffset = i - g_currentIndex;
            double finalOffset = lineOffset;
            if (g_animating)
            {
                // When animating, shift the entire stack by (1-t) slots in the direction opposite to the change
                // (so that lines slide up when advancing to a higher index)
                finalOffset += (1.0 - t);
            }

            // Check if this line is within the visible range (with a little extra for animation)
            if (finalOffset < -(linesAbove + 1.0) || finalOffset > (linesBelow + 1.0))
                continue;

            double closeness = (std::max)(0.0, 1.0 - std::abs(finalOffset)); // 1 at center, 0 a slot away
            HFONT font = (closeness > 0.5) ? g_hFontCurrent : g_hFontNormal;
            COLORREF color = lerp_color(APP_COLOR_ARTIST_TEXT, APP_COLOR_LIGHT_TEXT, closeness);

            int y = centerY + static_cast<int>(std::lround(finalOffset * slotHeight));
            const int horizontalMargin = 20; // pixels of padding left/right
            // Use slotHeight as the rectangle height to allow wrapping (up to ~2 lines)
            RECT lineRect = { area.left + horizontalMargin, 
                              y - slotHeight / 2, 
                              area.right - horizontalMargin, 
                              y + slotHeight / 2 };

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