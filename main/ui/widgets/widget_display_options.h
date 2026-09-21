/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Shared display-option helpers for BETTA widget tiles.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lvgl.h"
#include "ui/fonts/mdi_font_registry.h"

static inline bool widget_display_apply_mdi(lv_obj_t *label, const char *name)
{
    if (label == NULL || name == NULL || name[0] == '\0') return false;
    uint32_t cp = 0;
    if (!mdi_icon_lookup(name, &cp)) return false;
    const lv_font_t *font = mdi_font_icon_42();
    if (font == NULL) font = mdi_font_icon_56();
    if (font == NULL) font = mdi_font_large();
    if (font == NULL) return false;
    lv_font_glyph_dsc_t dsc = {0};
    if (!lv_font_get_glyph_dsc(font, &dsc, cp, 0)) return false;
    char utf8[5] = {0};
    if (!mdi_icon_codepoint_to_utf8(cp, utf8)) return false;
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_text(label, utf8);
    return true;
}

static inline void widget_display_set_visible(lv_obj_t *obj, bool visible)
{
    if (obj == NULL) return;
    if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
