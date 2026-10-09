/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ui/ui_widget_factory.h"

#include "ui/ui_pages.h"
#include "ui/ui_widget_detail_popup.h"

#include <stdio.h>
#include <string.h>

esp_err_t w_sensor_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_sensor_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_sensor_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_button_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_button_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_button_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_select_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_select_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_select_mark_unavailable(ui_widget_instance_t *instance);
esp_err_t w_input_text_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_input_text_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_input_text_mark_unavailable(ui_widget_instance_t *instance);
esp_err_t w_input_datetime_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_input_datetime_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_input_datetime_mark_unavailable(ui_widget_instance_t *instance);
esp_err_t w_alarm_control_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_alarm_control_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_alarm_control_mark_unavailable(ui_widget_instance_t *instance);
esp_err_t w_image_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_image_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_image_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_calendar_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_calendar_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_calendar_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_update_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_update_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_update_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_input_number_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_input_number_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_input_number_mark_unavailable(ui_widget_instance_t *instance);
esp_err_t w_slider_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_slider_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_slider_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_graph_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_graph_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_graph_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_empty_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_empty_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_empty_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_status_banner_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_status_banner_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_status_banner_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_light_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_light_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_light_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_fan_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_fan_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_fan_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_heating_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_heating_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_heating_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_weather_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_weather_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_weather_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_todo_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_todo_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_todo_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_media_player_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_media_player_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_media_player_mark_unavailable(ui_widget_instance_t *instance);
void w_media_player_set_visible(ui_widget_instance_t *instance, bool visible);

esp_err_t w_roborock_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_roborock_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_roborock_mark_unavailable(ui_widget_instance_t *instance);
void w_roborock_set_visible(ui_widget_instance_t *instance, bool visible);

esp_err_t w_timer_create(
    const ui_widget_def_t *def,
    lv_obj_t *parent,
    ui_widget_instance_t *out_instance);

void w_timer_apply_state(
    ui_widget_instance_t *instance,
    const ha_state_t *state);

void w_timer_mark_unavailable(
    ui_widget_instance_t *instance);

typedef esp_err_t (*widget_create_fn_t)(const ui_widget_def_t *, lv_obj_t *, ui_widget_instance_t *);
typedef void (*widget_state_fn_t)(ui_widget_instance_t *, const ha_state_t *);
typedef void (*widget_unavailable_fn_t)(ui_widget_instance_t *);

typedef struct {
    const char *type;
    widget_create_fn_t create;
    widget_state_fn_t apply_state;
    widget_unavailable_fn_t mark_unavailable;
} widget_factory_entry_t;

static const widget_factory_entry_t WIDGET_FACTORY[] = {
    {"sensor", w_sensor_create, w_sensor_apply_state, w_sensor_mark_unavailable},
    {"binary_sensor", w_sensor_create, w_sensor_apply_state, w_sensor_mark_unavailable},
    {"person", w_sensor_create, w_sensor_apply_state, w_sensor_mark_unavailable},
    {"device_tracker", w_sensor_create, w_sensor_apply_state, w_sensor_mark_unavailable},
    {"button", w_button_create, w_button_apply_state, w_button_mark_unavailable},
    {"slider", w_slider_create, w_slider_apply_state, w_slider_mark_unavailable},
    {"input_number", w_input_number_create, w_input_number_apply_state, w_input_number_mark_unavailable},
    {"select", w_select_create, w_select_apply_state, w_select_mark_unavailable},
    {"input_text", w_input_text_create, w_input_text_apply_state, w_input_text_mark_unavailable},
    {"input_datetime", w_input_datetime_create, w_input_datetime_apply_state, w_input_datetime_mark_unavailable},
    {"alarm_control_panel", w_alarm_control_create, w_alarm_control_apply_state, w_alarm_control_mark_unavailable},
    {"update", w_update_create, w_update_apply_state, w_update_mark_unavailable},
    {"calendar", w_calendar_create, w_calendar_apply_state, w_calendar_mark_unavailable},
    {"image", w_image_create, w_image_apply_state, w_image_mark_unavailable},
    {"graph", w_graph_create, w_graph_apply_state, w_graph_mark_unavailable},
    {"empty_tile", w_empty_tile_create, w_empty_tile_apply_state, w_empty_tile_mark_unavailable},
    {"status_banner", w_status_banner_create, w_status_banner_apply_state, w_status_banner_mark_unavailable},
    {"light_tile", w_light_tile_create, w_light_tile_apply_state, w_light_tile_mark_unavailable},
    {"fan_tile", w_fan_tile_create, w_fan_tile_apply_state, w_fan_tile_mark_unavailable},
    {"heating_tile", w_heating_tile_create, w_heating_tile_apply_state, w_heating_tile_mark_unavailable},
    {"weather_tile", w_weather_tile_create, w_weather_tile_apply_state, w_weather_tile_mark_unavailable},
    {"weather_3day", w_weather_tile_create, w_weather_tile_apply_state, w_weather_tile_mark_unavailable},
    {"todo_list", w_todo_create, w_todo_apply_state, w_todo_mark_unavailable},
    {"media_player", w_media_player_create, w_media_player_apply_state, w_media_player_mark_unavailable},
    {"roborock_tile", w_roborock_create, w_roborock_apply_state, w_roborock_mark_unavailable},
    {"timer", w_timer_create, w_timer_apply_state, w_timer_mark_unavailable},
};

static bool widget_supports_tap_action(const char *type)
{
    return type != NULL &&
        (strcmp(type, "sensor") == 0 ||
         strcmp(type, "binary_sensor") == 0 ||
         strcmp(type, "person") == 0 ||
         strcmp(type, "device_tracker") == 0 ||
         strcmp(type, "graph") == 0 ||
         strcmp(type, "status_banner") == 0 ||
         strcmp(type, "empty_tile") == 0 ||
         strcmp(type, "weather_tile") == 0 ||
         strcmp(type, "weather_3day") == 0);
}

static void ui_widget_tap_action_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    ui_widget_instance_t *instance =
        (ui_widget_instance_t *)lv_event_get_user_data(event);
    if (instance == NULL) return;

    if (strcmp(instance->tap_action, "detail") == 0) {
        ui_widget_detail_popup_show(instance->title, instance->entity_id);
    } else if (strcmp(instance->tap_action, "navigate") == 0 &&
               instance->tap_target_page[0] != '\0') {
        (void)ui_pages_show(instance->tap_target_page);
    }
}

static const widget_factory_entry_t *widget_factory_entry(const char *type)
{
    if (type == NULL) return NULL;
    for (size_t i = 0; i < sizeof(WIDGET_FACTORY) / sizeof(WIDGET_FACTORY[0]); i++) {
        if (strcmp(type, WIDGET_FACTORY[i].type) == 0) return &WIDGET_FACTORY[i];
    }
    return NULL;
}

esp_err_t ui_widget_factory_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out_instance, 0, sizeof(*out_instance));
    snprintf(out_instance->id, sizeof(out_instance->id), "%s", def->id);
    snprintf(out_instance->type, sizeof(out_instance->type), "%s", def->type);
    snprintf(out_instance->title, sizeof(out_instance->title), "%s", def->title);
    snprintf(out_instance->entity_id, sizeof(out_instance->entity_id), "%s", def->entity_id);
    snprintf(out_instance->icon,
    sizeof(out_instance->icon),
    "%s",
    def->icon);

out_instance->show_icon = def->show_icon;
out_instance->show_state = def->show_state;
out_instance->show_title = def->show_title;
out_instance->sensor_decimal_places = def->sensor_decimal_places;

out_instance->timer_show_start = def->timer_show_start;
out_instance->timer_show_pause = def->timer_show_pause;
out_instance->timer_show_cancel = def->timer_show_cancel;
out_instance->timer_show_finish = def->timer_show_finish;
    snprintf(out_instance->secondary_entity_id, sizeof(out_instance->secondary_entity_id), "%s", def->secondary_entity_id);
    snprintf(out_instance->visibility_mode, sizeof(out_instance->visibility_mode), "%s", def->visibility_mode);
    snprintf(out_instance->visibility_entity_id, sizeof(out_instance->visibility_entity_id), "%s", def->visibility_entity_id);
    snprintf(out_instance->visibility_state, sizeof(out_instance->visibility_state), "%s", def->visibility_state);
    snprintf(out_instance->tap_action, sizeof(out_instance->tap_action), "%s", def->tap_action);
    snprintf(out_instance->tap_target_page, sizeof(out_instance->tap_target_page), "%s", def->tap_target_page);
    snprintf(out_instance->slider_direction, sizeof(out_instance->slider_direction), "%s", def->slider_direction);
    snprintf(out_instance->slider_accent_color, sizeof(out_instance->slider_accent_color), "%s", def->slider_accent_color);
    snprintf(out_instance->button_accent_color, sizeof(out_instance->button_accent_color), "%s", def->button_accent_color);
    snprintf(out_instance->state_icon_off_color, sizeof(out_instance->state_icon_off_color), "%s", def->state_icon_off_color);
    snprintf(out_instance->state_icon_on_color, sizeof(out_instance->state_icon_on_color), "%s", def->state_icon_on_color);
    snprintf(out_instance->button_mode, sizeof(out_instance->button_mode), "%s", def->button_mode);
    snprintf(out_instance->button_appearance, sizeof(out_instance->button_appearance), "%s", def->button_appearance);
    snprintf(out_instance->graph_line_color, sizeof(out_instance->graph_line_color), "%s", def->graph_line_color);
    out_instance->graph_point_count = def->graph_point_count;
    out_instance->graph_time_window_min = def->graph_time_window_min;
    snprintf(out_instance->graph_display_mode, sizeof(out_instance->graph_display_mode), "%s", def->graph_display_mode);
    out_instance->graph_bar_bucket_min = def->graph_bar_bucket_min;
    snprintf(out_instance->style_variant, sizeof(out_instance->style_variant), "%s", def->style_variant);
    snprintf(out_instance->arc_opening, sizeof(out_instance->arc_opening), "%s", def->arc_opening);
    out_instance->ctx = NULL;

    const widget_factory_entry_t *entry = widget_factory_entry(def->type);
    if (entry == NULL) return ESP_ERR_NOT_SUPPORTED;

    esp_err_t err = entry->create(def, parent, out_instance);
    if (err == ESP_OK &&
        out_instance->obj != NULL &&
        widget_supports_tap_action(out_instance->type) &&
        out_instance->tap_action[0] != '\0' &&
        strcmp(out_instance->tap_action, "default") != 0) {
        lv_obj_add_flag(out_instance->obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(out_instance->obj, ui_widget_tap_action_cb, LV_EVENT_CLICKED, out_instance);
    }
    return err;
}

void ui_widget_factory_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) return;
    const widget_factory_entry_t *entry = widget_factory_entry(instance->type);
    if (entry != NULL) entry->apply_state(instance, state);
}

void ui_widget_factory_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) return;
    const widget_factory_entry_t *entry = widget_factory_entry(instance->type);
    if (entry != NULL) entry->mark_unavailable(instance);
}

void ui_widget_factory_set_visible(ui_widget_instance_t *instance, bool visible)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    const bool changed = (instance->visible != visible);
    instance->visible = visible;

    /* Always enforce the LVGL hidden flag, even when the logical value did
     * not change. Newly-created instances are zero-initialised (visible=false),
     * so a conditional rule that starts hidden must still hide the object on
     * its very first visibility pass. */
    if (visible) {
        lv_obj_clear_flag(instance->obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(instance->obj, LV_OBJ_FLAG_HIDDEN);
    }

    if (!changed) {
        return;
    }

    if (strcmp(instance->type, "media_player") == 0) {
        w_media_player_set_visible(instance, visible);
    } else if (strcmp(instance->type, "roborock_tile") == 0) {
        w_roborock_set_visible(instance, visible);
    }
}
