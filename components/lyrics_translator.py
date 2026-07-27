import re
import cutlet as _cutlet
from pypinyin import lazy_pinyin as _lazy_pinyin, Style as _Style

"""LYRICS UTILS - Pure helper functions for tranlating
- Japanese and Korean to Romaji
- Chinese to PinYin
"""

# Single shared instances — initialising these is expensive so do it once at import
_cutlet_engine = _cutlet.Cutlet()

# Lazy-loaded Korean romanizer engine
_korean_romanizer = None


def _get_korean_romanizer():
    """Lazy-load the Korean romanizer to avoid import overhead if not needed."""
    global _korean_romanizer
    if _korean_romanizer is not None:
        return _korean_romanizer
    try:
        from korean_romanizer.romanizer import Romanizer

        _korean_romanizer = Romanizer
        return _korean_romanizer
    except Exception as e:
        print("korean-romanizer import failed:", e)
        _korean_romanizer = None
        return None


def to_romaji(text):
    """Convert Japanese text to Hepburn romaji using cutlet (primary) with pykakasi as fallback."""
    if not text:
        return text
    try:
        return _cutlet_engine.romaji(text)
    except Exception:
        return text


def to_pinyin(text):
    """Convert Chinese text to pinyin with tone marks."""
    if not text:
        return text
    return " ".join(_lazy_pinyin(text, style=_Style.TONE))


def to_romanized_korean(text):
    """Convert Korean Hangul text to Revised Romanization using korean-romanizer.
    Falls back to original text if the library is unavailable."""
    if not text:
        return text
    Romanizer = _get_korean_romanizer()
    if not Romanizer:
        return text
    try:
        r = Romanizer(text)
        return r.romanize()
    except Exception:
        return text
