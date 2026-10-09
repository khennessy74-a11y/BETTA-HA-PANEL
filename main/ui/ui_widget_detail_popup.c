/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ui/ui_widget_detail_popup.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "lvgl.h"

#include "ha/ha_model.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"

static lv_obj_t *s_detail_overlay = NULL;

static void detail_popup_close(void)
{
    if (s_detail_overlay != NULL) {
        lv_obj_del(s_detail_overlay);
        s_detail_overlay = NULL;
    }
}

static void detail_popup_close_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) detail_popup_close();
}

static void detail_popup_overlay_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED &&
        lv_event_get_target(event) == s_detail_overlay) {
        detail_popup_close();
    }
}

void ui_widget_detail_popup_show(const char *title, const char *entity_id)
{
    detail_popup_close();

    ha_state_t state = {0};
    bool have_state = entity_id != NULL && entity_id[0] != '\0' &&
                      ha_model_get_state(entity_id, &state);

    char state_text[160] = {0};
    char friendly_name[APP_MAX_NAME_LEN] = {0};
    char details_text[320] = {0};
    if (have_state) {
        const char *unit = ""; cJSON *attrs = cJSON_Parse(state.attributes_json);
        if (attrs != NULL) {
            cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(attrs, "unit_of_measurement");
            if (cJSON_IsString(unit_item) && unit_item->valuestring) unit = unit_item->valuestring;
            cJSON *friendly_item = cJSON_GetObjectItemCaseSensitive(attrs, "friendly_name");
            if (cJSON_IsString(friendly_item) && friendly_item->valuestring) snprintf(friendly_name, sizeof(friendly_name), "%s", friendly_item->valuestring);
            const char *keys[] = {"device_class","battery_level","temperature","humidity","current_position","media_title"};
            size_t used = 0U;
            for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
                cJSON *item = cJSON_GetObjectItemCaseSensitive(attrs, keys[i]); if (item == NULL) continue;
                char value[64] = {0};
                if (cJSON_IsString(item) && item->valuestring) snprintf(value,sizeof(value),"%s",item->valuestring);
                else if (cJSON_IsNumber(item)) snprintf(value,sizeof(value),"%.2f",item->valuedouble);
                else if (cJSON_IsBool(item)) snprintf(value,sizeof(value),"%s",cJSON_IsTrue(item)?"true":"false"); else continue;
                int wrote = snprintf(details_text+used,sizeof(details_text)-used,"%s%s: %s",used?"\n":"",keys[i],value);
                if (wrote<=0 || (size_t)wrote>=sizeof(details_text)-used) break; used += (size_t)wrote;
            }
        }
        snprintf(state_text,sizeof(state_text),"%s%s%s",state.state,unit[0]?" ":"",unit);
        if (state.last_changed_unix_ms > 0) { time_t changed=(time_t)(state.last_changed_unix_ms/1000); struct tm tm_info={0}; localtime_r(&changed,&tm_info); char changed_text[64]={0}; strftime(changed_text,sizeof(changed_text),"%d %b %H:%M",&tm_info); size_t used=strlen(details_text); snprintf(details_text+used,sizeof(details_text)-used,"%sLast changed: %s",used?"\n":"",changed_text); }
        if (attrs != NULL) cJSON_Delete(attrs);
    } else snprintf(state_text,sizeof(state_text),"%s","State unavailable");

    s_detail_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_detail_overlay);
    lv_obj_set_size(s_detail_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_detail_overlay);
    lv_obj_add_flag(s_detail_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_detail_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_detail_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_detail_overlay, LV_OPA_60, LV_PART_MAIN);
    lv_obj_add_event_cb(s_detail_overlay, detail_popup_overlay_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *panel = lv_obj_create(s_detail_overlay);
    lv_obj_set_size(panel, LV_PCT(86), 340);
    lv_obj_center(panel);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(panel, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(APP_UI_COLOR_CARD_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 22, LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(panel);
    lv_label_set_text(title_label,
        (title != NULL && title[0] != '\0') ? title :
        (friendly_name[0] != '\0' ? friendly_name :
        ((entity_id != NULL && entity_id[0] != '\0') ? entity_id : "Details")));
    lv_obj_set_width(title_label, LV_PCT(78));
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(title_label, APP_FONT_TEXT_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *close_btn = lv_btn_create(panel);
    lv_obj_set_size(close_btn, 46, 42);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 0, -4);
    lv_obj_add_event_cb(close_btn, detail_popup_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);

    lv_obj_t *state_label = lv_label_create(panel);
    lv_label_set_text(state_label, state_text);
    lv_obj_set_width(state_label, LV_PCT(100));
    lv_label_set_long_mode(state_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(state_label, APP_FONT_TEXT_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(state_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 62);

    lv_obj_t *entity_label = lv_label_create(panel);
    lv_label_set_text(entity_label,
        (entity_id != NULL && entity_id[0] != '\0') ? entity_id : "No entity");
    lv_obj_set_width(entity_label, LV_PCT(100));
    lv_label_set_long_mode(entity_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(entity_label, APP_FONT_TEXT_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(entity_label, lv_color_hex(APP_UI_COLOR_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_align(entity_label, LV_ALIGN_TOP_LEFT, 0, 132);
    if (details_text[0] != '\0') {
        lv_obj_t *details_label = lv_label_create(panel); lv_label_set_text(details_label, details_text); lv_obj_set_width(details_label, LV_PCT(100));
        lv_label_set_long_mode(details_label, LV_LABEL_LONG_WRAP); lv_obj_set_style_text_font(details_label, APP_FONT_TEXT_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(details_label, lv_color_hex(APP_UI_COLOR_TEXT_MUTED), LV_PART_MAIN); lv_obj_align(details_label, LV_ALIGN_TOP_LEFT, 0, 170);
    }

    lv_obj_move_foreground(s_detail_overlay);
}
