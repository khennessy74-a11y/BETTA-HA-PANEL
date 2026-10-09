/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#pragma once

#include "lvgl.h"

/*
 * Shared card-style tile geometry.
 *
 * This intentionally covers only the common card rhythm: card padding,
 * top-state anchor, bottom-title anchor, and centering an icon between the
 * visible state/title labels. Tiles with specialised geometry (weather,
 * heating arcs, media controls, graphs, etc.) can keep their own internals
 * while still opting into these anchors where appropriate.
 */

static inline lv_coord_t app_tile_card_padding(bool compact)
{
#if defined(CONFIG_APP_PANEL_VARIANT_S3_480)
    return compact ? 12 : 16;
#else
    return compact ? 12 : 16;
#endif
}

static inline lv_coord_t app_tile_state_top_y(bool compact)
{
    (void)compact;
    return 2;
}

static inline lv_coord_t app_tile_title_bottom_y(bool compact)
{
#if defined(CONFIG_APP_PANEL_VARIANT_S3_480)
    return compact ? -28 : -12;
#else
    return compact ? -40 : -12;
#endif
}

static inline lv_coord_t app_tile_icon_gap(bool compact)
{
    return compact ? 4 : 8;
}

static inline bool app_tile_obj_visible(lv_obj_t *obj)
{
    return obj != NULL && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static inline void app_tile_position_icon_between_labels(
    lv_obj_t *card,
    lv_obj_t *icon,
    lv_obj_t *state_label,
    lv_obj_t *title_label,
    lv_coord_t gap,
    lv_coord_t bias_y)
{
    if (card == NULL || icon == NULL) {
        return;
    }

    lv_obj_update_layout(card);

    if (gap < 0) {
        gap = 0;
    }

    lv_coord_t content_h = lv_obj_get_height(card)
        - lv_obj_get_style_pad_top(card, LV_PART_MAIN)
        - lv_obj_get_style_pad_bottom(card, LV_PART_MAIN);
    if (content_h < 1) {
        content_h = lv_obj_get_height(card);
    }

    lv_coord_t top = gap;
    if (app_tile_obj_visible(state_label)) {
        top = lv_obj_get_y(state_label) + lv_obj_get_height(state_label) + gap;
    }

    lv_coord_t bottom = content_h - gap;
    if (app_tile_obj_visible(title_label)) {
        bottom = lv_obj_get_y(title_label) - gap;
    }

    lv_coord_t icon_h = lv_obj_get_height(icon);
    if (icon_h < 1) {
        const lv_font_t *font = lv_obj_get_style_text_font(icon, LV_PART_MAIN);
        if (font != NULL) {
            icon_h = font->line_height;
        }
    }
    if (icon_h < 1) {
        icon_h = 1;
    }

    lv_coord_t y = top;
    lv_coord_t room = bottom - top;
    if (room >= icon_h) {
        y = top + (room - icon_h) / 2;
    }

    lv_coord_t max_y = bottom - icon_h;
    if (max_y < top) {
        max_y = top;
    }

    y += bias_y;
    if (y < top) y = top;
    if (y > max_y) y = max_y;

    lv_obj_set_y(icon, y);
}
