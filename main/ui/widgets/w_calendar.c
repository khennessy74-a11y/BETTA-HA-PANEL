/* SPDX-License-Identifier: LicenseRef-FNCL-1.1 */
#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"

typedef struct {
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *message;
    lv_obj_t *time;
    bool show_title;
    bool show_state;
} w_calendar_ctx_t;

static void calendar_set_text(lv_obj_t *label, const char *text)
{
    lv_label_set_text(label, (text != NULL && text[0] != '\0') ? text : "--");
}

esp_err_t w_calendar_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, APP_UI_CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 14, LV_PART_MAIN);

    w_calendar_ctx_t *ctx = ui_calloc_prefer_psram(1, sizeof(*ctx));
    if (ctx == NULL) { lv_obj_del(card); return ESP_ERR_NO_MEM; }
    ctx->card = card; ctx->show_title = def->show_title; ctx->show_state = def->show_state;

    ctx->title = lv_label_create(card);
    lv_label_set_text(ctx->title, def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_font(ctx->title, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(ctx->title, LV_ALIGN_TOP_LEFT, 0, 0);
    if (!ctx->show_title) lv_obj_add_flag(ctx->title, LV_OBJ_FLAG_HIDDEN);

    ctx->message = lv_label_create(card);
    lv_obj_set_width(ctx->message, def->w > 32 ? def->w - 32 : 1);
    lv_label_set_long_mode(ctx->message, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(ctx->message, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(ctx->message, LV_ALIGN_CENTER, 0, -4);

    ctx->time = lv_label_create(card);
    lv_obj_set_width(ctx->time, def->w > 32 ? def->w - 32 : 1);
    lv_label_set_long_mode(ctx->time, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(ctx->time, APP_FONT_TEXT_14, LV_PART_MAIN);
    lv_obj_align(ctx->time, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    if (!ctx->show_state) { lv_obj_add_flag(ctx->message, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(ctx->time, LV_OBJ_FLAG_HIDDEN); }

    calendar_set_text(ctx->message, "--");
    calendar_set_text(ctx->time, "");
    out_instance->obj = card; out_instance->ctx = ctx;
    return ESP_OK;
}

void w_calendar_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL || instance->ctx == NULL) return;
    w_calendar_ctx_t *ctx = instance->ctx;
    bool unavailable = strcmp(state->state, "unavailable") == 0 || strcmp(state->state, "unknown") == 0;
    if (unavailable) {
        calendar_set_text(ctx->message, ui_i18n_get("common.unavailable", "Unavailable"));
        calendar_set_text(ctx->time, "");
        return;
    }

    const char *message = NULL, *start = NULL, *end = NULL;
    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(attrs, "message");
        if (cJSON_IsString(item)) message = item->valuestring;
        item = cJSON_GetObjectItemCaseSensitive(attrs, "start_time");
        if (cJSON_IsString(item)) start = item->valuestring;
        item = cJSON_GetObjectItemCaseSensitive(attrs, "end_time");
        if (cJSON_IsString(item)) end = item->valuestring;
        calendar_set_text(ctx->message, message != NULL ? message : state->state);
        char range[160] = {0};
        if (start != NULL && end != NULL) snprintf(range, sizeof(range), "%.76s - %.76s", start, end);
        else if (start != NULL) snprintf(range, sizeof(range), "%.159s", start);
        calendar_set_text(ctx->time, range);
        cJSON_Delete(attrs);
    } else {
        calendar_set_text(ctx->message, state->state);
        calendar_set_text(ctx->time, "");
    }
}

void w_calendar_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->ctx == NULL) return;
    w_calendar_ctx_t *ctx = instance->ctx;
    calendar_set_text(ctx->message, ui_i18n_get("common.unavailable", "Unavailable"));
    calendar_set_text(ctx->time, "");
}
