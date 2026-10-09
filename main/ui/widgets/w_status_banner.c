/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ui/ui_widget_factory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"

typedef struct {
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *state;
} w_status_banner_ctx_t;

static bool status_banner_is_inactive(const char *state)
{
    if (state == NULL) return true;
    return strcmp(state, "off") == 0 ||
           strcmp(state, "closed") == 0 ||
           strcmp(state, "idle") == 0 ||
           strcmp(state, "clear") == 0 ||
           strcmp(state, "ok") == 0 ||
           strcmp(state, "home") == 0;
}

static uint32_t status_banner_severity_color(const char *severity)
{
    if (severity != NULL && strcmp(severity, "critical") == 0) {
        return APP_UI_COLOR_ERROR;
    }
    if (severity != NULL && strcmp(severity, "warning") == 0) {
        return APP_UI_COLOR_HEAT_ICON_ON;
    }
    if (severity != NULL && strcmp(severity, "success") == 0) {
        return APP_UI_COLOR_OK;
    }
    return APP_UI_COLOR_STATE_ON;
}

static void status_banner_apply_visual(
    w_status_banner_ctx_t *ctx,
    bool active,
    const char *severity)
{
    if (ctx == NULL || ctx->card == NULL) {
        return;
    }

    uint32_t accent = status_banner_severity_color(severity);

    lv_obj_set_style_bg_color(
        ctx->card,
        lv_color_hex(active ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF),
        LV_PART_MAIN);
    lv_obj_set_style_border_color(
        ctx->card,
        lv_color_hex(active ? accent : APP_UI_COLOR_CARD_BORDER),
        LV_PART_MAIN);

    if (ctx->state != NULL) {
        lv_obj_set_style_text_color(
            ctx->state,
            lv_color_hex(active ? accent : APP_UI_COLOR_TEXT_PRIMARY),
            LV_PART_MAIN);
    }
}

esp_err_t w_status_banner_create(
    const ui_widget_def_t *def,
    lv_obj_t *parent,
    ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    w_status_banner_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) return ESP_ERR_NO_MEM;

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(card, 12, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->entity_id);
    lv_obj_set_width(title, LV_PCT(58));
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(title, APP_FONT_TEXT_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *state = lv_label_create(card);
    lv_label_set_text(state, "--");
    lv_obj_set_width(state, LV_PCT(38));
    lv_label_set_long_mode(state, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(state, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_set_style_text_align(state, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align(state, LV_ALIGN_RIGHT_MID, 0, 0);

    ctx->card = card;
    ctx->title = title;
    ctx->state = state;
    status_banner_apply_visual(ctx, false, def->status_severity);

    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_status_banner_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL || instance->ctx == NULL) return;
    w_status_banner_ctx_t *ctx = (w_status_banner_ctx_t *)instance->ctx;

    char text[128] = {0};
    const char *unit = "";
    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(attrs, "unit_of_measurement");
        if (cJSON_IsString(unit_item) && unit_item->valuestring != NULL) {
            unit = unit_item->valuestring;
        }
    }
    snprintf(text, sizeof(text), "%s%s%s", state->state, unit[0] ? " " : "", unit);
    lv_label_set_text(ctx->state, text);
    status_banner_apply_visual(ctx, !status_banner_is_inactive(state->state), instance->status_severity);
    if (attrs != NULL) cJSON_Delete(attrs);
}

void w_status_banner_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->ctx == NULL) return;
    w_status_banner_ctx_t *ctx = (w_status_banner_ctx_t *)instance->ctx;
    lv_label_set_text(ctx->state, "unavailable");
    status_banner_apply_visual(ctx, false, instance->status_severity);
}
