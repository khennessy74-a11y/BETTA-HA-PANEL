/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */

#include "ui/ui_widget_factory.h"
#include "ui/fonts/mdi_font_registry.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "cJSON.h"
#include "esp_timer.h"

#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/ui_memory.h"


/* --------------------------------------------------------------------------
 * Timer widget
 *
 * Home Assistant is the source of truth.
 *
 * HA timer states:
 *   idle
 *   active
 *   paused
 *
 * For an active timer we use finishes_at where available and calculate the
 * countdown locally. This avoids polling Home Assistant every second.
 *
 * For a paused timer we use the "remaining" attribute.
 * -------------------------------------------------------------------------- */

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];

    lv_obj_t *card;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *state;
    lv_obj_t *remaining;

    lv_obj_t *start_button;
    lv_obj_t *pause_button;
    lv_obj_t *cancel_button;
    lv_obj_t *finish_button;

    bool show_icon;
    bool show_state;
    bool show_title;

    bool show_start;
    bool show_pause;
    bool show_cancel;
    bool show_finish;

    /*
     * Unix timestamp, milliseconds, at which an active timer finishes.
     * 0 means that no usable finishes_at value is available.
     */
    int64_t finishes_at_ms;

    /*
     * Remaining milliseconds used for paused/idle/fallback operation.
     */
    int64_t remaining_ms;

    bool unavailable;

    lv_timer_t *tick_timer;
} w_timer_ctx_t;


/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static bool timer_state_is_unavailable(const char *state)
{
    if (state == NULL || state[0] == '\0') {
        return true;
    }

    return strcmp(state, "unavailable") == 0 ||
           strcmp(state, "unknown") == 0;
}


static int64_t timer_now_unix_ms(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }

    return ((int64_t)tv.tv_sec * 1000LL) +
           ((int64_t)tv.tv_usec / 1000LL);
}


/*
 * Convert a civil UTC date/time into Unix seconds.
 *
 * This avoids depending on timegm(), which is not consistently available
 * across all ESP-IDF/newlib configurations.
 */
static int64_t timer_days_from_civil(
    int year,
    unsigned month,
    unsigned day)
{
    year -= month <= 2;

    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned mp = month + (month > 2 ? (unsigned)-3 : 9);
    const unsigned doy =
        (153 * mp + 2) / 5 + day - 1;
    const unsigned doe =
        yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return (int64_t)era * 146097LL +
           (int64_t)doe -
           719468LL;
}


static bool timer_parse_iso8601_ms(
    const char *text,
    int64_t *out_ms)
{
    if (text == NULL || out_ms == NULL || text[0] == '\0') {
        return false;
    }

    /*
     * HA normally returns values such as:
     *
     * 2026-09-15T12:34:56+00:00
     * 2026-09-15T12:34:56.123456+00:00
     * 2026-09-15T12:34:56Z
     *
     * We parse the date/time portion and then apply the explicit timezone.
     */

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (sscanf(
            text,
            "%d-%d-%dT%d:%d:%d",
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &second) != 6) {
        return false;
    }

    if (year < 1970 ||
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return false;
    }

    int64_t seconds =
        timer_days_from_civil(
            year,
            (unsigned)month,
            (unsigned)day) * 86400LL;

    seconds += (int64_t)hour * 3600LL;
    seconds += (int64_t)minute * 60LL;
    seconds += (int64_t)second;

    /*
     * Work out the timezone offset.
     *
     * HA generally gives UTC ("Z" or "+00:00"), but accepting an explicit
     * offset makes the parser safer.
     */
    const char *timezone = strchr(text, 'T');
    if (timezone != NULL) {
        timezone = strchr(timezone, '+');

        if (timezone == NULL) {
            /*
             * Look for a negative offset after the time portion.
             */
            const char *minus = strchr(text + 19, '-');
            if (minus != NULL) {
                timezone = minus;
            }
        }
    }

    if (timezone != NULL) {
        int tz_hour = 0;
        int tz_minute = 0;

        if (sscanf(
                timezone,
                "%*c%d:%d",
                &tz_hour,
                &tz_minute) == 2) {

            int offset_seconds =
                (tz_hour * 3600) +
                (tz_minute * 60);

            if (timezone[0] == '+') {
                /*
                 * Local time = UTC + offset.
                 * Therefore UTC = local - offset.
                 */
                seconds -= offset_seconds;
            } else if (timezone[0] == '-') {
                /*
                 * Local time = UTC - offset.
                 * Therefore UTC = local + offset.
                 */
                seconds += offset_seconds;
            }
        }
    }

    /*
     * Optional fractional seconds.
     */
    int milliseconds = 0;

    const char *fraction = strchr(text, '.');
    if (fraction != NULL) {
        int fraction_digits = 0;
        int fraction_value = 0;

        fraction++;

        while (*fraction >= '0' &&
               *fraction <= '9' &&
               fraction_digits < 3) {
            fraction_value =
                fraction_value * 10 +
                (*fraction - '0');

            fraction_digits++;
            fraction++;
        }

        if (fraction_digits == 1) {
            milliseconds = fraction_value * 100;
        } else if (fraction_digits == 2) {
            milliseconds = fraction_value * 10;
        } else if (fraction_digits == 3) {
            milliseconds = fraction_value;
        }
    }

    *out_ms =
        seconds * 1000LL +
        milliseconds;

    return true;
}


static int64_t timer_parse_duration_ms(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return 0;
    }

    /*
     * HA timer "remaining" is normally:
     *
     * HH:MM:SS
     *
     * Be slightly more tolerant and accept:
     *
     * HH:MM:SS.mmm
     */

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int milliseconds = 0;

    if (sscanf(
            text,
            "%d:%d:%d.%d",
            &hours,
            &minutes,
            &seconds,
            &milliseconds) >= 3) {

        if (hours < 0 ||
            minutes < 0 ||
            minutes > 59 ||
            seconds < 0 ||
            seconds > 59) {
            return 0;
        }

        int64_t result =
            ((int64_t)hours * 3600LL +
             (int64_t)minutes * 60LL +
             (int64_t)seconds) * 1000LL;

        /*
         * Only the first three fractional digits are milliseconds.
         */
        if (milliseconds > 999) {
            milliseconds %= 1000;
        }

        result += milliseconds;

        return result;
    }

    /*
     * Also accept a plain number as seconds.
     */
    char *end = NULL;
    long seconds_value = strtol(text, &end, 10);

    if (end != text &&
        *end == '\0' &&
        seconds_value >= 0) {
        return (int64_t)seconds_value * 1000LL;
    }

    return 0;
}


static void timer_format_remaining(
    int64_t remaining_ms,
    char *out,
    size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }

    if (remaining_ms < 0) {
        remaining_ms = 0;
    }

    int64_t total_seconds =
        (remaining_ms + 999LL) / 1000LL;

    int64_t hours = total_seconds / 3600LL;
    int64_t minutes =
        (total_seconds % 3600LL) / 60LL;
    int64_t seconds =
        total_seconds % 60LL;

    if (hours > 99) {
        hours = 99;
    }

    snprintf(
        out,
        out_size,
        "%02lld:%02lld:%02lld",
        (long long)hours,
        (long long)minutes,
        (long long)seconds);
}


static const char *timer_state_text(const char *state)
{
    if (state == NULL || state[0] == '\0') {
        return ui_i18n_get(
            "common.unknown",
            "unknown");
    }

    if (strcmp(state, "active") == 0) {
        return ui_i18n_get(
            "timer.active",
            "Active");
    }

    if (strcmp(state, "paused") == 0) {
        return ui_i18n_get(
            "timer.paused",
            "Paused");
    }

    if (strcmp(state, "idle") == 0) {
        return ui_i18n_get(
            "timer.idle",
            "Idle");
    }

    if (strcmp(state, "unavailable") == 0) {
        return ui_i18n_get(
            "common.unavailable",
            "unavailable");
    }

    return state;
}


/* --------------------------------------------------------------------------
 * Button helpers
 * -------------------------------------------------------------------------- */

typedef enum {
    TIMER_BUTTON_START = 0,
    TIMER_BUTTON_PAUSE,
    TIMER_BUTTON_CANCEL,
    TIMER_BUTTON_FINISH,
} timer_button_action_t;


static void timer_button_event_cb(lv_event_t *event)
{
    if (event == NULL) {
        return;
    }

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    w_timer_ctx_t *ctx =
        (w_timer_ctx_t *)lv_event_get_user_data(event);

    if (ctx == NULL || ctx->entity_id[0] == '\0') {
        return;
    }

    lv_obj_t *button =
        lv_event_get_target(event);

    timer_button_action_t action =
        (timer_button_action_t)(uintptr_t)
            lv_obj_get_user_data(button);

    ui_bindings_timer_action_t ha_action;

    switch (action) {
    case TIMER_BUTTON_START:
        ha_action = UI_BINDINGS_TIMER_START;
        break;

    case TIMER_BUTTON_PAUSE:
        ha_action = UI_BINDINGS_TIMER_PAUSE;
        break;

    case TIMER_BUTTON_CANCEL:
        ha_action = UI_BINDINGS_TIMER_CANCEL;
        break;

    case TIMER_BUTTON_FINISH:
        ha_action = UI_BINDINGS_TIMER_FINISH;
        break;

    default:
        return;
    }

    (void)ui_bindings_timer_action(
        ctx->entity_id,
        ha_action);
}


static lv_obj_t *timer_create_button(
    lv_obj_t *parent,
    const char *text,
    timer_button_action_t action,
    w_timer_ctx_t *ctx)
{
    lv_obj_t *button =
        lv_btn_create(parent);

    if (button == NULL) {
        return NULL;
    }

    lv_obj_set_user_data(
        button,
        (void *)(uintptr_t)action);

    lv_obj_set_style_radius(
        button,
        8,
        LV_PART_MAIN);

    lv_obj_set_style_bg_color(
        button,
        theme_default_color_surface_alt(),
        LV_PART_MAIN);

    lv_obj_set_style_bg_opa(
        button,
        LV_OPA_COVER,
        LV_PART_MAIN);

    lv_obj_set_style_border_width(
        button,
        0,
        LV_PART_MAIN);

    lv_obj_t *label =
        lv_label_create(button);

    if (label != NULL) {
        lv_label_set_text(
            label,
            text);

        lv_obj_set_style_text_color(
            label,
            theme_default_color_text_primary(),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            label,
            APP_FONT_TEXT_14,
            LV_PART_MAIN);

        lv_obj_center(label);
    }

    lv_obj_add_event_cb(
        button,
        timer_button_event_cb,
        LV_EVENT_CLICKED,
        ctx);

    return button;
}


/* --------------------------------------------------------------------------
 * Layout
 * -------------------------------------------------------------------------- */

static void timer_apply_button_visibility(
    w_timer_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->start_button != NULL) {
        if (ctx->show_start) {
            lv_obj_clear_flag(
                ctx->start_button,
                LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(
                ctx->start_button,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->pause_button != NULL) {
        if (ctx->show_pause) {
            lv_obj_clear_flag(
                ctx->pause_button,
                LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(
                ctx->pause_button,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->cancel_button != NULL) {
        if (ctx->show_cancel) {
            lv_obj_clear_flag(
                ctx->cancel_button,
                LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(
                ctx->cancel_button,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->finish_button != NULL) {
        if (ctx->show_finish) {
            lv_obj_clear_flag(
                ctx->finish_button,
                LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(
                ctx->finish_button,
                LV_OBJ_FLAG_HIDDEN);
        }
    }
}


static void timer_apply_layout(
    w_timer_ctx_t *ctx)
{
    if (ctx == NULL ||
        ctx->card == NULL ||
        ctx->title == NULL ||
        ctx->state == NULL ||
        ctx->remaining == NULL) {
        return;
    }

    lv_obj_update_layout(ctx->card);

    lv_coord_t card_w =
        lv_obj_get_width(ctx->card);

    lv_coord_t card_h =
        lv_obj_get_height(ctx->card);

    lv_coord_t pad_left =
        lv_obj_get_style_pad_left(
            ctx->card,
            LV_PART_MAIN);

    lv_coord_t pad_right =
        lv_obj_get_style_pad_right(
            ctx->card,
            LV_PART_MAIN);

    lv_coord_t pad_top =
        lv_obj_get_style_pad_top(
            ctx->card,
            LV_PART_MAIN);

    lv_coord_t pad_bottom =
        lv_obj_get_style_pad_bottom(
            ctx->card,
            LV_PART_MAIN);

    lv_coord_t content_w =
        card_w - pad_left - pad_right;

    lv_coord_t content_h =
        card_h - pad_top - pad_bottom;

    if (content_w < 30) {
        content_w = 30;
    }

    if (content_h < 40) {
        content_h = 40;
    }

    /*
     * Icon.
     */
    if (ctx->icon != NULL) {
        if (ctx->show_icon) {
            lv_obj_clear_flag(
                ctx->icon,
                LV_OBJ_FLAG_HIDDEN);

            lv_obj_align(
                ctx->icon,
                LV_ALIGN_TOP_MID,
                0,
                0);
        } else {
            lv_obj_add_flag(
                ctx->icon,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

/*
 * Title.
 */
if (ctx->show_title) {
    lv_obj_clear_flag(
        ctx->title,
        LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_width(
        ctx->title,
        content_w);

    lv_obj_set_style_text_align(
        ctx->title,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN);

    lv_obj_align(
        ctx->title,
        LV_ALIGN_TOP_MID,
        0,
        ctx->show_icon ? 30 : 0);
} else {
    lv_obj_add_flag(
        ctx->title,
        LV_OBJ_FLAG_HIDDEN);
}

    /*
     * State.
     */
    if (ctx->show_state) {
        lv_obj_clear_flag(
            ctx->state,
            LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_width(
            ctx->state,
            content_w);

        lv_obj_set_style_text_align(
            ctx->state,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);

        lv_obj_align(
            ctx->state,
            LV_ALIGN_TOP_MID,
            0,
            ctx->show_icon ? 54 : 28);
    } else {
        lv_obj_add_flag(
            ctx->state,
            LV_OBJ_FLAG_HIDDEN);
    }

    /*
     * Remaining time.
     */
    lv_obj_set_width(
        ctx->remaining,
        content_w);

    lv_obj_set_style_text_align(
        ctx->remaining,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN);

    lv_obj_align(
        ctx->remaining,
        LV_ALIGN_CENTER,
        0,
        4);

    /*
     * Buttons are positioned along the bottom.
     */
    lv_obj_t *buttons[] = {
        ctx->start_button,
        ctx->pause_button,
        ctx->cancel_button,
        ctx->finish_button,
    };

    const size_t button_count =
        sizeof(buttons) / sizeof(buttons[0]);

    lv_coord_t visible_count = 0;

    for (size_t i = 0; i < button_count; i++) {
        if (buttons[i] != NULL &&
            !lv_obj_has_flag(
                buttons[i],
                LV_OBJ_FLAG_HIDDEN)) {
            visible_count++;
        }
    }

    if (visible_count > 0) {
        const lv_coord_t gap = 5;
        const lv_coord_t total_gap =
            (visible_count > 1)
                ? (lv_coord_t)(visible_count - 1) * gap
                : 0;

        lv_coord_t button_w =
            (content_w - total_gap) /
            (lv_coord_t)visible_count;

        if (button_w < 45) {
            button_w = 45;
        }

        lv_coord_t button_h = 34;

        if (content_h < 120) {
            button_h = 28;
        }

        lv_coord_t y =
            content_h - button_h;

        if (y < 0) {
            y = 0;
        }

        lv_coord_t x = 0;

        for (size_t i = 0; i < button_count; i++) {
            lv_obj_t *button = buttons[i];

            if (button == NULL ||
                lv_obj_has_flag(
                    button,
                    LV_OBJ_FLAG_HIDDEN)) {
                continue;
            }

            lv_obj_set_size(
                button,
                button_w,
                button_h);

            lv_obj_set_pos(
                button,
                x,
                y);

            x += button_w + gap;
        }
    }
}


/* --------------------------------------------------------------------------
 * Display update
 * -------------------------------------------------------------------------- */

static void timer_update_display(
    w_timer_ctx_t *ctx,
    const char *state)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->state != NULL) {
        lv_label_set_text(
            ctx->state,
            timer_state_text(state));
    }

    int64_t remaining_ms =
        ctx->remaining_ms;

    if (state != NULL &&
        strcmp(state, "active") == 0 &&
        ctx->finishes_at_ms > 0) {

        int64_t now_ms =
            timer_now_unix_ms();

        if (now_ms > 0) {
            remaining_ms =
                ctx->finishes_at_ms - now_ms;

            if (remaining_ms < 0) {
                remaining_ms = 0;
            }
        }
    }

    char text[32] = {0};

    timer_format_remaining(
        remaining_ms,
        text,
        sizeof(text));

    if (ctx->remaining != NULL) {
        lv_label_set_text(
            ctx->remaining,
            text);
    }
}


static void timer_tick_cb(lv_timer_t *timer)
{
    if (timer == NULL) {
        return;
    }

    w_timer_ctx_t *ctx =
        (w_timer_ctx_t *)lv_timer_get_user_data(
            timer);

    if (ctx == NULL ||
        ctx->unavailable) {
        return;
    }

    /*
     * We don't need to know the current HA state here.
     *
     * If finishes_at_ms is populated, the timer is active and we calculate
     * the remaining time locally.
     *
     * If it isn't populated, the stored remaining_ms is displayed.
     */
    int64_t remaining_ms =
        ctx->remaining_ms;

    if (ctx->finishes_at_ms > 0) {
        int64_t now_ms =
            timer_now_unix_ms();

        if (now_ms > 0) {
            remaining_ms =
                ctx->finishes_at_ms - now_ms;

            if (remaining_ms < 0) {
                remaining_ms = 0;
            }
        }
    }

    char text[32] = {0};

    timer_format_remaining(
        remaining_ms,
        text,
        sizeof(text));

    if (ctx->remaining != NULL) {
        lv_label_set_text(
            ctx->remaining,
            text);
    }
}


/* --------------------------------------------------------------------------
 * LVGL events
 * -------------------------------------------------------------------------- */

static void w_timer_event_cb(lv_event_t *event)
{
    if (event == NULL) {
        return;
    }

    w_timer_ctx_t *ctx =
        (w_timer_ctx_t *)lv_event_get_user_data(
            event);

    if (ctx == NULL) {
        return;
    }

    lv_event_code_t code =
        lv_event_get_code(event);

    if (code == LV_EVENT_SIZE_CHANGED) {
        timer_apply_layout(ctx);
        return;
    }

    if (code == LV_EVENT_DELETE) {
        if (ctx->tick_timer != NULL) {
            lv_timer_del(ctx->tick_timer);
            ctx->tick_timer = NULL;
        }

        free(ctx);
    }
}


/* --------------------------------------------------------------------------
 * Create
 * -------------------------------------------------------------------------- */

esp_err_t w_timer_create(
    const ui_widget_def_t *def,
    lv_obj_t *parent,
    ui_widget_instance_t *out_instance)
{
    if (def == NULL ||
        parent == NULL ||
        out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    lv_obj_t *card =
        lv_obj_create(parent);

    if (card == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_obj_set_pos(
        card,
        def->x,
        def->y);

    lv_obj_set_size(
        card,
        def->w,
        def->h);

    lv_obj_clear_flag(
        card,
        LV_OBJ_FLAG_SCROLLABLE);

    theme_default_style_card(card);

    lv_obj_set_style_pad_left(
        card,
        10,
        LV_PART_MAIN);

    lv_obj_set_style_pad_right(
        card,
        10,
        LV_PART_MAIN);

    lv_obj_set_style_pad_top(
        card,
        8,
        LV_PART_MAIN);

    lv_obj_set_style_pad_bottom(
        card,
        8,
        LV_PART_MAIN);


    /*
     * Timer icon.
     *
     * LVGL's standard timer/refresh symbol is used here as a safe fallback.
     * The custom MDI icon registry can be connected separately once the
     * firmware-side MDI name -> glyph lookup is in place.
     */
    lv_obj_t *icon =
        lv_label_create(card);

    if (icon == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    lv_label_set_text(
        icon,
        LV_SYMBOL_REFRESH);

    lv_obj_set_style_text_color(
        icon,
        theme_default_color_text_primary(),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        icon,
        APP_FONT_TEXT_28,
        LV_PART_MAIN);


    /*
     * Title.
     */
    lv_obj_t *title =
        lv_label_create(card);

    if (title == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    lv_label_set_text(
        title,
        def->title[0] != '\0'
            ? def->title
            : def->entity_id);

    lv_obj_set_style_text_color(
        title,
        theme_default_color_text_muted(),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        title,
        APP_FONT_TEXT_16,
        LV_PART_MAIN);


    /*
     * HA state.
     */
    lv_obj_t *state =
        lv_label_create(card);

    if (state == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    lv_label_set_text(
        state,
        ui_i18n_get(
            "timer.idle",
            "Idle"));

    lv_obj_set_style_text_color(
        state,
        theme_default_color_text_muted(),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        state,
        APP_FONT_TEXT_14,
        LV_PART_MAIN);


    /*
     * Countdown.
     */
    lv_obj_t *remaining =
        lv_label_create(card);

    if (remaining == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    lv_label_set_text(
        remaining,
        "00:00:00");

    lv_obj_set_style_text_color(
        remaining,
        theme_default_color_text_primary(),
        LV_PART_MAIN);

#if LV_FONT_MONTSERRAT_36
    lv_obj_set_style_text_font(
        remaining,
        &lv_font_montserrat_36,
        LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_32
    lv_obj_set_style_text_font(
        remaining,
        &lv_font_montserrat_32,
        LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(
        remaining,
        APP_FONT_TEXT_28,
        LV_PART_MAIN);
#endif


    /*
     * Context.
     */
    w_timer_ctx_t *ctx =
        ui_calloc_prefer_psram(
            1,
            sizeof(w_timer_ctx_t));

    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    snprintf(
        ctx->entity_id,
        sizeof(ctx->entity_id),
        "%s",
        def->entity_id);

    ctx->card = card;
    ctx->icon = icon;
    ctx->title = title;
    ctx->state = state;
    ctx->remaining = remaining;

    ctx->show_icon =
        def->show_icon;

    ctx->show_state =
        def->show_state;

    ctx->show_title =
        def->show_title;

    ctx->show_start =
        def->timer_show_start;

    ctx->show_pause =
        def->timer_show_pause;

    ctx->show_cancel =
        def->timer_show_cancel;

    ctx->show_finish =
        def->timer_show_finish;

    ctx->finishes_at_ms = 0;
    ctx->remaining_ms = 0;
    ctx->unavailable = false;


    /*
     * Buttons.
     */
    ctx->start_button =
        timer_create_button(
            card,
            ui_i18n_get(
                "timer.start",
                "Start"),
            TIMER_BUTTON_START,
            ctx);

    ctx->pause_button =
        timer_create_button(
            card,
            ui_i18n_get(
                "timer.pause",
                "Pause"),
            TIMER_BUTTON_PAUSE,
            ctx);

    ctx->cancel_button =
        timer_create_button(
            card,
            ui_i18n_get(
                "timer.cancel",
                "Cancel"),
            TIMER_BUTTON_CANCEL,
            ctx);

    ctx->finish_button =
        timer_create_button(
            card,
            ui_i18n_get(
                "timer.finish",
                "Finish"),
            TIMER_BUTTON_FINISH,
            ctx);


    timer_apply_button_visibility(ctx);


    /*
     * Local one-second countdown.
     */
    ctx->tick_timer =
        lv_timer_create(
            timer_tick_cb,
            1000,
            ctx);

    if (ctx->tick_timer == NULL) {
        lv_obj_del(card);
        /*
         * LV_EVENT_DELETE will free ctx.
         */
        return ESP_ERR_NO_MEM;
    }


    lv_obj_add_event_cb(
        card,
        w_timer_event_cb,
        LV_EVENT_SIZE_CHANGED,
        ctx);

    lv_obj_add_event_cb(
        card,
        w_timer_event_cb,
        LV_EVENT_DELETE,
        ctx);


    timer_apply_layout(ctx);


    memset(
        out_instance,
        0,
        sizeof(*out_instance));

    out_instance->obj = card;
    out_instance->ctx = ctx;

    return ESP_OK;
}


/* --------------------------------------------------------------------------
 * Apply HA state
 * -------------------------------------------------------------------------- */

void w_timer_apply_state(
    ui_widget_instance_t *instance,
    const ha_state_t *state)
{
    if (instance == NULL ||
        instance->obj == NULL ||
        state == NULL) {
        return;
    }

    w_timer_ctx_t *ctx =
        (w_timer_ctx_t *)instance->ctx;

    if (ctx == NULL) {
        return;
    }


    /*
     * HA unavailable/unknown.
     */
    if (timer_state_is_unavailable(state->state)) {
        ctx->unavailable = true;
        ctx->finishes_at_ms = 0;
        ctx->remaining_ms = 0;

        if (ctx->state != NULL) {
            lv_label_set_text(
                ctx->state,
                ui_i18n_get(
                    "common.unavailable",
                    "unavailable"));
        }

        if (ctx->remaining != NULL) {
            lv_label_set_text(
                ctx->remaining,
                "--:--:--");
        }

        timer_apply_layout(ctx);
        return;
    }


    ctx->unavailable = false;

    /*
     * Defaults.
     */
    ctx->finishes_at_ms = 0;
    ctx->remaining_ms = 0;


    /*
     * Parse HA timer attributes.
     */
    cJSON *root =
        cJSON_Parse(state->attributes_json);

    if (root != NULL) {

        /*
         * remaining
         */
        cJSON *remaining =
            cJSON_GetObjectItemCaseSensitive(
                root,
                "remaining");

        if (cJSON_IsString(remaining) &&
            remaining->valuestring != NULL) {

            ctx->remaining_ms =
                timer_parse_duration_ms(
                    remaining->valuestring);
        }


        /*
         * finishes_at
         */
        cJSON *finishes_at =
            cJSON_GetObjectItemCaseSensitive(
                root,
                "finishes_at");

        if (cJSON_IsString(finishes_at) &&
            finishes_at->valuestring != NULL) {

            int64_t parsed_finish_ms = 0;

            if (timer_parse_iso8601_ms(
                    finishes_at->valuestring,
                    &parsed_finish_ms)) {

                ctx->finishes_at_ms =
                    parsed_finish_ms;
            }
        }

        cJSON_Delete(root);
    }


    /*
     * Only an active timer should use finishes_at as a live countdown.
     *
     * A paused timer must remain stationary even if HA happens to retain
     * an old finishes_at value.
     */
    if (strcmp(state->state, "active") != 0) {
        ctx->finishes_at_ms = 0;
    }


    /*
     * Idle means zero remaining.
     */
    if (strcmp(state->state, "idle") == 0) {
        ctx->remaining_ms = 0;
        ctx->finishes_at_ms = 0;
    }


    timer_update_display(
        ctx,
        state->state);

    timer_apply_layout(ctx);
}


/* --------------------------------------------------------------------------
 * Mark unavailable
 * -------------------------------------------------------------------------- */

void w_timer_mark_unavailable(
    ui_widget_instance_t *instance)
{
    if (instance == NULL ||
        instance->obj == NULL) {
        return;
    }

    w_timer_ctx_t *ctx =
        (w_timer_ctx_t *)instance->ctx;

    if (ctx == NULL) {
        return;
    }

    ctx->unavailable = true;
    ctx->finishes_at_ms = 0;
    ctx->remaining_ms = 0;

    if (ctx->state != NULL) {
        lv_label_set_text(
            ctx->state,
            ui_i18n_get(
                "common.unavailable",
                "unavailable"));
    }

    if (ctx->remaining != NULL) {
        lv_label_set_text(
            ctx->remaining,
            "--:--:--");
    }

    timer_apply_layout(ctx);
}
