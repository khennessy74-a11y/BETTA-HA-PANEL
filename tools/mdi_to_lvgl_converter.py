#!/usr/bin/env python3
"""Build the Material Design Icons range used by BETTA's LVGL icon fonts.

The manifest below is the single place to add/remove icons.  The script reads
the bundled MDI TTF, resolves each icon name from its cmap, and prints the
numeric --range accepted by lv_font_conv / the LVGL online converter.

Requires: fonttools (pip install fonttools)
"""
from pathlib import Path
import argparse

ICONS = [
    "vector-square", "access-point-network", "account-switch", "airballoon",
    "alert-circle", "alert-octagon", "arrow-all", "arrow-right-drop-circle",
    "arrow-up", "assistant", "battery-90", "camera-front", "chart-histogram",
    "chevron-double-left", "clock-outline", "code-not-equal-variant", "domain",
    "drag", "eye-off", "fan", "file-check", "file-music", "file-outline",
    "file-code", "gate", "gauge", "hololens", "contactless-payment-circle",
    "laptop", "link-off", "menu", "menu-down", "menu-left", "numeric-5-box",
    "playlist-play", "plus", "projector", "reddit", "cog", "swap-vertical",
    "weather-cloudy", "weather-fog", "weather-hail", "weather-lightning",
    "weather-night", "weather-partly-cloudy", "weather-pouring", "weather-rainy",
    "weather-snowy", "weather-sunny", "weather-sunset", "weather-sunset-down",
    "weather-sunset-up", "weather-windy", "weather-windy-variant",
    "weather-lightning-rainy", "weather-snowy-rainy", "weather-hurricane",
    "weather-cloudy-arrow-right", "weather-cloudy-alert", "weather-hazy",
    "weather-night-partly-cloudy", "weather-partly-lightning",
    "weather-partly-rainy", "weather-partly-snowy",
    "weather-partly-snowy-rainy", "weather-snowy-heavy",
    "weather-sunny-alert", "weather-tornado", "weather-sunny-off",
    # Entity/domain expansion
    "lock", "lock-open-variant", "door", "window-closed", "motion-sensor",
    "account", "water-alert", "smoke-detector", "blinds", "script-text",
    "robot", "gesture-tap-button", "numeric", "toggle-switch", "play-circle",
    "home-automation",
]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", default="main/ui/fonts/materialdesignicons-webfont.ttf")
    parser.add_argument("--list", action="store_true", help="print resolved name/codepoint pairs")
    args = parser.parse_args()

    try:
        from fontTools.ttLib import TTFont
    except ImportError:
        raise SystemExit("fonttools is required: pip install fonttools")

    font = TTFont(Path(args.font))
    cmap = font.getBestCmap() or {}
    # MDI glyph names in the font are normally prefixed mdi-.
    by_glyph = {glyph: cp for cp, glyph in cmap.items()}
    resolved, missing = [], []
    for name in ICONS:
        candidates = (name, "mdi-" + name)
        cp = next((by_glyph[c] for c in candidates if c in by_glyph), None)
        if cp is None:
            missing.append(name)
        else:
            resolved.append((name, cp))

    if missing:
        raise SystemExit("Missing icons in bundled font: " + ", ".join(missing))

    resolved.sort(key=lambda item: item[1])
    if args.list:
        for name, cp in resolved:
            print(f"mdi:{name}=U+{cp:05X}")
    print("Range: " + ",".join(str(cp) for _, cp in resolved))

if __name__ == "__main__":
    main()
