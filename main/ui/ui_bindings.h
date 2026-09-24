/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    UI_BINDINGS_MEDIA_ACTION_PLAY_PAUSE = 0,
    UI_BINDINGS_MEDIA_ACTION_STOP,
    UI_BINDINGS_MEDIA_ACTION_NEXT,
    UI_BINDINGS_MEDIA_ACTION_PREVIOUS,
} ui_bindings_media_action_t;

esp_err_t ui_bindings_toggle_entity(const char *entity_id);
/* True for entities that are "run once" actions rather than stateful switches
 * (script.*, scene.*).  Those are activated with ui_bindings_run_entity()
 * instead of being toggled. */
bool ui_bindings_entity_is_runnable(const char *entity_id);
/* Fire-and-forget activation: script.turn_on / scene.turn_on.  Returns
 * ESP_ERR_NOT_SUPPORTED for entities outside those domains. */
esp_err_t ui_bindings_run_entity(const char *entity_id);
/* Trigger an automation immediately via automation.trigger. */
esp_err_t ui_bindings_trigger_automation(const char *entity_id);
/* Press a stateless Home Assistant button.* entity via button.press. */
esp_err_t ui_bindings_press_button(const char *entity_id);
/* Lock or unlock a Home Assistant lock.* entity. */
esp_err_t ui_bindings_set_lock_state(const char *entity_id, bool locked);
/* Stop a running script (script.turn_off).  Scenes cannot be cancelled and
 * return ESP_ERR_NOT_SUPPORTED. */
esp_err_t ui_bindings_cancel_entity(const char *entity_id);
esp_err_t ui_bindings_set_entity_power(const char *entity_id, bool on);
esp_err_t ui_bindings_set_slider_value(const char *entity_id, int value);
/* Select a Home Assistant fan preset mode. */
esp_err_t ui_bindings_set_fan_preset_mode(const char *entity_id, const char *preset_mode);
typedef enum {
    UI_BINDINGS_COVER_OPEN = 0,
    UI_BINDINGS_COVER_STOP,
    UI_BINDINGS_COVER_CLOSE,
} ui_bindings_cover_action_t;
esp_err_t ui_bindings_cover_action(const char *entity_id, ui_bindings_cover_action_t action);
esp_err_t ui_bindings_set_cover_tilt_position(const char *entity_id, int tilt_position);
esp_err_t ui_bindings_set_number_value(const char *entity_id, double value);
esp_err_t ui_bindings_select_option(const char *entity_id, const char *option);
esp_err_t ui_bindings_set_climate_target_c(const char *entity_id, float celsius);
esp_err_t ui_bindings_set_light_color_temp_kelvin(const char *entity_id, int kelvin);
esp_err_t ui_bindings_set_light_rgb_color(const char *entity_id, uint8_t r, uint8_t g, uint8_t b);
esp_err_t ui_bindings_media_player_action(const char *entity_id, ui_bindings_media_action_t action);

typedef enum {
    UI_BINDINGS_TIMER_START = 0,
    UI_BINDINGS_TIMER_PAUSE,
    UI_BINDINGS_TIMER_CANCEL,
    UI_BINDINGS_TIMER_FINISH,
} ui_bindings_timer_action_t;

esp_err_t ui_bindings_timer_action(
    const char *entity_id,
    ui_bindings_timer_action_t action);
