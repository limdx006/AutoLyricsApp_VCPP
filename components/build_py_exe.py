import PyInstaller.__main__
import subprocess
import sys

# 1. Build the lyrics-fetcher helper
PyInstaller.__main__.run([
    'components/lyrics_fetcher.py',
    '--onefile',
    '--console',
    '--name', 'py_helper',
    '--distpath', '.',

    # Core dependencies
    '--hidden-import', 'syncedlyrics',
    '--hidden-import', 'flask',
])

# 2. Build the translator / romanization helper
PyInstaller.__main__.run([
    'components/lyrics_translator.py',
    '--onefile',
    '--console',
    '--name', 'py_translator',
    '--distpath', '.',

    # Romanization dependencies
    '--hidden-import', 'cutlet',
    '--hidden-import', 'fugashi',
    '--hidden-import', 'unidic_lite',
    '--hidden-import', 'pypinyin',
    '--hidden-import', 'korean_romanizer',

    # Data / binaries that need to be bundled alongside
    '--collect-data', 'unidic_lite',
    '--collect-data', 'fugashi',
    '--collect-data', 'cutlet',
    '--collect-data', 'pypinyin',
    '--collect-data', 'korean_romanizer',
    '--collect-binaries', 'fugashi',
    '--collect-binaries', 'mecab',
    '--collect-submodules', 'unidic_lite',
    '--collect-submodules', 'fugashi',
    '--collect-submodules', 'cutlet',
    '--collect-submodules', 'pypinyin',
    '--collect-submodules', 'korean_romanizer',
])
