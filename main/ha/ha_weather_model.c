/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ha/ha_weather_model.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static ha_weather_snapshot_t s_weather = {0};
static SemaphoreHandle_t s_weather_mutex = NULL;
static uint32_t s_weather_revision = 0;

static void weather_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strnlen(src, dst_size - 1U);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool weather_entity_matches_locked(const char *entity_id)
{
    return entity_id && entity_id[0] != '\0' &&
        strncmp(s_weather.entity_id, entity_id, sizeof(s_weather.entity_id)) == 0;
}

static void weather_select_entity_locked(const char *entity_id)
{
    if (weather_entity_matches_locked(entity_id)) return;
    uint32_t rev = ++s_weather_revision;
    memset(&s_weather, 0, sizeof(s_weather));
    weather_copy(s_weather.entity_id, sizeof(s_weather.entity_id), entity_id);
    s_weather.humidity = -1;
    weather_copy(s_weather.unit, sizeof(s_weather.unit), "C");
    s_weather.revision = rev;
}

static void weather_mark_changed_locked(void)
{
    s_weather.revision = ++s_weather_revision;
}

static void weather_apply_observed_to_today_locked(void)
{
    if (s_weather.observed_date_key == 0) return;
    for (size_t i = 0; i < s_weather.day_count; i++) {
        ha_weather_day_t *day = &s_weather.days[i];
        if (!day->valid || day->date_key != s_weather.observed_date_key) continue;
        if (s_weather.has_observed_low && (!day->has_low || s_weather.observed_low < day->low_temp)) {
            day->low_temp = s_weather.observed_low;
            day->has_low = true;
        }
        if (s_weather.has_observed_high && (!day->has_high || s_weather.observed_high > day->high_temp)) {
            day->high_temp = s_weather.observed_high;
            day->has_high = true;
        }
        return;
    }
}

esp_err_t ha_weather_model_init(void)
{
    if (s_weather_mutex) return ESP_OK;
    s_weather_mutex = xSemaphoreCreateMutex();
    if (!s_weather_mutex) return ESP_ERR_NO_MEM;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    memset(&s_weather, 0, sizeof(s_weather));
    s_weather.humidity = -1;
    weather_copy(s_weather.unit, sizeof(s_weather.unit), "C");
    s_weather_revision = 0;
    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

void ha_weather_model_reset(void)
{
    if (!s_weather_mutex) return;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    memset(&s_weather, 0, sizeof(s_weather));
    s_weather.humidity = -1;
    weather_copy(s_weather.unit, sizeof(s_weather.unit), "C");
    weather_mark_changed_locked();
    xSemaphoreGive(s_weather_mutex);
}

esp_err_t ha_weather_model_set_current(const char *entity_id, int local_date_key, bool has_temp,
    float temp, int humidity, const char *unit, const char *condition)
{
    if (!s_weather_mutex || !entity_id || entity_id[0] == '\0') return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    weather_select_entity_locked(entity_id);
    s_weather.has_current_temp = has_temp;
    s_weather.current_temp = temp;
    s_weather.humidity = humidity;
    if (unit && unit[0] != '\0') weather_copy(s_weather.unit, sizeof(s_weather.unit), unit);
    weather_copy(s_weather.current_condition, sizeof(s_weather.current_condition), condition);

    if (local_date_key != 0 && s_weather.observed_date_key != local_date_key) {
        s_weather.observed_date_key = local_date_key;
        s_weather.has_observed_low = false;
        s_weather.has_observed_high = false;
        s_weather.observed_low = 0.0f;
        s_weather.observed_high = 0.0f;
    }
    if (has_temp && local_date_key != 0) {
        if (!s_weather.has_observed_low || temp < s_weather.observed_low) {
            s_weather.observed_low = temp; s_weather.has_observed_low = true;
        }
        if (!s_weather.has_observed_high || temp > s_weather.observed_high) {
            s_weather.observed_high = temp; s_weather.has_observed_high = true;
        }
    }
    weather_apply_observed_to_today_locked();
    weather_mark_changed_locked();
    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

esp_err_t ha_weather_model_replace_daily(const char *entity_id, const ha_weather_day_t *days, size_t day_count)
{
    if (!s_weather_mutex || !entity_id || entity_id[0] == '\0') return ESP_ERR_INVALID_STATE;
    if (day_count > HA_WEATHER_MODEL_MAX_DAYS || (day_count > 0 && !days)) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    weather_select_entity_locked(entity_id);
    memset(s_weather.days, 0, sizeof(s_weather.days));
    s_weather.day_count = day_count;
    if (day_count > 0) memcpy(s_weather.days, days, day_count * sizeof(s_weather.days[0]));
    weather_apply_observed_to_today_locked();
    weather_mark_changed_locked();
    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

esp_err_t ha_weather_model_set_today_hourly_range(const char *entity_id, int local_date_key,
    bool has_low, float low_temp, bool has_high, float high_temp, const char *condition)
{
    if (!s_weather_mutex || !entity_id || entity_id[0] == '\0' || local_date_key == 0) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    weather_select_entity_locked(entity_id);

    ha_weather_day_t *today = NULL;
    for (size_t i = 0; i < s_weather.day_count; i++) {
        if (s_weather.days[i].valid && s_weather.days[i].date_key == local_date_key) {
            today = &s_weather.days[i];
            break;
        }
    }
    if (!today) {
        size_t move_count = s_weather.day_count;
        if (move_count >= HA_WEATHER_MODEL_MAX_DAYS) move_count = HA_WEATHER_MODEL_MAX_DAYS - 1U;
        memmove(&s_weather.days[1], &s_weather.days[0], move_count * sizeof(s_weather.days[0]));
        if (s_weather.day_count < HA_WEATHER_MODEL_MAX_DAYS) s_weather.day_count++;
        today = &s_weather.days[0];
        memset(today, 0, sizeof(*today));
        today->valid = true;
        today->date_key = local_date_key;
    }
    if (!today->has_low && has_low) { today->has_low = true; today->low_temp = low_temp; }
    if (!today->has_high && has_high) { today->has_high = true; today->high_temp = high_temp; }
    if (today->condition[0] == '\0' && condition) weather_copy(today->condition, sizeof(today->condition), condition);
    weather_apply_observed_to_today_locked();
    weather_mark_changed_locked();
    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

bool ha_weather_model_get_snapshot(const char *entity_id, ha_weather_snapshot_t *out_snapshot)
{
    if (!s_weather_mutex || !entity_id || !out_snapshot) return false;
    bool found = false;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    if (weather_entity_matches_locked(entity_id)) {
        memcpy(out_snapshot, &s_weather, sizeof(*out_snapshot));
        found = true;
    }
    xSemaphoreGive(s_weather_mutex);
    return found;
}

uint32_t ha_weather_model_revision(void)
{
    if (!s_weather_mutex) return 0;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    uint32_t rev = s_weather_revision;
    xSemaphoreGive(s_weather_mutex);
    return rev;
}
