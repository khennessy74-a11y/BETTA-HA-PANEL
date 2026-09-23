/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t display_init(void);
bool display_is_ready(void);
bool display_lock(uint32_t timeout_ms);
void display_unlock(void);
esp_err_t display_set_brightness_percent(int percent);
void display_note_activity(void);
bool display_is_screen_off(void);
void display_configure_night_mode(int day_percent, int night_percent, int mode, int night_start_hour, int night_start_minute, int day_start_hour, int day_start_minute);
void display_configure_idle(int timeout_seconds, int brightness_percent);
