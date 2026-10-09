/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "app_config.h"
#include "esp_err.h"

#define HA_WEATHER_MODEL_MAX_DAYS 10

typedef struct {
    bool valid;
    int date_key;
    char condition[32];
    bool has_low;
    bool has_high;
    float low_temp;
    float high_temp;
} ha_weather_day_t;

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    bool has_current_temp;
    float current_temp;
    int humidity;
    char unit[12];
    char current_condition[32];
    size_t day_count;
    ha_weather_day_t days[HA_WEATHER_MODEL_MAX_DAYS];
    int observed_date_key;
    bool has_observed_low;
    bool has_observed_high;
    float observed_low;
    float observed_high;
    uint32_t revision;
} ha_weather_snapshot_t;

esp_err_t ha_weather_model_init(void);
void ha_weather_model_reset(void);
esp_err_t ha_weather_model_set_current(const char *entity_id, int local_date_key, bool has_temp,
    float temp, int humidity, const char *unit, const char *condition);
esp_err_t ha_weather_model_replace_daily(const char *entity_id, const ha_weather_day_t *days, size_t day_count);
esp_err_t ha_weather_model_set_today_hourly_range(const char *entity_id, int local_date_key,
    bool has_low, float low_temp, bool has_high, float high_temp, const char *condition);
bool ha_weather_model_get_snapshot(const char *entity_id, ha_weather_snapshot_t *out_snapshot);
uint32_t ha_weather_model_revision(void);
