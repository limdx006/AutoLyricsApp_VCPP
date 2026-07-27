import PyInstaller.__main__

PyInstaller.__main__.run([
    # Entry point — lyrics_fetcher.py reads TRACK_TITLE / TRACK_ARTIST from
    # environment, writes JSON to stdout.  The --console flag keeps the pipe
    # working (--windowed would break subprocess I/O).
    'components/lyrics_fetcher.py',
    '--onefile',
    '--console',
    '--name', 'py_helper',
    '--distpath', '.',

    # Core dependencies
    '--hidden-import', 'syncedlyrics',
    '--hidden-import', 'flask',

    # ── Romanization (disabled for now; uncomment when translator is ready) ──
    # --hidden-import cutlet
    # --hidden-import fugashi
    # --hidden-import unidic_lite
    # --hidden-import pypinyin
    # --hidden-import korean_romanizer
    # --collect-data unidic_lite
    # --collect-data fugashi
    # --collect-data cutlet
    # --collect-data pypinyin
    # --collect-data korean_romanizer
    # --collect-binaries fugashi
    # --collect-binaries mecab
    # --collect-submodules unidic_lite
    # --collect-submodules fugashi
    # --collect-submodules cutlet
    # --collect-submodules pypinyin
    # --collect-submodules korean_romanizer
])
