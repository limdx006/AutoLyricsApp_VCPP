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

        const int horizontalMargin = 20; // pixels of padding left/right
        int textLeft  = area.left + horizontalMargin;
        int textRight = area.right - horizontalMargin;
        int textWidth = textRight - textLeft;

        // Fixed vertical gap kept between adjacent lyric line blocks
        const int LINE_MARGIN = 24;

        auto measureHeight = [&](const wstring& text, HFONT font) -> int
        {
            RECT calc = { 0, 0, textWidth, 0 };
            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            DrawTextW(hdc, text.c_str(), -1, &calc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
            return (std::max)(1, (int)(calc.bottom - calc.top));
        };

        // Typical single-line height for normal-font lines (governs every
        // gap except the two right next to the center line).
        TEXTMETRICW tmNormal;
        {
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontNormal);
            GetTextMetricsW(hdc, &tmNormal);
            SelectObject(hdc, oldFont);
        }
        int normalLineHeight = tmNormal.tmHeight + tmNormal.tmExternalLeading;

        // Actual height of the (target) center line 
        int currentHeight = measureHeight(g_lines[g_currentIndex].text, g_hFontCurrent);

        // Distance from the center slot to the slot right next to it has to account for the center block's real (possibly taller) height;
        double firstStep = currentHeight / 2.0 + normalLineHeight / 2.0 + LINE_MARGIN;
        double otherStep = normalLineHeight + LINE_MARGIN;

        // Y offset of a given whole slot (0 = center) from centerY.
        auto slotOffsetY = [&](int slot) -> double
        {
            if (slot == 0) return 0.0;
            double y = firstStep;
            for (int s = 2; s <= std::abs(slot); ++s)
                y += otherStep;
            return slot > 0 ? y : -y;
        };

        // How many extra lines fit above/below the center within the area,
        // using the plain single-line spacing as the yardstick.
        int areaHeight = area.bottom - area.top;
        int maxExtra = (int)(areaHeight / 2.0 / (otherStep)) ; // per side, rough fit
        if (maxExtra < 1) maxExtra = 1;
        int linesAbove = maxExtra;
        int linesBelow = maxExtra;

        // 0 = just started sliding, 1 = settled on the new current line. Ease-out cubic for a smooth finish.
        double t = 1.0;
        if (g_animating)
        {
            double raw = (std::min)(1.0, (double)(GetTickCount64() - g_animStartTick) / LYRICS_ANIM_DURATION_MS);
            t = 1.0 - std::pow(1.0 - raw, 3.0);
        }
        double shift = g_animating ? (1.0 - t) : 0.0; // extra slots of vertical shift, 1 -> 0 over the animation

        int centerY = (area.top + area.bottom) / 2;

        HRGN oldClip = CreateRectRgn(0, 0, 0, 0);
        int hadClip = GetClipRgn(hdc, oldClip);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);

        int minIndex = (std::max)(0, g_currentIndex - linesAbove - 1);
        int maxIndex = (std::min)((int)g_lines.size() - 1, g_currentIndex + linesBelow + 1);

        for (int i = minIndex; i <= maxIndex; ++i)
        {
            double newOffsetSlots = i - g_currentIndex;   // this line's settled target slot
            double finalOffsetSlots = newOffsetSlots + shift; // shifted while animating

            if (finalOffsetSlots < -(linesAbove + 1.0) || finalOffsetSlots > (linesBelow + 1.0))
                continue;

            double closeness = (std::max)(0.0, 1.0 - std::abs(finalOffsetSlots)); // 1 at center, 0 a slot away
            HFONT font = (closeness > 0.5) ? g_hFontCurrent : g_hFontNormal;
            COLORREF color = lerp_color(APP_COLOR_ARTIST_TEXT, APP_COLOR_LIGHT_TEXT, closeness);

            // Interpolate between the (fixed) integer-slot Y positions
            int lowSlot = (int)std::floor(finalOffsetSlots);
            int highSlot = lowSlot + 1;
            double frac = finalOffsetSlots - lowSlot;
            double y0 = slotOffsetY(lowSlot);
            double y1 = slotOffsetY(highSlot);
            double yOffset = y0 + (y1 - y0) * frac;

            int blockHeight = (font == g_hFontCurrent) ? currentHeight : normalLineHeight;
            int y = centerY + (int)std::lround(yOffset);
            RECT lineRect = { textLeft, y - blockHeight / 2 - 2, textRight, y + blockHeight / 2 + 2 };

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetTextColor(hdc, color);
            DrawTextW(hdc, g_lines[i].text.c_str(), -1, &lineRect,
                DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
        }

        SetBkMode(hdc, oldBkMode);
        SelectClipRgn(hdc, hadClip == 1 ? oldClip : nullptr);
        DeleteObject(oldClip);
    }
}