/* SPDX-License-Identifier: LicenseRef-FNCL-1.1 */
#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "ha/ha_cover_fetcher.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"

#define W_IMAGE_URL_LEN 512

typedef struct {
    lv_obj_t *card, *image, *placeholder, *title;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char url[W_IMAGE_URL_LEN];
    lv_image_dsc_t dsc;
    bool unavailable;
} w_image_ctx_t;

static void release_image(w_image_ctx_t *c)
{
    if (c == NULL || c->dsc.data == NULL) return;
    lv_image_set_src(c->image, NULL);
    heap_caps_free((void *)c->dsc.data);
    memset(&c->dsc, 0, sizeof(c->dsc));
}

static void show_placeholder(w_image_ctx_t *c, const char *text)
{
    if (c == NULL) return;
    lv_label_set_text(c->placeholder, text != NULL ? text : "");
    lv_obj_remove_flag(c->placeholder, LV_OBJ_FLAG_HIDDEN);
}

static void image_cb(void *user, const ha_cover_result_t *result)
{
    w_image_ctx_t *c = user;
    if (c == NULL) {
        if (result != NULL && result->valid && result->image.data != NULL) {
            ha_cover_result_t owned = *result; ha_cover_result_release(&owned);
        }
        return;
    }
    if (result == NULL || !result->valid || result->image.data == NULL) {
        release_image(c);
        show_placeholder(c, "Image unavailable");
        return;
    }
    release_image(c);
    c->dsc = result->image;
    lv_image_set_src(c->image, &c->dsc);
    lv_image_set_inner_align(c->image, LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_add_flag(c->placeholder, LV_OBJ_FLAG_HIDDEN);
}

static void request_image(w_image_ctx_t *c)
{
    if (c == NULL || c->unavailable || c->url[0] == '\0') return;
    lv_coord_t w = lv_obj_get_width(c->card) - 20;
    lv_coord_t h = lv_obj_get_height(c->card) - 20;
    if (ha_cover_fetcher_request(c->url, w > 1 ? w : 1, h > 1 ? h : 1, image_cb, c) != ESP_OK) {
        show_placeholder(c, "Image unavailable");
    }
}

static void delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    w_image_ctx_t *c = lv_event_get_user_data(e);
    if (c == NULL) return;
    ha_cover_fetcher_cancel(c);
    release_image(c);
    free(c);
}

esp_err_t w_image_create(const ui_widget_def_t *d, lv_obj_t *p, ui_widget_instance_t *o)
{
    if (d == NULL || p == NULL || o == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_pos(card, d->x, d->y); lv_obj_set_size(card, d->w, d->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);
    w_image_ctx_t *c = ui_calloc_prefer_psram(1, sizeof(*c));
    if (c == NULL) { lv_obj_delete(card); return ESP_ERR_NO_MEM; }
    c->card = card; snprintf(c->entity_id, sizeof(c->entity_id), "%s", d->entity_id);
    c->image = lv_image_create(card); lv_obj_set_size(c->image, LV_PCT(100), LV_PCT(100)); lv_obj_center(c->image);
    c->placeholder = lv_label_create(card); lv_obj_set_width(c->placeholder, LV_PCT(90));
    lv_obj_set_style_text_align(c->placeholder, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN); lv_obj_center(c->placeholder);
    c->title = lv_label_create(card); lv_label_set_text(c->title, d->title[0] ? d->title : d->id);
    lv_obj_set_style_text_font(c->title, APP_FONT_TEXT_14, LV_PART_MAIN); lv_obj_align(c->title, LV_ALIGN_TOP_LEFT, 8, 6);
    if (!d->show_title) lv_obj_add_flag(c->title, LV_OBJ_FLAG_HIDDEN);
    show_placeholder(c, "Waiting for image");
    lv_obj_add_event_cb(card, delete_cb, LV_EVENT_DELETE, c);
    o->obj = card; o->ctx = c; return ESP_OK;
}

void w_image_apply_state(ui_widget_instance_t *i, const ha_state_t *s)
{
    if (i == NULL || s == NULL || i->ctx == NULL) return;
    w_image_ctx_t *c = i->ctx;
    c->unavailable = strcmp(s->state, "unavailable") == 0 || strcmp(s->state, "unknown") == 0;
    if (c->unavailable) { ha_cover_fetcher_cancel(c); release_image(c); show_placeholder(c, ui_i18n_get("common.unavailable", "Unavailable")); return; }
    char next[W_IMAGE_URL_LEN] = {0};
    cJSON *a = cJSON_Parse(s->attributes_json);
    if (a != NULL) {
        cJSON *pic = cJSON_GetObjectItemCaseSensitive(a, "entity_picture");
        if (cJSON_IsString(pic) && pic->valuestring != NULL) snprintf(next, sizeof(next), "%s", pic->valuestring);
        cJSON_Delete(a);
    }
    if (next[0] == '\0') {
        snprintf(next, sizeof(next), "/api/image_proxy/%s?state=%s", c->entity_id, s->state);
    }
    if (strcmp(next, c->url) != 0 || c->dsc.data == NULL) {
        snprintf(c->url, sizeof(c->url), "%s", next);
        ha_cover_fetcher_cancel(c);
        request_image(c);
    }
}

void w_image_mark_unavailable(ui_widget_instance_t *i)
{
    if (i == NULL || i->ctx == NULL) return;
    w_image_ctx_t *c = i->ctx; c->unavailable = true; ha_cover_fetcher_cancel(c); release_image(c);
    show_placeholder(c, ui_i18n_get("common.unavailable", "Unavailable"));
}
