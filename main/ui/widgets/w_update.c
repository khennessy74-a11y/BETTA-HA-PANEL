/* SPDX-License-Identifier: LicenseRef-FNCL-1.1 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_err.h"
#include "lvgl.h"
#include "app_config.h"
#include "ha/ha_model.h"
#include "layout/layout_schema.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"
#include "ui/ui_widget_factory.h"

#define UPDATE_VERSION_LEN 64

typedef struct {
    lv_obj_t *card, *title, *state_label, *version_label, *install_btn;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char installed[UPDATE_VERSION_LEN];
    char latest[UPDATE_VERSION_LEN];
    bool unavailable, update_available, in_progress, skipped, show_title, show_state;
} w_update_ctx_t;

static void install_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    w_update_ctx_t *c = lv_event_get_user_data(e);
    if (c == NULL || c->unavailable || !c->update_available || c->in_progress) return;
    (void)ui_bindings_install_update(c->entity_id);
}

static void refresh(w_update_ctx_t *c)
{
    if (c == NULL) return;
    const char *state = c->unavailable ? ui_i18n_get("common.unavailable", "Unavailable") :
        (c->in_progress ? "Installing..." : (c->update_available ? (c->skipped ? "Update skipped" : "Update available") : "Up to date"));
    lv_label_set_text(c->state_label, state);
    char versions[144];
    if (c->installed[0] && c->latest[0]) {
        snprintf(versions, sizeof(versions), "%s -> %s", c->installed, c->latest);
    } else if (c->latest[0]) {
        snprintf(versions, sizeof(versions), "Latest: %s", c->latest);
    } else if (c->installed[0]) {
        snprintf(versions, sizeof(versions), "Installed: %s", c->installed);
    } else {
        versions[0] = '\0';
    }
    lv_label_set_text(c->version_label, versions);
    if (c->unavailable || !c->update_available || c->in_progress) lv_obj_add_state(c->install_btn, LV_STATE_DISABLED);
    else lv_obj_remove_state(c->install_btn, LV_STATE_DISABLED);
}

static void delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE) free(lv_event_get_user_data(e));
}

esp_err_t w_update_create(const ui_widget_def_t *d, lv_obj_t *p, ui_widget_instance_t *o)
{
    if (d == NULL || p == NULL || o == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_pos(card, d->x, d->y); lv_obj_set_size(card, d->w, d->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    w_update_ctx_t *c = ui_calloc_prefer_psram(1, sizeof(*c));
    if (c == NULL) { lv_obj_delete(card); return ESP_ERR_NO_MEM; }
    c->card = card; c->show_title = d->show_title; c->show_state = d->show_state;
    snprintf(c->entity_id, sizeof(c->entity_id), "%s", d->entity_id);
    c->title = lv_label_create(card); lv_label_set_text(c->title, d->title[0] ? d->title : d->id);
    lv_obj_set_style_text_font(c->title, APP_FONT_TEXT_20, LV_PART_MAIN); lv_obj_align(c->title, LV_ALIGN_TOP_MID, 0, 0);
    if (!c->show_title) lv_obj_add_flag(c->title, LV_OBJ_FLAG_HIDDEN);
    c->state_label = lv_label_create(card); lv_label_set_text(c->state_label, "");
    lv_obj_set_style_text_font(c->state_label, APP_FONT_TEXT_20, LV_PART_MAIN); lv_obj_align(c->state_label, LV_ALIGN_TOP_MID, 0, 34);
    if (!c->show_state) lv_obj_add_flag(c->state_label, LV_OBJ_FLAG_HIDDEN);
    c->version_label = lv_label_create(card); lv_label_set_text(c->version_label, "");
    lv_label_set_long_mode(c->version_label, LV_LABEL_LONG_DOT); lv_obj_set_width(c->version_label, LV_PCT(100));
    lv_obj_set_style_text_align(c->version_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN); lv_obj_align(c->version_label, LV_ALIGN_CENTER, 0, 12);
    c->install_btn = lv_btn_create(card); lv_obj_set_size(c->install_btn, 120, 42); lv_obj_align(c->install_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *label = lv_label_create(c->install_btn); lv_label_set_text(label, "Install"); lv_obj_center(label);
    lv_obj_add_event_cb(c->install_btn, install_cb, LV_EVENT_CLICKED, c);
    lv_obj_add_event_cb(card, delete_cb, LV_EVENT_DELETE, c);
    c->unavailable = true; refresh(c); o->obj = card; o->ctx = c; return ESP_OK;
}

void w_update_apply_state(ui_widget_instance_t *i, const ha_state_t *s)
{
    if (i == NULL || s == NULL || i->ctx == NULL) return;
    w_update_ctx_t *c = i->ctx;
    c->unavailable = strcmp(s->state, "unavailable") == 0 || strcmp(s->state, "unknown") == 0;
    c->update_available = strcmp(s->state, "on") == 0;
    c->in_progress = false;
    c->skipped = false;
    c->installed[0] = '\0'; c->latest[0] = '\0';
    cJSON *a = cJSON_Parse(s->attributes_json);
    if (a != NULL) {
        cJSON *installed = cJSON_GetObjectItemCaseSensitive(a, "installed_version");
        cJSON *latest = cJSON_GetObjectItemCaseSensitive(a, "latest_version");
        cJSON *progress = cJSON_GetObjectItemCaseSensitive(a, "in_progress");
        cJSON *skipped = cJSON_GetObjectItemCaseSensitive(a, "skipped_version");
        if (cJSON_IsString(installed) && installed->valuestring) snprintf(c->installed, sizeof(c->installed), "%s", installed->valuestring);
        if (cJSON_IsString(latest) && latest->valuestring) snprintf(c->latest, sizeof(c->latest), "%s", latest->valuestring);
        c->skipped = cJSON_IsString(skipped) && skipped->valuestring != NULL && c->latest[0] != '\\0' && strcmp(skipped->valuestring, c->latest) == 0;
        if (cJSON_IsBool(progress)) {
            c->in_progress = cJSON_IsTrue(progress);
        } else if (cJSON_IsNumber(progress)) {
            c->in_progress = progress->valuedouble > 0.0 && progress->valuedouble < 100.0;
        } else if (cJSON_IsString(progress) && progress->valuestring != NULL) {
            c->in_progress = progress->valuestring[0] != '\0' &&
                strcmp(progress->valuestring, "0") != 0 &&
                strcmp(progress->valuestring, "100") != 0 &&
                strcmp(progress->valuestring, "false") != 0;
        }
        cJSON_Delete(a);
    }
    refresh(c);
}

void w_update_mark_unavailable(ui_widget_instance_t *i)
{
    if (i != NULL && i->ctx != NULL) { w_update_ctx_t *c = i->ctx; c->unavailable = true; refresh(c); }
}
