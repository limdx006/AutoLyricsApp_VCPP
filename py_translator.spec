# -*- mode: python ; coding: utf-8 -*-
from PyInstaller.utils.hooks import collect_data_files
from PyInstaller.utils.hooks import collect_dynamic_libs
from PyInstaller.utils.hooks import collect_submodules

datas = []
binaries = []
hiddenimports = ['cutlet', 'fugashi', 'unidic_lite', 'pypinyin', 'korean_romanizer']
datas += collect_data_files('unidic_lite')
datas += collect_data_files('fugashi')
datas += collect_data_files('cutlet')
datas += collect_data_files('pypinyin')
datas += collect_data_files('korean_romanizer')
binaries += collect_dynamic_libs('fugashi')
binaries += collect_dynamic_libs('mecab')
hiddenimports += collect_submodules('unidic_lite')
hiddenimports += collect_submodules('fugashi')
hiddenimports += collect_submodules('cutlet')
hiddenimports += collect_submodules('pypinyin')
hiddenimports += collect_submodules('korean_romanizer')


a = Analysis(
    ['components\\lyrics_translator.py'],
    pathex=[],
    binaries=binaries,
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='py_translator',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
