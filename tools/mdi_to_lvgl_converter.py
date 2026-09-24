#!/usr/bin/env python3
"""Build the Material Design Icons range used by BETTA's LVGL icon fonts.

The manifest below is the single place to add/remove icons.  The script reads
the bundled MDI TTF, resolves each icon name from its cmap, and prints the
numeric --range accepted by lv_font_conv / the LVGL online converter.

Requires: fonttools (pip install fonttools)
"""
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
EXPANDED_CODEPOINTS = {
    "lock": 0xF033E,
    "lock-open-variant": 0xF0FC6,
    "door": 0xF081A,
    "window-closed": 0xF05AE,
    "motion-sensor": 0xF0D91,
    "account": 0xF0004,
    "water-alert": 0xF1502,
    "smoke-detector": 0xF0392,
    "blinds": 0xF00AC,
    "script-text": 0xF0BC2,
    "robot": 0xF06A9,
    "gesture-tap-button": 0xF12A8,
    "numeric": 0xF03A0,
    "toggle-switch": 0xF0521,
    "play-circle": 0xF040C,
    "home-automation": 0xF07D1,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--list", action="store_true", help="print resolved name/codepoint pairs")
    args = parser.parse_args()

    # The bundled MDI webfont does not preserve semantic icon names in its
    # cmap, so name->glyph discovery from TTFont is not reliable.  Keep the
    # expansion codepoints explicit and verify them when updating MDI.
    legacy = {
        "vector-square": 0xF0001, "access-point-network": 0xF0002,
        "account-switch": 0xF0019, "airballoon": 0xF001C,
        "alert-circle": 0xF0028, "alert-octagon": 0xF0029,
        "arrow-all": 0xF0041, "arrow-right-drop-circle": 0xF0059,
        "arrow-up": 0xF005D, "assistant": 0xF0064, "battery-90": 0xF0082,
        "camera-front": 0xF0102, "chart-histogram": 0xF0129,
        "chevron-double-left": 0xF013D, "clock-outline": 0xF0150,
        "code-not-equal-variant": 0xF0171, "domain": 0xF01D7, "drag": 0xF01DB,
        "eye-off": 0xF0209, "fan": 0xF0210, "file-check": 0xF0216,
        "file-music": 0xF0223, "file-outline": 0xF0224, "file-code": 0xF022E,
        "gate": 0xF0299, "gauge": 0xF029A, "hololens": 0xF02DB,
        "contactless-payment-circle": 0xF0321, "laptop": 0xF0322,
        "link-off": 0xF0338, "menu": 0xF035C, "menu-down": 0xF035D,
        "menu-left": 0xF035E, "numeric-5-box": 0xF03B1,
        "playlist-play": 0xF0411, "plus": 0xF0415, "projector": 0xF042E,
        "reddit": 0xF044D, "cog": 0xF0493, "swap-vertical": 0xF04E2,
        "weather-cloudy": 0xF0590, "weather-fog": 0xF0591,
        "weather-hail": 0xF0592, "weather-lightning": 0xF0593,
        "weather-night": 0xF0594, "weather-partly-cloudy": 0xF0595,
        "weather-pouring": 0xF0596, "weather-rainy": 0xF0597,
        "weather-snowy": 0xF0598, "weather-sunny": 0xF0599,
        "weather-sunset": 0xF059A, "weather-sunset-down": 0xF059B,
        "weather-sunset-up": 0xF059C, "weather-windy": 0xF059D,
        "weather-windy-variant": 0xF059E, "weather-lightning-rainy": 0xF067E,
        "weather-snowy-rainy": 0xF067F, "weather-hurricane": 0xF0898,
        "weather-cloudy-arrow-right": 0xF0E6E, "weather-cloudy-alert": 0xF0F2F,
        "weather-hazy": 0xF0F30, "weather-night-partly-cloudy": 0xF0F31,
        "weather-partly-lightning": 0xF0F32, "weather-partly-rainy": 0xF0F33,
        "weather-partly-snowy": 0xF0F34, "weather-partly-snowy-rainy": 0xF0F35,
        "weather-snowy-heavy": 0xF0F36, "weather-sunny-alert": 0xF0F37,
        "weather-tornado": 0xF0F38, "weather-sunny-off": 0xF14E4,
    }
    codepoints = {**legacy, **EXPANDED_CODEPOINTS}
    missing = [name for name in ICONS if name not in codepoints]
    if missing:
        raise SystemExit("Missing codepoints: " + ", ".join(missing))
    resolved = [(name, codepoints[name]) for name in ICONS]

    resolved.sort(key=lambda item: item[1])
    if args.list:
        for name, cp in resolved:
            print(f"mdi:{name}=U+{cp:05X}")
    print("Range: " + ",".join(str(cp) for _, cp in resolved))

if __name__ == "__main__":
    main()
