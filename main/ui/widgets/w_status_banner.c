/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 khennessy74-a11y
 */
#include "ui/ui_widget_factory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/fonts/mdi_font_registry.h"
#include "ui/ui_pages.h"
#include "ui/theme/theme_default.h"

typedef struct {
    lv_obj_t *card;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *state;
    char message[APP_MAX_NAME_LEN];
    bool append_state;
    int timeout_sec;
    lv_timer_t *dismiss_timer;
    ui_widget_instance_t *instance;
} w_status_banner_ctx_t;

static bool status_banner_apply_icon(
    lv_obj_t *label,
    const char *icon_name)
{
    if (label == NULL) {
        return false;
    }

    const char *name =
        (icon_name != NULL && icon_name[0] != '\0')
            ? icon_name
            : "mdi:alert-circle";

    uint32_t codepoint = 0U;
    if (!mdi_icon_lookup(name, &codepoint) &&
        !mdi_icon_lookup("mdi:alert-circle", &codepoint)) {
        return false;
    }

    const lv_font_t *font = mdi_font_large();
    if (font == NULL) {
        font = mdi_font_icon_56();
    }
    if (font == NULL) {
        return false;
    }

    char utf8[5] = {0};
    if (!mdi_icon_codepoint_to_utf8(codepoint, utf8)) {
        return false;
    }

    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_text(label, utf8);
    return true;
}

static void status_banner_dismiss_timer_cb(lv_timer_t *timer)
{
    w_status_banner_ctx_t *ctx =
        (w_status_banner_ctx_t *)lv_timer_get_user_data(timer);

    if (ctx == NULL || ctx->instance == NULL) {
        return;
    }

    ctx->instance->status_dismissed = true;
    ui_widget_factory_set_visible(ctx->instance, false);
}

static void status_banner_restart_dismiss_timer(w_status_banner_ctx_t *ctx)
{
    if (ctx == NULL || ctx->timeout_sec <= 0) {
        return;
    }

    if (ctx->dismiss_timer == NULL) {
        ctx->dismiss_timer =
            lv_timer_create(
                status_banner_dismiss_timer_cb,
                (uint32_t)ctx->timeout_sec * 1000U,
                ctx);
        if (ctx->dismiss_timer != NULL) {
            lv_timer_set_repeat_count(ctx->dismiss_timer, 1);
        }
    } else {
        lv_timer_set_period(
            ctx->dismiss_timer,
            (uint32_t)ctx->timeout_sec * 1000U);
        lv_timer_reset(ctx->dismiss_timer);
        lv_timer_resume(ctx->dismiss_timer);
    }
}

static bool status_banner_is_inactive(const char *state)
{
    if (state == NULL) return true;
    return strcmp(state, "off") == 0 ||
           strcmp(state, "closed") == 0 ||
           strcmp(state, "idle") == 0 ||
           strcmp(state, "clear") == 0 ||
           strcmp(state, "ok") == 0 ||
           strcmp(state, "home") == 0;
}

static uint32_t status_banner_severity_color(const char *severity)
{
    if (severity != NULL && strcmp(severity, "critical") == 0) {
        return APP_UI_COLOR_ERROR;
    }
    if (severity != NULL && strcmp(severity, "warning") == 0) {
        return APP_UI_COLOR_HEAT_ICON_ON;
    }
    if (severity != NULL && strcmp(severity, "success") == 0) {
        return APP_UI_COLOR_OK;
    }
    return APP_UI_COLOR_STATE_ON;
}

static void status_banner_apply_visual(
    w_status_banner_ctx_t *ctx,
    bool active,
    const char *severity)
{
    if (ctx == NULL || ctx->card == NULL) {
        return;
    }

    uint32_t accent = status_banner_severity_color(severity);

    lv_obj_set_style_bg_color(
        ctx->card,
        lv_color_hex(active ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF),
        LV_PART_MAIN);
    lv_obj_set_style_border_color(
        ctx->card,
        lv_color_hex(active ? accent : APP_UI_COLOR_CARD_BORDER),
        LV_PART_MAIN);

    if (ctx->state != NULL) {
        lv_obj_set_style_text_color(
            ctx->state,
            lv_color_hex(active ? accent : APP_UI_COLOR_TEXT_PRIMARY),
            LV_PART_MAIN);
    }

    if (ctx->icon != NULL) {
        lv_obj_set_style_text_color(
            ctx->icon,
            lv_color_hex(active ? accent : APP_UI_COLOR_TEXT_MUTED),
            LV_PART_MAIN);
    }
}

esp_err_t w_status_banner_create(
    const ui_widget_def_t *def,
    lv_obj_t *parent,
    ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    w_status_banner_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) return ESP_ERR_NO_MEM;

    const bool overlay =
        strcmp(def->status_presentation, "overlay") == 0;

    lv_obj_t *card =
        lv_obj_create(overlay ? lv_layer_top() : parent);

    if (overlay) {
        const ui_pages_geometry_t *geometry = ui_pages_geometry();
        lv_coord_t overlay_width =
            geometry != NULL && geometry->screen_w > 32
                ? geometry->screen_w - 24
                : def->w;
        if (overlay_width > 620) {
            overlay_width = 620;
        }

        lv_obj_set_size(card, overlay_width, 86);
        lv_obj_align(
            card,
            LV_ALIGN_TOP_MID,
            0,
            geometry != NULL ? geometry->content_y + 8 : 56);
        lv_obj_move_foreground(card);
    } else {
        lv_obj_set_pos(card, def->x, def->y);
        lv_obj_set_size(card, def->w, def->h);
    }
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(card, 12, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(card);
    lv_obj_set_width(icon, 42);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
    if (!def->show_icon ||
        !status_banner_apply_icon(icon, def->icon)) {
        lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
    }

    const lv_coord_t title_x =
        lv_obj_has_flag(icon, LV_OBJ_FLAG_HIDDEN) ? 0 : 50;

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->entity_id);
    lv_obj_set_width(title, def->show_state ? LV_PCT(50) : LV_PCT(78));
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(title, APP_FONT_TEXT_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, title_x, 0);
    if (!def->show_title) {
        lv_obj_add_flag(title, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *state = lv_label_create(card);
    lv_label_set_text(state, "--");
    lv_obj_set_width(state, def->show_title ? LV_PCT(38) : LV_PCT(72));
    lv_label_set_long_mode(state, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(state, APP_FONT_TEXT_20, LV_PART_MAIN);
    lv_obj_set_style_text_align(state, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align(state, LV_ALIGN_RIGHT_MID, 0, 0);
    if (!def->show_state) {
        lv_obj_add_flag(state, LV_OBJ_FLAG_HIDDEN);
    }

    ctx->card = card;
    ctx->icon = icon;
    ctx->title = title;
    ctx->state = state;
    snprintf(ctx->message, sizeof(ctx->message), "%s", def->status_message);
    ctx->append_state = def->status_append_state;
    ctx->timeout_sec = def->status_timeout_sec;
    ctx->instance = out_instance;
    snprintf(
        out_instance->status_presentation,
        sizeof(out_instance->status_presentation),
        "%s",
        overlay ? "overlay" : "inline");
    status_banner_apply_visual(ctx, false, def->status_severity);

    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_status_banner_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL || instance->ctx == NULL) return;
    w_status_banner_ctx_t *ctx = (w_status_banner_ctx_t *)instance->ctx;

    char text[192] = {0};
    char live_state[128] = {0};
    const char *unit = "";
    instance->status_dismissed = false;
    status_banner_restart_dismiss_timer(ctx);

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(attrs, "unit_of_measurement");
        if (cJSON_IsString(unit_item) && unit_item->valuestring != NULL) {
            unit = unit_item->valuestring;
        }
    }
    snprintf(
        live_state,
        sizeof(live_state),
        "%s%s%s",
        state->state,
        unit[0] ? " " : "",
        unit);

    if (ctx->message[0] != '\0' && ctx->append_state) {
        size_t used = 0U;

        int wrote = snprintf(
            text,
            sizeof(text),
            "%.*s",
            (int)(sizeof(text) - 1U),
            ctx->message);

        if (wrote > 0) {
            used = (size_t)wrote;
            if (used >= sizeof(text)) {
                used = sizeof(text) - 1U;
            }
        }

        if (used < sizeof(text) - 1U) {
            static const char separator[] = " · ";
            const size_t remaining = sizeof(text) - used - 1U;
            strncat(text, separator, remaining);
            used = strlen(text);
        }

        if (used < sizeof(text) - 1U) {
            const size_t remaining = sizeof(text) - used - 1U;
            strncat(text, live_state, remaining);
        }

    } else if (ctx->message[0] != '\0') {

        snprintf(
            text,
            sizeof(text),
            "%.*s",
            (int)(sizeof(text) - 1U),
            ctx->message);

    } else {

        snprintf(
            text,
            sizeof(text),
            "%.*s",
            (int)(sizeof(text) - 1U),
            live_state);
    }

    if (instance->show_state && ctx->state != NULL) {
        lv_label_set_text(ctx->state, text);
    }
    status_banner_apply_visual(ctx, !status_banner_is_inactive(state->state), instance->status_severity);
    if (attrs != NULL) cJSON_Delete(attrs);
}

void w_status_banner_destroy(ui_widget_instance_t *instance)
{
    if (instance == NULL) {
        return;
    }

    w_status_banner_ctx_t *ctx =
        (w_status_banner_ctx_t *)instance->ctx;

    if (ctx != NULL) {
        if (ctx->dismiss_timer != NULL) {
            lv_timer_del(ctx->dismiss_timer);
            ctx->dismiss_timer = NULL;
        }

        if (ctx->card != NULL) {
            lv_obj_del(ctx->card);
            ctx->card = NULL;
        }

        free(ctx);
    } else if (instance->obj != NULL) {
        lv_obj_del(instance->obj);
    }

    instance->ctx = NULL;
    instance->obj = NULL;
}

void w_status_banner_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->ctx == NULL) return;
    w_status_banner_ctx_t *ctx = (w_status_banner_ctx_t *)instance->ctx;
    if (!instance->show_state || ctx->state == NULL) {
        status_banner_apply_visual(ctx, false, instance->status_severity);
        return;
    }

    if (ctx->message[0] != '\0' && !ctx->append_state) {
        lv_label_set_text(ctx->state, ctx->message);
    } else if (ctx->message[0] != '\0') {
        char text[192] = {0};
        snprintf(
            text,
            sizeof(text),
            "%.*s",
            (int)(sizeof(text) - 1U),
            ctx->message);

        size_t used = strlen(text);
        if (used < sizeof(text) - 1U) {
            const size_t remaining = sizeof(text) - used - 1U;
            strncat(text, " · unavailable", remaining);
        }

        lv_label_set_text(ctx->state, text);
    } else {
        lv_label_set_text(ctx->state, "unavailable");
    }
    status_banner_apply_visual(ctx, false, instance->status_severity);
}
