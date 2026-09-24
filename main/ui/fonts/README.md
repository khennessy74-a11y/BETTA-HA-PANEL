<!-- SPDX-License-Identifier: LicenseRef-FNCL-1.1 | Copyright (c) 2026 Cpt_Kirk -->
# MDI Icon Font Workflow

BETTA keeps the Material Design Icons source font at:

- `main/ui/fonts/materialdesignicons-webfont.ttf`

The authoritative firmware icon manifest lives in:

- `tools/mdi_to_lvgl_converter.py`

## Generate the converter range

From the repository root:

```powershell
python tools/mdi_to_lvgl_converter.py
```

Install the only tooling dependency if needed:

```powershell
python -m pip install fonttools
```

Use `--list` to audit the resolved MDI names and Unicode codepoints.

The script validates every requested icon against the bundled TTF and fails if
an icon name is unavailable. This prevents the registry and generated fonts
from silently drifting apart.

## Generate LVGL fonts

Use the printed `Range:` value with `lv_font_conv` or the LVGL online font
converter. Current BETTA shared icon sizes are 42, 56 and 72 px, BPP 4.

Example with `lv_font_conv`:

```powershell
lv_font_conv --font main/ui/fonts/materialdesignicons-webfont.ttf --range <RANGE> --size 42 --bpp 4 --format lvgl -o main/ui/fonts/mditop50icons42.c
lv_font_conv --font main/ui/fonts/materialdesignicons-webfont.ttf --range <RANGE> --size 56 --bpp 4 --format lvgl -o main/ui/fonts/mditop50icons56.c
lv_font_conv --font main/ui/fonts/materialdesignicons-webfont.ttf --range <RANGE> --size 72 --bpp 4 --format lvgl -o main/ui/fonts/mditop50icons72.c
```

Keep the generated C symbol names compatible with the existing files:
`mditop50icons42`, `mditop50icons56`, and `mditop50icons72`.

## Adding icons

Add the MDI name (without the `mdi:` prefix) to `ICONS`, regenerate all
three fonts, then add the resolved name/codepoint to `mdi_font_registry.c`.
Commit the manifest, generated fonts and registry together so every registered
icon is actually present in the compiled font.
