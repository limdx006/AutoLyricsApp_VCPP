#include "language_detector.h"
#include <cstdlib>

namespace {

    // CJK Unified Ideographs (U+4E00–U+9FFF) and Extension A (U+3400–U+4DBF)
    bool is_cjk(wchar_t ch) {
        return (ch >= 0x3400 && ch <= 0x4DBF) ||
               (ch >= 0x4E00 && ch <= 0x9FFF);
    }

    // Hiragana (U+3040–U+309F)
    bool is_hiragana(wchar_t ch) {
        return ch >= 0x3040 && ch <= 0x309F;
    }

    // Full-width Katakana (U+30A0–U+30FF) and half-width katakana (U+FF65–U+FF9F)
    bool is_katakana(wchar_t ch) {
        return (ch >= 0x30A0 && ch <= 0x30FF) ||
               (ch >= 0xFF65 && ch <= 0xFF9F);
    }

    // Hangul Syllables (U+AC00–U+D7AF), Jamo (U+1100–U+11FF), Compatibility Jamo (U+3130–U+318F)
    bool is_hangul(wchar_t ch) {
        return (ch >= 0x1100 && ch <= 0x11FF) ||
               (ch >= 0x3130 && ch <= 0x318F) ||
               (ch >= 0xAC00 && ch <= 0xD7AF);
    }

    // Basic Latin + Latin-1 Supplement + Latin Extended-A/B
    bool is_latin(wchar_t ch) {
        return (ch >= L'A' && ch <= L'Z') ||
               (ch >= L'a' && ch <= L'z') ||
               (ch >= 0x00C0 && ch <= 0x024F);
    }
}

Language detect_language(const vector<wstring>& lyricLines) {
    int cjkCount = 0, hiraganaCount = 0, katakanaCount = 0;
    int hangulCount = 0, latinCount = 0;
    int totalNonSpace = 0;

    for (const auto& line : lyricLines) {
        for (wchar_t ch : line) {
            if (ch == L' ' || ch == L'\r' || ch == L'\n')
                continue;
            totalNonSpace++;
            if (is_hiragana(ch))
                hiraganaCount++;
            else if (is_katakana(ch))
                katakanaCount++;
            else if (is_hangul(ch))
                hangulCount++;
            else if (is_cjk(ch))
                cjkCount++;
            else if (is_latin(ch))
                latinCount++;
        }
    }

    if (totalNonSpace == 0)
        return Language::English; // fallback for empty/whitespace-only lyrics

    // Priority: Japanese-specific characters (hiragana/katakana) → Korean (hangul) → CJK → Latin
    if (hiraganaCount > 0 || katakanaCount > 0)
        return Language::Japanese;

    if (hangulCount > 0)
        return Language::Korean;

    if (cjkCount > latinCount)
        return Language::Chinese;

    return Language::English;
}

const wchar_t* language_to_wstring(Language lang) {
    switch (lang) {
        case Language::Chinese:  return L"Chinese";
        case Language::English:  return L"English";
        case Language::Japanese: return L"Japanese";
        case Language::Korean:   return L"Korean";
        default:                 return L"Unknown";
    }
}
