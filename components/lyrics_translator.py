import json
import sys

import cutlet as _cutlet
from pypinyin import lazy_pinyin as _lazy_pinyin, Style as _Style

"""LYRICS TRANSLATOR - Romanization helpers + standalone entry point

Functions:
- Japanese/Korean  text → Romaji
- Chinese          text → PinYin

Standalone usage (JSON-in-JSON-out via stdin/stdout):
    echo '{"language":"ja","lines":["こんにちは"]}' | python translator.py
    → {"success":true,"lines":["konnichiwa"]}
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
        print("korean-romanizer import failed:", e, file=sys.stderr)
        _korean_romanizer = None
        return None


def to_romaji(text):
    """Convert Japanese text to Hepburn romaji using cutlet."""
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


def _batch_convert(lines, language):
    """Convert a list of lines according to *language*."""
    if language == "ja":
        return [to_romaji(line) for line in lines]
    elif language == "ko":
        return [to_romanized_korean(line) for line in lines]
    elif language == "zh":
        return [to_pinyin(line) for line in lines]
    else:
        return list(lines)  # passthrough


def main():
    """Read JSON from an env-var-specified file, a file argument, or stdin
    (in that order of priority).  Write JSON to stdout."""
    sys.stdin.reconfigure(encoding="utf-8")
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

    import os  # delayed import so the top-level is clean

    input_file = os.environ.get("TRANSLATOR_INPUT_FILE")
    if input_file:
        with open(input_file, "r", encoding="utf-8") as f:
            data = json.load(f)
    elif len(sys.argv) > 1:
        with open(sys.argv[1], "r", encoding="utf-8") as f:
            data = json.load(f)
    else:
        data = json.load(sys.stdin)
    language = data.get("language", "")
    lines = data.get("lines", [])

    converted = _batch_convert(lines, language)

    json.dump({"success": True, "lines": converted}, sys.stdout,
              ensure_ascii=False)
    sys.stdout.flush()


if __name__ == "__main__":
    main()
