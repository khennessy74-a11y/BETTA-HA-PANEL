/* Shared optional icon colours for stateful tiles. Empty strings mean theme defaults. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "lvgl.h"

static inline int state_icon_hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

static inline bool state_icon_parse_color(const char *text, lv_color_t *out)
{
    if (text == NULL || out == NULL || text[0] == '\0') return false;
    const char *p = text;
    if (*p == '#') p++;
    else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    if (strlen(p) != 6) return false;
    uint32_t rgb = 0;
    for (size_t i = 0; i < 6; ++i) {
        int n = state_icon_hex_nibble(p[i]);
        if (n < 0) return false;
        rgb = (rgb << 4) | (uint32_t)n;
    }
    *out = lv_color_hex(rgb);
    return true;
}
