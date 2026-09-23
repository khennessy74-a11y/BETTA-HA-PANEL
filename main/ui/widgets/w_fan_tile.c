/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"
#include "ui/widgets/widget_display_options.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *state;
    lv_obj_t *value;
    lv_obj_t *power;
    lv_obj_t *slider;
    lv_obj_t *preset;
    bool is_on;
    bool unavailable;
    bool supports_percentage;
    bool suppress;
    int percentage;
    bool show_title;
    bool show_state;
    char preset_mode[48];
    char preset_modes[16][48];
    int preset_count;
} w_fan_ctx_t;

static int clamp_percent(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

static void fan_apply_visual(w_fan_ctx_t *ctx)
{
    if (ctx == NULL) return;
    lv_obj_set_style_bg_color(ctx->card,
        lv_color_hex(ctx->is_on && !ctx->unavailable ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF),
        LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ctx->card, LV_OPA_COVER, LV_PART_MAIN);
    lv_label_set_text(ctx->state,
        ctx->unavailable
            ? ui_i18n_get("common.unavailable", "unavailable")
            : (ctx->is_on ? ui_i18n_get("common.on", "ON") : ui_i18n_get("common.off", "OFF")));
    lv_obj_set_style_text_color(ctx->state,
        lv_color_hex(ctx->unavailable ? APP_UI_COLOR_TEXT_MUTED :
            (ctx->is_on ? APP_UI_COLOR_STATE_ON : APP_UI_COLOR_STATE_OFF)), LV_PART_MAIN);

    char value[16];
    snprintf(value, sizeof(value), "%d%%", ctx->percentage);
    lv_label_set_text(ctx->value, value);

    /* Rebuilding/selecting the dropdown can emit VALUE_CHANGED. Suppress
     * those programmatic events so an HA state refresh never calls a service. */
    ctx->suppress = true;
    if (ctx->preset_count > 0) {
        lv_obj_clear_flag(ctx->preset, LV_OBJ_FLAG_HIDDEN);
        lv_dropdown_clear_options(ctx->preset);
        int selected = 0;
        for (int i = 0; i < ctx->preset_count; i++) {
            lv_dropdown_add_option(ctx->preset, ctx->preset_modes[i], LV_DROPDOWN_POS_LAST);
            if (strcmp(ctx->preset_modes[i], ctx->preset_mode) == 0) {
                selected = i;
            }
        }
        lv_dropdown_set_selected(ctx->preset, selected);
    } else {
        lv_obj_add_flag(ctx->preset, LV_OBJ_FLAG_HIDDEN);
    }

    if (ctx->supports_percentage) {
        lv_obj_clear_flag(ctx->slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ctx->value, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctx->slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctx->value, LV_OBJ_FLAG_HIDDEN);
    }

    /* Unavailable entities should look and behave unavailable. Keep the
     * controls visible so the tile layout does not jump during reconnects. */
    if (ctx->unavailable) {
        lv_obj_add_state(ctx->power, LV_STATE_DISABLED);
        lv_obj_add_state(ctx->slider, LV_STATE_DISABLED);
        lv_obj_add_state(ctx->preset, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(ctx->power, LV_STATE_DISABLED);
        lv_obj_remove_state(ctx->slider, LV_STATE_DISABLED);
        lv_obj_remove_state(ctx->preset, LV_STATE_DISABLED);
    }

    ctx->suppress = true;
    if (ctx->is_on) lv_obj_add_state(ctx->power, LV_STATE_CHECKED);
    else lv_obj_remove_state(ctx->power, LV_STATE_CHECKED);
    lv_slider_set_value(ctx->slider, ctx->percentage, LV_ANIM_OFF);
    ctx->suppress = false;
}

static void fan_power_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    w_fan_ctx_t *ctx = lv_event_get_user_data(event);
    if (ctx == NULL || ctx->suppress || ctx->unavailable) return;
    bool on = lv_obj_has_state(ctx->power, LV_STATE_CHECKED);
    if (ui_bindings_set_entity_power(ctx->entity_id, on) == ESP_OK) {
        ctx->is_on = on;
        fan_apply_visual(ctx);
    }
}

static void fan_slider_event(lv_event_t *event)
{
    w_fan_ctx_t *ctx = lv_event_get_user_data(event);
    if (ctx == NULL || ctx->suppress || ctx->unavailable || !ctx->supports_percentage) return;
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
        ctx->percentage = clamp_percent(lv_slider_get_value(ctx->slider));
        fan_apply_visual(ctx);
    } else if (code == LV_EVENT_RELEASED) {
        int value = clamp_percent(lv_slider_get_value(ctx->slider));
        if (ui_bindings_set_slider_value(ctx->entity_id, value) == ESP_OK) {
            ctx->percentage = value;
            ctx->is_on = value > 0;
            fan_apply_visual(ctx);
        }
    }
}

static void fan_preset_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    w_fan_ctx_t *ctx = lv_event_get_user_data(event);
    if (ctx == NULL || ctx->suppress || ctx->unavailable || ctx->preset_count <= 0) return;
    uint32_t selected = lv_dropdown_get_selected(ctx->preset);
    if (selected >= (uint32_t)ctx->preset_count) return;
    if (ui_bindings_set_fan_preset_mode(ctx->entity_id, ctx->preset_modes[selected]) == ESP_OK) {
        snprintf(ctx->preset_mode, sizeof(ctx->preset_mode), "%s", ctx->preset_modes[selected]);
    }
}

static void fan_delete_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_DELETE) free(lv_event_get_user_data(event));
}

esp_err_t w_fan_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out)
{
    if (def == NULL || parent == NULL || out == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, APP_UI_CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);

    w_fan_ctx_t *ctx = ui_calloc_prefer_psram(1, sizeof(*ctx));
    if (ctx == NULL) { lv_obj_del(card); return ESP_ERR_NO_MEM; }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->card = card;
    ctx->show_title = def->show_title;
    ctx->show_state = def->show_state;

    ctx->title = lv_label_create(card);
    lv_label_set_text(ctx->title, def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_font(ctx->title, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(ctx->title, LV_ALIGN_BOTTOM_MID, 0, -8);
    if (!ctx->show_title) {
        lv_obj_add_flag(ctx->title, LV_OBJ_FLAG_HIDDEN);
    }

    ctx->state = lv_label_create(card);
    lv_obj_set_style_text_font(ctx->state, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(ctx->state, LV_ALIGN_TOP_LEFT, 0, 2);
    if (!ctx->show_state) {
        lv_obj_add_flag(ctx->state, LV_OBJ_FLAG_HIDDEN);
    }

    ctx->value = lv_label_create(card);
    lv_obj_set_style_text_font(ctx->value, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(ctx->value, LV_ALIGN_TOP_RIGHT, 0, 2);

    ctx->power = lv_switch_create(card);
    lv_obj_set_size(ctx->power, 70, 34);
    lv_obj_align(ctx->power, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_event_cb(ctx->power, fan_power_event, LV_EVENT_VALUE_CHANGED, ctx);

    ctx->slider = lv_slider_create(card);
    lv_slider_set_range(ctx->slider, 0, 100);
    lv_obj_set_size(ctx->slider, def->w - 44, 18);
    lv_obj_align(ctx->slider, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(ctx->slider, fan_slider_event, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(ctx->slider, fan_slider_event, LV_EVENT_RELEASED, ctx);

    ctx->preset = lv_dropdown_create(card);
    lv_obj_set_width(ctx->preset, def->w - 44);
    lv_obj_align(ctx->preset, LV_ALIGN_CENTER, 0, 66);
    lv_obj_add_flag(ctx->preset, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ctx->preset, fan_preset_event, LV_EVENT_VALUE_CHANGED, ctx);

    lv_obj_add_event_cb(card, fan_delete_event, LV_EVENT_DELETE, ctx);

    fan_apply_visual(ctx);
    out->obj = card;
    out->ctx = ctx;
    return ESP_OK;
}

void w_fan_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL) return;
    w_fan_ctx_t *ctx = instance->ctx;
    if (ctx == NULL) return;
    ctx->unavailable = strcmp(state->state, "unavailable") == 0 || strcmp(state->state, "unknown") == 0;
    ctx->is_on = strcmp(state->state, "on") == 0;
    ctx->supports_percentage = false;
    ctx->percentage = ctx->is_on ? 100 : 0;
    ctx->preset_count = 0;
    ctx->preset_mode[0] = '\0';

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *preset = cJSON_GetObjectItemCaseSensitive(attrs, "preset_mode");
        if (cJSON_IsString(preset) && preset->valuestring != NULL) {
            snprintf(ctx->preset_mode, sizeof(ctx->preset_mode), "%s", preset->valuestring);
        }
        cJSON *modes = cJSON_GetObjectItemCaseSensitive(attrs, "preset_modes");
        if (cJSON_IsArray(modes)) {
            cJSON *mode = NULL;
            cJSON_ArrayForEach(mode, modes) {
                if (ctx->preset_count >= 16) break;
                if (cJSON_IsString(mode) && mode->valuestring != NULL) {
                    snprintf(ctx->preset_modes[ctx->preset_count],
                        sizeof(ctx->preset_modes[ctx->preset_count]), "%s", mode->valuestring);
                    ctx->preset_count++;
                }
            }
        }

        cJSON *pct = cJSON_GetObjectItemCaseSensitive(attrs, "percentage");
        if (cJSON_IsNumber(pct)) {
            ctx->supports_percentage = true;
            ctx->percentage = clamp_percent((int)(pct->valuedouble + 0.5));
        }
        cJSON_Delete(attrs);
    }
    fan_apply_visual(ctx);
}

void w_fan_tile_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL) return;
    w_fan_ctx_t *ctx = instance->ctx;
    if (ctx == NULL) return;
    ctx->unavailable = true;
    ctx->is_on = false;
    fan_apply_visual(ctx);
}
