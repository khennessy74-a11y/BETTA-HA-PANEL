/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"

/*
 * A named Material Design Icon that is compiled into
 * BETTA's shared icon font.
 */
typedef struct {
    const char *name;
    uint32_t codepoint;
} mdi_icon_entry_t;


/*
 * Shared MDI fonts
 */

const lv_font_t *mdi_font_large(void);
bool mdi_font_large_available(void);

const lv_font_t *mdi_font_icon_42(void);
bool mdi_font_icon_42_available(void);

const lv_font_t *mdi_font_icon_56(void);
bool mdi_font_icon_56_available(void);

const lv_font_t *mdi_font_icon_72(void);
bool mdi_font_icon_72_available(void);

const lv_font_t *mdi_font_weather(void);
bool mdi_font_weather_available(void);

const lv_font_t *mdi_font_weather_20(void);
bool mdi_font_weather_20_available(void);

const lv_font_t *mdi_font_weather_small(void);
bool mdi_font_weather_small_available(void);


/*
 * Named icon registry
 *
 * Names use Home Assistant / MDI notation:
 *
 *     mdi:power
 *     mdi:timer-outline
 *
 * mdi_icon_lookup() returns true when the requested
 * icon is known and writes its Unicode codepoint to
 * out_codepoint.
 */
bool mdi_icon_lookup(
    const char *name,
    uint32_t *out_codepoint);


/*
 * Returns the registry table.
 *
 * The returned table is static and must not be freed.
 */
const mdi_icon_entry_t *mdi_icon_registry(
    size_t *out_count);


/*
 * Convert a Unicode codepoint to UTF-8.
 *
 * out_utf8 must provide at least 5 bytes.
 */
bool mdi_icon_codepoint_to_utf8(
    uint32_t codepoint,
    char out_utf8[5]);
