/* SPDX-License-Identifier: LicenseRef-FNCL-1.1 */
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
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"
#include "ui/ui_widget_factory.h"

#define ALARM_CODE_LEN 32
#define ALARM_SUPPORT_ARM_HOME 1
#define ALARM_SUPPORT_ARM_AWAY 2
#define ALARM_SUPPORT_ARM_NIGHT 4
#define ALARM_SUPPORT_TRIGGER 8
#define ALARM_SUPPORT_ARM_CUSTOM_BYPASS 16
#define ALARM_SUPPORT_ARM_VACATION 32

typedef struct {
    lv_obj_t *card, *title, *state_label, *actions;
    lv_obj_t *overlay, *code_area, *keyboard;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char pending_service[32];
    uint32_t supported_features;
    bool unavailable, code_required, show_title, show_state;
} w_alarm_control_ctx_t;

static void close_code(w_alarm_control_ctx_t *c)
{
    if (c == NULL || c->overlay == NULL) return;
    lv_obj_delete(c->overlay);
    c->overlay = NULL; c->code_area = NULL; c->keyboard = NULL;
}

static void send_service(w_alarm_control_ctx_t *c, const char *service, const char *code)
{
    if (c == NULL || service == NULL || c->unavailable) return;
    (void)ui_bindings_alarm_control(c->entity_id, service, code);
}

static void code_keyboard_cb(lv_event_t *e)
{
    w_alarm_control_ctx_t *c = lv_event_get_user_data(e);
    if (c == NULL) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        const char *pin = c->code_area ? lv_textarea_get_text(c->code_area) : "";
        if (pin[0] != '\0') {
            send_service(c, c->pending_service, pin);
            close_code(c);
        }
    } else if (code == LV_EVENT_CANCEL) {
        close_code(c);
    }
}

static void request_code(w_alarm_control_ctx_t *c, const char *service)
{
    if (c == NULL || c->overlay != NULL) return;
    snprintf(c->pending_service, sizeof(c->pending_service), "%s", service);
    c->overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(c->overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(c->overlay, 0, 0);
    lv_obj_set_style_pad_all(c->overlay, 10, LV_PART_MAIN);
    c->code_area = lv_textarea_create(c->overlay);
    lv_obj_set_size(c->code_area, LV_PCT(80), 70);
    lv_obj_align(c->code_area, LV_ALIGN_TOP_MID, 0, 8);
    lv_textarea_set_one_line(c->code_area, true);
    lv_textarea_set_password_mode(c->code_area, true);
    lv_textarea_set_max_length(c->code_area, ALARM_CODE_LEN - 1);
    lv_textarea_set_accepted_chars(c->code_area, "0123456789");
    c->keyboard = lv_keyboard_create(c->overlay);
    lv_keyboard_set_mode(c->keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(c->keyboard, LV_PCT(100), LV_PCT(68));
    lv_obj_align(c->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(c->keyboard, c->code_area);
    lv_obj_add_event_cb(c->keyboard, code_keyboard_cb, LV_EVENT_READY, c);
    lv_obj_add_event_cb(c->keyboard, code_keyboard_cb, LV_EVENT_CANCEL, c);
}

static void action_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    w_alarm_control_ctx_t *c = lv_event_get_user_data(e);
    const char *service = lv_obj_get_user_data(btn);
    if (c == NULL || service == NULL || c->unavailable) return;
    if (c->code_required) request_code(c, service);
    else send_service(c, service, NULL);
}

static void add_action(w_alarm_control_ctx_t *c, const char *label, const char *service)
{
    lv_obj_t *btn = lv_btn_create(c->actions);
    lv_obj_set_size(btn, 86, 38);
    lv_obj_set_user_data(btn, (void *)service);
    lv_obj_t *text = lv_label_create(btn);
    lv_label_set_text(text, label); lv_obj_center(text);
    lv_obj_add_event_cb(btn, action_cb, LV_EVENT_CLICKED, c);
}

static void rebuild_actions(w_alarm_control_ctx_t *c)
{
    lv_obj_clean(c->actions);
    add_action(c, "Disarm", "alarm_disarm");
    if (c->supported_features & ALARM_SUPPORT_ARM_HOME) add_action(c, "Home", "alarm_arm_home");
    if (c->supported_features & ALARM_SUPPORT_ARM_AWAY) add_action(c, "Away", "alarm_arm_away");
    if (c->supported_features & ALARM_SUPPORT_ARM_NIGHT) add_action(c, "Night", "alarm_arm_night");
    if (c->supported_features & ALARM_SUPPORT_ARM_VACATION) add_action(c, "Vacation", "alarm_arm_vacation");
    if (c->supported_features & ALARM_SUPPORT_ARM_CUSTOM_BYPASS) add_action(c, "Custom", "alarm_arm_custom_bypass");
}

static void apply_visual(w_alarm_control_ctx_t *c, const char *state)
{
    if (c == NULL) return;
    lv_label_set_text(c->state_label, c->unavailable ? ui_i18n_get("common.unavailable", "unavailable") : state);
    uint32_t count = lv_obj_get_child_count(c->actions);
    for (uint32_t idx = 0; idx < count; ++idx) {
        lv_obj_t *btn = lv_obj_get_child(c->actions, idx);
        if (c->unavailable) lv_obj_add_state(btn, LV_STATE_DISABLED);
        else lv_obj_remove_state(btn, LV_STATE_DISABLED);
    }
}

static void delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    w_alarm_control_ctx_t *c = lv_event_get_user_data(e);
    if (c != NULL) { close_code(c); free(c); }
}

esp_err_t w_alarm_control_create(const ui_widget_def_t *d, lv_obj_t *p, ui_widget_instance_t *o)
{
    if (d == NULL || p == NULL || o == NULL) return ESP_ERR_INVALID_ARG;
    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_pos(card, d->x, d->y); lv_obj_set_size(card, d->w, d->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, APP_UI_CARD_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    w_alarm_control_ctx_t *c = ui_calloc_prefer_psram(1, sizeof(*c));
    if (c == NULL) { lv_obj_delete(card); return ESP_ERR_NO_MEM; }
    c->card = card; c->show_title = d->show_title; c->show_state = d->show_state;
    snprintf(c->entity_id, sizeof(c->entity_id), "%s", d->entity_id);
    c->title = lv_label_create(card); lv_label_set_text(c->title, d->title[0] ? d->title : d->id);
    lv_obj_set_style_text_font(c->title, APP_FONT_TEXT_20, LV_PART_MAIN); lv_obj_align(c->title, LV_ALIGN_TOP_MID, 0, 0);
    if (!c->show_title) lv_obj_add_flag(c->title, LV_OBJ_FLAG_HIDDEN);
    c->state_label = lv_label_create(card); lv_label_set_text(c->state_label, "");
    lv_obj_set_style_text_font(c->state_label, APP_FONT_TEXT_20, LV_PART_MAIN); lv_obj_align(c->state_label, LV_ALIGN_TOP_MID, 0, 34);
    if (!c->show_state) lv_obj_add_flag(c->state_label, LV_OBJ_FLAG_HIDDEN);
    c->actions = lv_obj_create(card); lv_obj_set_width(c->actions, LV_PCT(100)); lv_obj_set_height(c->actions, LV_SIZE_CONTENT);
    lv_obj_align(c->actions, LV_ALIGN_BOTTOM_MID, 0, 0); lv_obj_set_flex_flow(c->actions, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(c->actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(c->actions, LV_OPA_TRANSP, LV_PART_MAIN); lv_obj_set_style_border_width(c->actions, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(card, delete_cb, LV_EVENT_DELETE, c);
    rebuild_actions(c); o->obj = card; o->ctx = c; return ESP_OK;
}

void w_alarm_control_apply_state(ui_widget_instance_t *i, const ha_state_t *s)
{
    if (i == NULL || s == NULL || i->ctx == NULL) return;
    w_alarm_control_ctx_t *c = i->ctx;
    c->unavailable = strcmp(s->state, "unavailable") == 0 || strcmp(s->state, "unknown") == 0;
    uint32_t old_features = c->supported_features;
    c->supported_features = 0;
    c->code_required = false;
    cJSON *a = cJSON_Parse(s->attributes_json);
    if (a != NULL) {
        cJSON *sf = cJSON_GetObjectItemCaseSensitive(a, "supported_features");
        cJSON *cr = cJSON_GetObjectItemCaseSensitive(a, "code_arm_required");
        if (cJSON_IsNumber(sf)) c->supported_features = (uint32_t)sf->valuedouble;
        c->code_required = cJSON_IsTrue(cr);
        cJSON_Delete(a);
    }
    if (old_features != c->supported_features) rebuild_actions(c);
    apply_visual(c, s->state);
}

void w_alarm_control_mark_unavailable(ui_widget_instance_t *i)
{
    if (i != NULL && i->ctx != NULL) {
        w_alarm_control_ctx_t *c = i->ctx; c->unavailable = true; apply_visual(c, "");
    }
}
