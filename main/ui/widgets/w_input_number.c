/* SPDX-License-Identifier: LicenseRef-FNCL-1.1 */
#include "ui/ui_widget_factory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"
#include "ui/ui_memory.h"
#include "ui/ui_i18n.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card, *title, *value_label, *slider;
    double min, max, step, value;
    char unit[24];
    char mode[8];
    int precision;
    bool unavailable, suppress;
    bool show_title, show_state;
} w_input_number_ctx_t;

static double clamp_value(w_input_number_ctx_t *c, double v) {
    if (v < c->min) v = c->min;
    if (v > c->max) v = c->max;
    return v;
}
static long round_nearest(double v) {
    return (long)(v >= 0.0 ? v + 0.5 : v - 0.5);
}
static int step_count(w_input_number_ctx_t *c) {
    if (c->step <= 0.0 || c->max <= c->min) return 1;
    const double span = c->max - c->min;
    long count = (long)(span / c->step);
    if ((double)count * c->step < span - 0.000001) {
        count++;
    }
    if (count < 1) count = 1;
    /* LVGL slider values are int32_t; keep headroom while still preserving
     * far more discrete input_number values than the old fixed 1000 range. */
    if (count > 1000000L) count = 1000000L;
    return (int)count;
}
static double pos_to_value(w_input_number_ctx_t *c, int pos) {
    if (c->step <= 0.0) return c->min;
    return clamp_value(c, c->min + (double)pos * c->step);
}
static int value_to_pos(w_input_number_ctx_t *c, double v) {
    if (c->step <= 0.0 || c->max <= c->min) return 0;
    long pos = round_nearest((clamp_value(c, v) - c->min) / c->step);
    int count = step_count(c);
    if (pos < 0) pos = 0;
    if (pos > count) pos = count;
    return (int)pos;
}
static int precision_for_step(double step) {
    if (step <= 0.0) return 0;
    double scaled = step;
    for (int precision = 0; precision < 6; precision++) {
        long nearest = round_nearest(scaled);
        double diff = scaled - (double)nearest;
        if (diff < 0.0) diff = -diff;
        if (diff < 0.000001) return precision;
        scaled *= 10.0;
    }
    return 6;
}
static void apply_visual(w_input_number_ctx_t *c) {
    char b[64];
    if (c->unit[0] != '\0') {
        snprintf(b, sizeof(b), "%.*f %s", c->precision, c->value, c->unit);
    } else {
        snprintf(b, sizeof(b), "%.*f", c->precision, c->value);
    }
    lv_label_set_text(c->value_label,c->unavailable?ui_i18n_get("common.unavailable", "unavailable"):b);
    c->suppress=true;
    lv_slider_set_range(c->slider,0,step_count(c));
    lv_slider_set_value(c->slider,value_to_pos(c,c->value),LV_ANIM_OFF);
    if (c->unavailable) {
        lv_obj_add_state(c->slider, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(c->slider, LV_STATE_DISABLED);
    }
    c->suppress=false;
    lv_obj_set_style_bg_color(c->card,lv_color_hex(APP_UI_COLOR_CARD_BG_OFF),LV_PART_MAIN);
}
static void event_cb(lv_event_t *e) {
    w_input_number_ctx_t *c=lv_event_get_user_data(e); if(!c)return;
    if(lv_event_get_code(e)==LV_EVENT_DELETE){free(c);return;}
    if(c->suppress||c->unavailable)return;
    if(lv_event_get_code(e)==LV_EVENT_VALUE_CHANGED){c->value=pos_to_value(c,lv_slider_get_value(c->slider));apply_visual(c);}
    else if(lv_event_get_code(e)==LV_EVENT_RELEASED){double v=pos_to_value(c,lv_slider_get_value(c->slider));if(ui_bindings_set_number_value(c->entity_id,v)==ESP_OK){c->value=v;apply_visual(c);}}
}
esp_err_t w_input_number_create(const ui_widget_def_t *d,lv_obj_t *p,ui_widget_instance_t *o){
    if(!d||!p||!o)return ESP_ERR_INVALID_ARG;
    lv_obj_t *card=lv_obj_create(p);lv_obj_set_pos(card,d->x,d->y);lv_obj_set_size(card,d->w,d->h);lv_obj_clear_flag(card,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_radius(card,APP_UI_CARD_RADIUS,LV_PART_MAIN);lv_obj_set_style_pad_all(card,16,LV_PART_MAIN);
    w_input_number_ctx_t *c=ui_calloc_prefer_psram(1,sizeof(*c));if(!c){lv_obj_del(card);return ESP_ERR_NO_MEM;} snprintf(c->entity_id,sizeof(c->entity_id),"%s",d->entity_id);c->card=card;c->min=0;c->max=100;c->step=1;c->precision=0;c->unit[0]='\0';snprintf(c->mode,sizeof(c->mode),"slider");c->show_title=d->show_title;c->show_state=d->show_state;
    c->title=lv_label_create(card);lv_label_set_text(c->title,d->title[0]?d->title:d->id);lv_obj_set_style_text_font(c->title,APP_FONT_TEXT_20,LV_PART_MAIN);lv_obj_align(c->title,LV_ALIGN_BOTTOM_MID,0,-8);if(!c->show_title)lv_obj_add_flag(c->title,LV_OBJ_FLAG_HIDDEN);
    c->value_label=lv_label_create(card);lv_obj_set_style_text_font(c->value_label,APP_FONT_TEXT_20,LV_PART_MAIN);lv_obj_align(c->value_label,LV_ALIGN_TOP_MID,0,2);if(!c->show_state)lv_obj_add_flag(c->value_label,LV_OBJ_FLAG_HIDDEN);
    c->slider=lv_slider_create(card);lv_slider_set_range(c->slider,0,step_count(c));lv_obj_set_size(c->slider,d->w-44,20);lv_obj_align(c->slider,LV_ALIGN_CENTER,0,0);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_VALUE_CHANGED,c);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_RELEASED,c);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_DELETE,c);
    apply_visual(c);o->obj=card;o->ctx=c;return ESP_OK;
}
void w_input_number_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || state == NULL) {
        return;
    }

    w_input_number_ctx_t *ctx = instance->ctx;
    if (ctx == NULL) {
        return;
    }

    ctx->unavailable =
        strcmp(state->state, "unavailable") == 0 ||
        strcmp(state->state, "unknown") == 0;

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *min_item = cJSON_GetObjectItemCaseSensitive(attrs, "min");
        cJSON *max_item = cJSON_GetObjectItemCaseSensitive(attrs, "max");
        cJSON *step_item = cJSON_GetObjectItemCaseSensitive(attrs, "step");
        cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(attrs, "unit_of_measurement");
        cJSON *mode_item = cJSON_GetObjectItemCaseSensitive(attrs, "mode");

        if (cJSON_IsNumber(min_item)) {
            ctx->min = min_item->valuedouble;
        }
        if (cJSON_IsNumber(max_item)) {
            ctx->max = max_item->valuedouble;
        }
        if (cJSON_IsNumber(step_item) && step_item->valuedouble > 0) {
            ctx->step = step_item->valuedouble;
        }
        ctx->precision = precision_for_step(ctx->step);
        if (cJSON_IsString(unit_item) && unit_item->valuestring != NULL) {
            snprintf(ctx->unit, sizeof(ctx->unit), "%.23s", unit_item->valuestring);
        } else {
            ctx->unit[0] = '\0';
        }
        if (cJSON_IsString(mode_item) && mode_item->valuestring != NULL) {
            snprintf(ctx->mode, sizeof(ctx->mode), "%.7s", mode_item->valuestring);
        } else {
            snprintf(ctx->mode, sizeof(ctx->mode), "slider");
        }
        cJSON_Delete(attrs);
    }

    if (ctx->max <= ctx->min) {
        ctx->max = ctx->min + 1;
    }

    char *end = NULL;
    double value = strtod(state->state, &end);
    if (end != state->state && end != NULL && *end == '\0') {
        ctx->value = clamp_value(ctx, value);
    }

    apply_visual(ctx);
}

void w_input_number_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL) {
        return;
    }

    w_input_number_ctx_t *ctx = instance->ctx;
    if (ctx != NULL) {
        ctx->unavailable = true;
        apply_visual(ctx);
    }
}
