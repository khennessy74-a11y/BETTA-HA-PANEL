/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_err.h"
#include "lvgl.h"
#include "app_config.h"
#include "ha/ha_model.h"
#include "layout/layout_schema.h"
#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_runtime.h"
#include "ui/ui_widget_factory.h"
#include "ui/theme/theme_default.h"

#define SELECT_MAX_OPTIONS 32
#define SELECT_OPTION_LEN 64

typedef struct {
    lv_obj_t *card, *title, *value, *prev_btn, *next_btn;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char options[SELECT_MAX_OPTIONS][SELECT_OPTION_LEN];
    int option_count, current_index;
    bool unavailable, show_title, show_state;
} w_select_ctx_t;

static void apply_visual(w_select_ctx_t *c)
{
    if (c == NULL) return;
    const char *text = c->unavailable ? ui_i18n_get("common.unavailable", "unavailable") :
        (c->current_index >= 0 && c->current_index < c->option_count ? c->options[c->current_index] : "-");
    lv_label_set_text(c->value, text);
    bool enabled = !c->unavailable && c->option_count > 1;
    if (enabled) {
        lv_obj_remove_state(c->prev_btn, LV_STATE_DISABLED);
        lv_obj_remove_state(c->next_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(c->prev_btn, LV_STATE_DISABLED);
        lv_obj_add_state(c->next_btn, LV_STATE_DISABLED);
    }
}

static void action_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    w_select_ctx_t *c = lv_event_get_user_data(e);
    if (c == NULL || c->unavailable || c->option_count < 1) return;
    int next = c->current_index;
    if (next < 0) next = 0;
    else if (lv_event_get_target(e) == c->prev_btn) next = (next + c->option_count - 1) % c->option_count;
    else next = (next + 1) % c->option_count;
    if (ui_bindings_select_option(c->entity_id, c->options[next]) == ESP_OK) {
        c->current_index = next;
        apply_visual(c);
    }
}

static void delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE) free(lv_event_get_user_data(e));
}

esp_err_t w_select_create(const ui_widget_def_t *d, lv_obj_t *p, ui_widget_instance_t *o)
{
    if (d == NULL || p == NULL || o == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_pos(card, d->x, d->y);
    lv_obj_set_size(card, d->w, d->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, APP_UI_CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
    w_select_ctx_t *c = ui_calloc_prefer_psram(1, sizeof(*c));
    if (c == NULL) { lv_obj_del(card); return ESP_ERR_NO_MEM; }
    c->card = card; c->current_index = -1;
    c->show_title = d->show_title; c->show_state = d->show_state;
    snprintf(c->entity_id, sizeof(c->entity_id), "%s", d->entity_id);

    c->title = lv_label_create(card);
    lv_label_set_text(c->title, d->title[0] ? d->title : d->id);
    lv_obj_set_style_text_font(c->title, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(c->title, LV_ALIGN_TOP_MID, 0, 2);
    if (!c->show_title) lv_obj_add_flag(c->title, LV_OBJ_FLAG_HIDDEN);

    c->value = lv_label_create(card);
    lv_obj_set_width(c->value, d->w > 100 ? d->w - 100 : (d->w > 20 ? d->w - 20 : 1));
    lv_label_set_long_mode(c->value, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(c->value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(c->value, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_align(c->value, LV_ALIGN_CENTER, 0, 8);
    if (!c->show_state) lv_obj_add_flag(c->value, LV_OBJ_FLAG_HIDDEN);

    c->prev_btn = lv_btn_create(card);
    lv_obj_set_size(c->prev_btn, 42, 42); lv_obj_align(c->prev_btn, LV_ALIGN_LEFT_MID, 0, 8);
    lv_obj_t *pl = lv_label_create(c->prev_btn); lv_label_set_text(pl, "<"); lv_obj_center(pl);
    c->next_btn = lv_btn_create(card);
    lv_obj_set_size(c->next_btn, 42, 42); lv_obj_align(c->next_btn, LV_ALIGN_RIGHT_MID, 0, 8);
    lv_obj_t *nl = lv_label_create(c->next_btn); lv_label_set_text(nl, ">"); lv_obj_center(nl);
    lv_obj_add_event_cb(c->prev_btn, action_cb, LV_EVENT_CLICKED, c);
    lv_obj_add_event_cb(c->next_btn, action_cb, LV_EVENT_CLICKED, c);
    lv_obj_add_event_cb(card, delete_cb, LV_EVENT_DELETE, c);
    apply_visual(c); o->obj = card; o->ctx = c; return ESP_OK;
}

void w_select_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL) return;
    w_select_ctx_t *c = instance->ctx; if (c == NULL) return;
    c->unavailable = strcmp(state->state, "unavailable") == 0 || strcmp(state->state, "unknown") == 0;
    c->option_count = 0; c->current_index = -1;
    cJSON *attrs = cJSON_Parse(state->attributes_json);
    cJSON *opts = attrs ? cJSON_GetObjectItemCaseSensitive(attrs, "options") : NULL;
    if (cJSON_IsArray(opts)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, opts) {
            if (c->option_count >= SELECT_MAX_OPTIONS) break;
            if (cJSON_IsString(item) && item->valuestring) {
                snprintf(c->options[c->option_count], SELECT_OPTION_LEN, "%s", item->valuestring);
                if (strcmp(state->state, c->options[c->option_count]) == 0) c->current_index = c->option_count;
                c->option_count++;
            }
        }
    }
    if (attrs) cJSON_Delete(attrs);
    apply_visual(c);
}

void w_select_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance && instance->ctx) {
        w_select_ctx_t *c = instance->ctx;
        c->unavailable = true;
        apply_visual(c);
    }
}
