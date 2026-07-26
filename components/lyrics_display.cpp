#include "lyrics_display.h"
#include "config.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace lyrics_display {
    static HWND g_hwnd = nullptr;
    static vector<LyricLine> g_lines;
    static int g_currentIndex = -1;
    static bool g_animating = false;
    static ULONGLONG g_animStartTick = 0;
    static HFONT g_hFontNormal = nullptr;
    static HFONT g_hFontNear = nullptr;
    static HFONT g_hFontCurrent = nullptr;
    constexpr double DEFAULT_OFFSET = 1.0; // positive = lyrics shown that many seconds earlier
    static double g_offsetSeconds = DEFAULT_OFFSET;
    static DisplayStatus g_status = DisplayStatus::NoMedia;

    // Cached fonts indexed by (size << 8) | weight -- avoids a CreateFontW /
    // DeleteObject cycle per visible line per frame in draw().
    static std::unordered_map<int, HFONT> g_fontCache;

    static int font_cache_key(int size, int weight)
    {
        return (size << 8) | (weight & 0xFF);
    }

    static HFONT cached_font(int size, int weight)
    {
        int key = font_cache_key(size, weight);
        auto it = g_fontCache.find(key);
        if (it != g_fontCache.end())
            return it->second;
        HFONT hFont = CreateFontW(
            size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);
        g_fontCache[key] = hFont;
        return hFont;
    }

    static void ensure_fonts()
    {
        if (g_hFontNormal)
            return;

        g_hFontNormal = CreateFontW(
            FONT_SIZE_LYRICS, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_FACE_UI);

        g_hFontNear = CreateFontW(
            FONT_SIZE_LYRICS_NEAR, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
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
        if (g_hFontNear)    { DeleteObject(g_hFontNear);    g_hFontNear = nullptr; }
        if (g_hFontCurrent) { DeleteObject(g_hFontCurrent); g_hFontCurrent = nullptr; }

        for (auto& [key, hFont] : g_fontCache)
            DeleteObject(hFont);
        g_fontCache.clear();
    }

    void set_lines(vector<LyricLine> lines)
    {
        if (!lines.empty())
            g_status = DisplayStatus::None;

        g_lines = std::move(lines);
        g_currentIndex = -1;
        g_animating = false;

        if (g_hwnd)
        {
            KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
            InvalidateRect(g_hwnd, nullptr, FALSE);
        }
    }

    void set_status(DisplayStatus status)
    {
        g_status = status;
        if (g_hwnd)
            InvalidateRect(g_hwnd, nullptr, FALSE);
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

        int newIndex = find_active_index(position_seconds + g_offsetSeconds);
        if (newIndex == g_currentIndex)
            return;

        // Animate only for a normal single-line advance when no animation
        // is already in flight.  If sync is called during an active animation
        // (e.g. from the 16 ms WM_PAINT or from the 500 ms timeline timer),
        // starting a *second* animation would snap the scroll back to the
        // starting offset, causing visible flickering.
        bool animate = !g_animating && (newIndex == g_currentIndex + 1);
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

        InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    float get_offset()
    {
        return g_offsetSeconds;
    }

    void set_offset(float offset_seconds)
    {
        g_offsetSeconds = offset_seconds;
    }

    void reset_offset()
    {
        g_offsetSeconds = DEFAULT_OFFSET;
    }

    static COLORREF lerp_color(COLORREF a, COLORREF b, float t)
    {
        auto lerp = [t](BYTE ca, BYTE cb) { return (BYTE)(ca + (cb - ca) * t); };
        return RGB(lerp(GetRValue(a), GetRValue(b)),
                    lerp(GetGValue(a), GetGValue(b)),
                    lerp(GetBValue(a), GetBValue(b)));
    }

    void draw(HDC hdc, const RECT& area)
    {
        const int horizontalMargin = 20; // pixels of padding left/right
        int textLeft  = area.left + horizontalMargin;
        int textRight = area.right - horizontalMargin;
        int textWidth = textRight - textLeft;

        // When no lines are loaded, show a status message instead.
        if (g_lines.empty())
        {
            if (g_status == DisplayStatus::None)
                return;

            const wchar_t* text = L"";
            COLORREF color;
            switch (g_status)
            {
                case DisplayStatus::Searching:
                    text = L"Searching lyrics......";
                    color = RGB(0x4a, 0xcc, 0x6a); // green
                    break;
                case DisplayStatus::NoLyrics:
                    text = L"No lyrics was found maybe try another song";
                    color = RGB(0xe0, 0x5a, 0x5a); // red
                    break;
                case DisplayStatus::NoMedia:
                    text = L"No detected media";
                    color = RGB(0x4a, 0xcc, 0x6a); // green
                    break;
                default: return;
            }

            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontCurrent);
            SetTextColor(hdc, color);
            SetBkMode(hdc, TRANSPARENT);

            RECT textRect = { textLeft, area.top, textRight, area.bottom };
            DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
            return;
        }

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

        // Typical single-line height per tier (governs gaps not touching the center or near-neighbor slots, which get their real measured height below instead).
        auto lineHeightFor = [&](HFONT font) -> int
        {
            TEXTMETRICW tm;
            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            GetTextMetricsW(hdc, &tm);
            SelectObject(hdc, oldFont);
            return tm.tmHeight + tm.tmExternalLeading;
        };
        int normalLineHeight = lineHeightFor(g_hFontNormal);

        // Breadth-first word-wrap helper: inserts \r\n so the text fits within textWidth
        // when drawn with the current font in the HDC. Caller must select the reference font first.
        auto word_wrap = [&](const wstring& text) -> wstring
        {
            if (text.empty())
                return text;

            vector<wstring> words;
            size_t s = 0, e;
            while ((e = text.find(L' ', s)) != wstring::npos)
            {
                if (e > s)
                    words.push_back(text.substr(s, e - s));
                s = e + 1;
            }
            if (s < text.size())
                words.push_back(text.substr(s));

            if (words.empty())
                return text;

            wstring result;
            wstring currentLine = words[0];

            for (size_t i = 1; i < words.size(); ++i)
            {
                wstring testLine = currentLine + L" " + words[i];
                SIZE sz;
                GetTextExtentPoint32W(hdc, testLine.c_str(), (int)testLine.size(), &sz);

                if (sz.cx > textWidth)
                {
                    if (!result.empty())
                        result += L"\r\n";
                    result += currentLine;
                    currentLine = words[i];
                }
                else
                {
                    currentLine = testLine;
                }
            }

            if (!result.empty())
                result += L"\r\n";
            result += currentLine;
            return result;
        };

        // How many extra lines fit above/below the center within the area,
        // using the plain single-line spacing as the yardstick.
        int areaHeight = area.bottom - area.top;
        float otherStep = normalLineHeight + LINE_MARGIN;
        int maxExtra = (int)(areaHeight / 2.0f / otherStep); // per side, rough fit
        if (maxExtra < 1) maxExtra = 1;
        int linesAbove = maxExtra;
        int linesBelow = maxExtra;

        // Pre-wrap all visible lines using the current-line font so wrapping is consistent
        // at every position (far, near, centre) rather than changing as the line scrolls.
        int minIndex = (std::max)(0, g_currentIndex - linesAbove - 1);
        int maxIndex = (std::min)((int)g_lines.size() - 1, g_currentIndex + linesBelow + 1);
        HFONT oldWrapFont = (HFONT)SelectObject(hdc, g_hFontCurrent);
        vector<wstring> wrappedText(g_lines.size());
        for (int i = minIndex; i <= maxIndex; ++i)
            wrappedText[i] = word_wrap(g_lines[i].text);
        SelectObject(hdc, oldWrapFont);

        // Real (possibly wrapped) height of a specific slot's line, measured fresh each frame
        auto heightForSlot = [&](int slot) -> int
        {
            int idx = g_currentIndex + slot;
            if (idx < 0 || idx >= (int)g_lines.size())
                return normalLineHeight;
            if (slot == 0)
                return measureHeight(wrappedText[idx], g_hFontCurrent);
            if (std::abs(slot) == 1)
                return measureHeight(wrappedText[idx], g_hFontNear);
            return normalLineHeight;
        };

        int currentHeight = heightForSlot(0);

        // Fixed vertical gap kept between adjacent lyric line blocks
        auto slotOffsetY = [&](int slot) -> float
        {
            if (slot == 0) return 0.0f;
            int dir = slot > 0 ? 1 : -1;
            float y = 0.0f;
            int prevHeight = currentHeight;
            for (int s = 1; s <= std::abs(slot); ++s)
            {
                int h = heightForSlot(dir * s);
                y += prevHeight / 2.0f + h / 2.0f + LINE_MARGIN;
                prevHeight = h;
            }
            return dir * y;
        };

        // 0 = just started sliding, 1 = settled on the new current line.
        // Ease-out cubic for a smooth finish.  The animation-end check is
        // done here (at render time) rather than in handle_anim_timer so
        // there is no race between the timer expiry and the final draw.
        float t = 1.0f;
        if (g_animating)
        {
            ULONGLONG elapsed = GetTickCount64() - g_animStartTick;
            if (elapsed >= LYRICS_ANIM_DURATION_MS)
            {
                g_animating = false;
                KillTimer(g_hwnd, TIMER_ID_LYRICS_ANIM);
            }
            else
            {
                float raw = static_cast<float>(elapsed) / LYRICS_ANIM_DURATION_MS;
                t = 1.0f - std::pow(1.0f - raw, 3.0f);
            }
        }
        float shift = g_animating ? (1.0f - t) : 0.0f; // extra slots of vertical shift, 1 -> 0 over the animation

        int centerY = (area.top + area.bottom) / 2;

        HRGN oldClip = CreateRectRgn(0, 0, 0, 0);
        int hadClip = GetClipRgn(hdc, oldClip);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);

        // Maps a signed slot offset to font size and weight with smooth interpolation.
        auto fontParamsForOffset = [](float absOffset, int& outSize, int& outWeight)
        {
            float size, weight;
            if (absOffset <= 1.0f)
            {
                float t = static_cast<float>(absOffset);
                size = static_cast<float>(FONT_SIZE_LYRICS_CURRENT) + t * (FONT_SIZE_LYRICS_NEAR - FONT_SIZE_LYRICS_CURRENT);
                weight = static_cast<float>(FW_BOLD) + t * (FW_NORMAL - FW_BOLD);
            }
            else if (absOffset <= 2.0f)
            {
                float t = static_cast<float>(absOffset - 1.0f);
                size = static_cast<float>(FONT_SIZE_LYRICS_NEAR) + t * (FONT_SIZE_LYRICS - FONT_SIZE_LYRICS_NEAR);
                weight = static_cast<float>(FW_NORMAL);
            }
            else
            {
                size = static_cast<float>(FONT_SIZE_LYRICS);
                weight = static_cast<float>(FW_NORMAL);
            }
            outSize = static_cast<int>(std::round(size));
            outWeight = static_cast<int>(std::round(weight));
        };

        for (int i = minIndex; i <= maxIndex; ++i)
        {
            float newOffsetSlots = i - g_currentIndex;   // this line's settled target slot
            float finalOffsetSlots = newOffsetSlots + shift; // shifted while animating

            if (finalOffsetSlots < -(linesAbove + 1.0f) || finalOffsetSlots > (linesBelow + 1.0f))
                continue;

            // Continuous font size/weight based on distance from centre.
            float absOffset = std::abs(finalOffsetSlots);
            int fontSize, fontWeight;
            fontParamsForOffset(absOffset, fontSize, fontWeight);
            HFONT font = cached_font(fontSize, fontWeight);

            // 3-stop color gradient: white at the center, a brighter "near" tone at the immediate neighbor (offset ~1)
            COLORREF color;
            if (absOffset <= 1.0f)
                color = lerp_color(APP_COLOR_LIGHT_TEXT, APP_COLOR_LYRICS_NEAR, absOffset);
            else
                color = lerp_color(APP_COLOR_LYRICS_NEAR, APP_COLOR_LYRICS_FAR, (std::min)(1.0f, absOffset - 1.0f));

            // Interpolate between the (fixed) integer-slot Y positions
            int lowSlot = (int)std::floor(finalOffsetSlots);
            int highSlot = lowSlot + 1;
            float frac = finalOffsetSlots - lowSlot;
            float y0 = slotOffsetY(lowSlot);
            float y1 = slotOffsetY(highSlot);
            float yOffset = y0 + (y1 - y0) * frac;

            int blockHeight = measureHeight(wrappedText[i], font);
            int y = centerY + (int)std::lround(yOffset);
            RECT lineRect = { textLeft, y - blockHeight / 2 - 2, textRight, y + blockHeight / 2 + 2 };

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetTextColor(hdc, color);
            DrawTextW(hdc, wrappedText[i].c_str(), -1, &lineRect,
                DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
            // font is cached; no DeleteObject here
        }

        SetBkMode(hdc, oldBkMode);
        SelectClipRgn(hdc, hadClip == 1 ? oldClip : nullptr);
        DeleteObject(oldClip);
    }
}