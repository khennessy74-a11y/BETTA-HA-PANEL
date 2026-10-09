/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ha/ha_weather_model.h"
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"

static ha_weather_snapshot_t s_weather = {0};
static SemaphoreHandle_t s_weather_mutex = NULL;
static uint32_t s_weather_revision = 0;

#define HA_WEATHER_TODAY_NVS_NS "ha_wx_today"

typedef struct {
    bool loaded;
    int date_key;
    bool has_low;
    bool has_high;
    float low_temp;
    float high_temp;
    char condition[32];
} weather_today_persist_t;

static weather_today_persist_t s_today_persist = {0};

static void weather_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strnlen(src, dst_size - 1U);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static int weather_local_date_key(void)
{
    time_t now = time(NULL);
    struct tm local_now = {0};
    localtime_r(&now, &local_now);
    int year = local_now.tm_year + 1900;
    int month = local_now.tm_mon + 1;
    int day = local_now.tm_mday;
    if (year < 2020 || month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }
    return year * 10000 + month * 100 + day;
}

static void weather_persist_load_locked(void)
{
    if (s_today_persist.loaded) return;
    s_today_persist.loaded = true;

    nvs_handle_t handle;
    if (nvs_open(HA_WEATHER_TODAY_NVS_NS, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }

    int32_t date_key = 0;
    uint8_t has_low = 0;
    uint8_t has_high = 0;
    size_t value_size = sizeof(float);
    float low_temp = 0.0f;
    float high_temp = 0.0f;
    size_t condition_size = sizeof(s_today_persist.condition);

    if (nvs_get_i32(handle, "date", &date_key) == ESP_OK) {
        s_today_persist.date_key = (int)date_key;
    }
    if (nvs_get_u8(handle, "has_low", &has_low) == ESP_OK && has_low != 0) {
        value_size = sizeof(float);
        if (nvs_get_blob(handle, "low", &low_temp, &value_size) == ESP_OK &&
            value_size == sizeof(float)) {
            s_today_persist.has_low = true;
            s_today_persist.low_temp = low_temp;
        }
    }
    if (nvs_get_u8(handle, "has_high", &has_high) == ESP_OK && has_high != 0) {
        value_size = sizeof(float);
        if (nvs_get_blob(handle, "high", &high_temp, &value_size) == ESP_OK &&
            value_size == sizeof(float)) {
            s_today_persist.has_high = true;
            s_today_persist.high_temp = high_temp;
        }
    }
    if (nvs_get_str(handle, "cond", s_today_persist.condition, &condition_size) != ESP_OK) {
        s_today_persist.condition[0] = '\0';
    }
    nvs_close(handle);
}

static void weather_persist_save_locked(void)
{
    if (s_today_persist.date_key == 0 ||
        (!s_today_persist.has_low && !s_today_persist.has_high)) {
        return;
    }

    nvs_handle_t handle;
    if (nvs_open(HA_WEATHER_TODAY_NVS_NS, NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    esp_err_t err = nvs_set_i32(handle, "date", (int32_t)s_today_persist.date_key);
    if (err == ESP_OK) err = nvs_set_u8(handle, "has_low", s_today_persist.has_low ? 1 : 0);
    if (err == ESP_OK) err = nvs_set_u8(handle, "has_high", s_today_persist.has_high ? 1 : 0);
    if (err == ESP_OK && s_today_persist.has_low) {
        err = nvs_set_blob(handle, "low", &s_today_persist.low_temp, sizeof(float));
    }
    if (err == ESP_OK && s_today_persist.has_high) {
        err = nvs_set_blob(handle, "high", &s_today_persist.high_temp, sizeof(float));
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "cond", s_today_persist.condition);
    }
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
}

static bool weather_restore_persisted_today_locked(int local_date_key)
{
    if (local_date_key == 0) return false;
    weather_persist_load_locked();
    if (s_today_persist.date_key != local_date_key ||
        (!s_today_persist.has_low && !s_today_persist.has_high)) {
        return false;
    }

    ha_weather_day_t *today = NULL;
    for (size_t i = 0; i < s_weather.day_count; i++) {
        if (s_weather.days[i].valid && s_weather.days[i].date_key == local_date_key) {
            today = &s_weather.days[i];
            break;
        }
    }

    if (today == NULL) {
        size_t move_count = s_weather.day_count;
        if (move_count >= HA_WEATHER_MODEL_MAX_DAYS) {
            move_count = HA_WEATHER_MODEL_MAX_DAYS - 1U;
        }
        memmove(&s_weather.days[1], &s_weather.days[0],
            move_count * sizeof(s_weather.days[0]));
        if (s_weather.day_count < HA_WEATHER_MODEL_MAX_DAYS) {
            s_weather.day_count++;
        }
        today = &s_weather.days[0];
        memset(today, 0, sizeof(*today));
        today->valid = true;
        today->date_key = local_date_key;
    }

    bool changed = false;
    if (s_today_persist.has_low &&
        (!today->has_low || s_today_persist.low_temp < today->low_temp)) {
        today->has_low = true;
        today->low_temp = s_today_persist.low_temp;
        changed = true;
    }
    if (s_today_persist.has_high &&
        (!today->has_high || s_today_persist.high_temp > today->high_temp)) {
        today->has_high = true;
        today->high_temp = s_today_persist.high_temp;
        changed = true;
    }
    if (today->condition[0] == '\0' && s_today_persist.condition[0] != '\0') {
        weather_copy(today->condition, sizeof(today->condition), s_today_persist.condition);
        changed = true;
    }
    return changed;
}

static void weather_seed_persisted_today_locked(const ha_weather_day_t *today)
{
    if (today == NULL || !today->valid || today->date_key == 0 ||
        (!today->has_low && !today->has_high)) {
        return;
    }

    weather_persist_load_locked();
    bool changed = s_today_persist.date_key != today->date_key ||
        s_today_persist.has_low != today->has_low ||
        s_today_persist.has_high != today->has_high ||
        (today->has_low && s_today_persist.low_temp != today->low_temp) ||
        (today->has_high && s_today_persist.high_temp != today->high_temp) ||
        strncmp(s_today_persist.condition, today->condition,
            sizeof(s_today_persist.condition)) != 0;

    s_today_persist.date_key = today->date_key;
    s_today_persist.has_low = today->has_low;
    s_today_persist.has_high = today->has_high;
    s_today_persist.low_temp = today->low_temp;
    s_today_persist.high_temp = today->high_temp;
    weather_copy(s_today_persist.condition, sizeof(s_today_persist.condition), today->condition);

    if (changed) {
        weather_persist_save_locked();
    }
}

static void weather_extend_persisted_with_observed_locked(void)
{
    if (s_weather.observed_date_key == 0) return;
    weather_persist_load_locked();
    if (s_today_persist.date_key != s_weather.observed_date_key) return;

    bool changed = false;
    if (s_weather.has_observed_low &&
        (!s_today_persist.has_low || s_weather.observed_low < s_today_persist.low_temp)) {
        s_today_persist.has_low = true;
        s_today_persist.low_temp = s_weather.observed_low;
        changed = true;
    }
    if (s_weather.has_observed_high &&
        (!s_today_persist.has_high || s_weather.observed_high > s_today_persist.high_temp)) {
        s_today_persist.has_high = true;
        s_today_persist.high_temp = s_weather.observed_high;
        changed = true;
    }
    if (changed) {
        weather_persist_save_locked();
    }
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

static bool weather_day_equal(const ha_weather_day_t *a, const ha_weather_day_t *b)
{
    if (a == NULL || b == NULL) return false;
    return a->valid == b->valid &&
        a->date_key == b->date_key &&
        a->has_low == b->has_low &&
        a->has_high == b->has_high &&
        (!a->has_low || a->low_temp == b->low_temp) &&
        (!a->has_high || a->high_temp == b->high_temp) &&
        strncmp(a->condition, b->condition, sizeof(a->condition)) == 0;
}

static bool weather_days_equal_locked(const ha_weather_day_t *days, size_t day_count)
{
    if (s_weather.day_count != day_count) return false;
    for (size_t i = 0; i < day_count; i++) {
        if (!weather_day_equal(&s_weather.days[i], &days[i])) return false;
    }
    return true;
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
    memset(&s_today_persist, 0, sizeof(s_today_persist));
    weather_persist_load_locked();
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
    uint32_t before_revision = s_weather.revision;
    weather_select_entity_locked(entity_id);
    bool changed = (s_weather.revision != before_revision);

    if (s_weather.has_current_temp != has_temp ||
        (has_temp && s_weather.current_temp != temp)) {
        s_weather.has_current_temp = has_temp;
        s_weather.current_temp = temp;
        changed = true;
    }
    if (s_weather.humidity != humidity) {
        s_weather.humidity = humidity;
        changed = true;
    }
    if (unit && unit[0] != '\0' &&
        strncmp(s_weather.unit, unit, sizeof(s_weather.unit)) != 0) {
        weather_copy(s_weather.unit, sizeof(s_weather.unit), unit);
        changed = true;
    }
    const char *safe_condition = condition ? condition : "";
    if (strncmp(s_weather.current_condition, safe_condition,
            sizeof(s_weather.current_condition)) != 0) {
        weather_copy(s_weather.current_condition, sizeof(s_weather.current_condition), safe_condition);
        changed = true;
    }

    if (local_date_key != 0 && s_weather.observed_date_key != local_date_key) {
        s_weather.observed_date_key = local_date_key;
        s_weather.has_observed_low = false;
        s_weather.has_observed_high = false;
        s_weather.observed_low = 0.0f;
        s_weather.observed_high = 0.0f;
        changed = true;
    }
    if (weather_restore_persisted_today_locked(local_date_key)) {
        changed = true;
    }

    if (has_temp && local_date_key != 0) {
        if (!s_weather.has_observed_low || temp < s_weather.observed_low) {
            s_weather.observed_low = temp;
            s_weather.has_observed_low = true;
            changed = true;
        }
        if (!s_weather.has_observed_high || temp > s_weather.observed_high) {
            s_weather.observed_high = temp;
            s_weather.has_observed_high = true;
            changed = true;
        }
        weather_extend_persisted_with_observed_locked();
    }

    ha_weather_day_t before_today = {0};
    bool had_today = false;
    for (size_t i = 0; i < s_weather.day_count; i++) {
        if (s_weather.days[i].valid &&
            s_weather.days[i].date_key == s_weather.observed_date_key) {
            before_today = s_weather.days[i];
            had_today = true;
            break;
        }
    }
    weather_apply_observed_to_today_locked();
    if (had_today) {
        for (size_t i = 0; i < s_weather.day_count; i++) {
            if (s_weather.days[i].valid &&
                s_weather.days[i].date_key == s_weather.observed_date_key &&
                !weather_day_equal(&before_today, &s_weather.days[i])) {
                changed = true;
                break;
            }
        }
    }

    if (changed && s_weather.revision == before_revision) {
        weather_mark_changed_locked();
    }
    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

esp_err_t ha_weather_model_replace_daily(const char *entity_id, const ha_weather_day_t *days, size_t day_count)
{
    if (!s_weather_mutex || !entity_id || entity_id[0] == '\0') return ESP_ERR_INVALID_STATE;
    if (day_count > HA_WEATHER_MODEL_MAX_DAYS || (day_count > 0 && !days)) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    uint32_t before_revision = s_weather.revision;
    weather_select_entity_locked(entity_id);

    ha_weather_day_t merged[HA_WEATHER_MODEL_MAX_DAYS] = {0};
    size_t merged_count = 0;

    /* Preserve an already-normalized Today row when the daily provider has
     * rolled Today out of its response. The later hourly refresh can update
     * that row in place instead of making the model oscillate 6 -> 5 -> 6. */
    ha_weather_day_t existing_today = {0};
    bool have_existing_today = false;
    if (s_weather.observed_date_key != 0) {
        for (size_t i = 0; i < s_weather.day_count; i++) {
            if (s_weather.days[i].valid &&
                s_weather.days[i].date_key == s_weather.observed_date_key) {
                existing_today = s_weather.days[i];
                have_existing_today = true;
                break;
            }
        }
    }

    int local_date_key = weather_local_date_key();
    bool incoming_has_today = false;
    for (size_t i = 0; i < day_count; i++) {
        if (days[i].valid && days[i].date_key == local_date_key) {
            incoming_has_today = true;
            weather_seed_persisted_today_locked(&days[i]);
            break;
        }
    }

    if (!have_existing_today && local_date_key != 0 &&
        weather_restore_persisted_today_locked(local_date_key)) {
        for (size_t i = 0; i < s_weather.day_count; i++) {
            if (s_weather.days[i].valid &&
                s_weather.days[i].date_key == local_date_key) {
                existing_today = s_weather.days[i];
                have_existing_today = true;
                break;
            }
        }
    }

    if (have_existing_today && !incoming_has_today &&
        merged_count < HA_WEATHER_MODEL_MAX_DAYS) {
        merged[merged_count++] = existing_today;
    }

    for (size_t i = 0; i < day_count && merged_count < HA_WEATHER_MODEL_MAX_DAYS; i++) {
        merged[merged_count++] = days[i];
    }

    bool changed = (s_weather.revision != before_revision) ||
        !weather_days_equal_locked(merged, merged_count);

    if (changed) {
        memset(s_weather.days, 0, sizeof(s_weather.days));
        s_weather.day_count = merged_count;
        if (merged_count > 0) {
            memcpy(s_weather.days, merged, merged_count * sizeof(s_weather.days[0]));
        }
        weather_apply_observed_to_today_locked();
        if (s_weather.revision == before_revision) {
            weather_mark_changed_locked();
        }
    }

    xSemaphoreGive(s_weather_mutex);
    return ESP_OK;
}

esp_err_t ha_weather_model_set_today_hourly_range(const char *entity_id, int local_date_key,
    bool has_low, float low_temp, bool has_high, float high_temp, const char *condition)
{
    if (!s_weather_mutex || !entity_id || entity_id[0] == '\0' || local_date_key == 0) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_weather_mutex, portMAX_DELAY);
    uint32_t before_revision = s_weather.revision;
    weather_select_entity_locked(entity_id);
    weather_restore_persisted_today_locked(local_date_key);

    ha_weather_day_t before_days[HA_WEATHER_MODEL_MAX_DAYS] = {0};
    size_t before_count = s_weather.day_count;
    memcpy(before_days, s_weather.days, sizeof(before_days));

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
    if (!today->has_low && has_low) {
        today->has_low = true;
        today->low_temp = low_temp;
    }
    if (!today->has_high && has_high) {
        today->has_high = true;
        today->high_temp = high_temp;
    }
    if (today->condition[0] == '\0' && condition) {
        weather_copy(today->condition, sizeof(today->condition), condition);
    }

    weather_apply_observed_to_today_locked();

    bool changed = (s_weather.revision != before_revision) ||
        (before_count != s_weather.day_count);
    if (!changed) {
        for (size_t i = 0; i < s_weather.day_count; i++) {
            if (!weather_day_equal(&before_days[i], &s_weather.days[i])) {
                changed = true;
                break;
            }
        }
    }
    if (changed && s_weather.revision == before_revision) {
        weather_mark_changed_locked();
    }

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
