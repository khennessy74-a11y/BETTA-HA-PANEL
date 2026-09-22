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

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card, *title, *value_label, *slider;
    double min, max, step, value;
    bool unavailable, suppress;
} w_input_number_ctx_t;

static double clamp_value(w_input_number_ctx_t *c, double v) {
    if (v < c->min) v = c->min;
    if (v > c->max) v = c->max;
    return v;
}
static long round_nearest(double v) {
    return (long)(v >= 0.0 ? v + 0.5 : v - 0.5);
}
static double pos_to_value(w_input_number_ctx_t *c, int pos) {
    double v = c->min + (c->max - c->min) * ((double)pos / 1000.0);
    if (c->step > 0) {
        v = c->min + (double)round_nearest((v - c->min) / c->step) * c->step;
    }
    return clamp_value(c, v);
}
static int value_to_pos(w_input_number_ctx_t *c, double v) {
    if (c->max <= c->min) return 0;
    return (int)round_nearest((clamp_value(c, v) - c->min) * 1000.0 / (c->max - c->min));
}
static void apply_visual(w_input_number_ctx_t *c) {
    char b[32]; snprintf(b,sizeof(b),"%.6g",c->value); lv_label_set_text(c->value_label,c->unavailable?"unavailable":b);
    c->suppress=true; lv_slider_set_value(c->slider,value_to_pos(c,c->value),LV_ANIM_OFF); c->suppress=false;
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
    w_input_number_ctx_t *c=ui_calloc_prefer_psram(1,sizeof(*c));if(!c){lv_obj_del(card);return ESP_ERR_NO_MEM;} snprintf(c->entity_id,sizeof(c->entity_id),"%s",d->entity_id);c->card=card;c->min=0;c->max=100;c->step=1;
    c->title=lv_label_create(card);lv_label_set_text(c->title,d->title[0]?d->title:d->id);lv_obj_set_style_text_font(c->title,APP_FONT_TEXT_20,LV_PART_MAIN);lv_obj_align(c->title,LV_ALIGN_BOTTOM_MID,0,-8);
    c->value_label=lv_label_create(card);lv_obj_set_style_text_font(c->value_label,APP_FONT_TEXT_20,LV_PART_MAIN);lv_obj_align(c->value_label,LV_ALIGN_TOP_MID,0,2);
    c->slider=lv_slider_create(card);lv_slider_set_range(c->slider,0,1000);lv_obj_set_size(c->slider,d->w-44,20);lv_obj_align(c->slider,LV_ALIGN_CENTER,0,0);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_VALUE_CHANGED,c);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_RELEASED,c);lv_obj_add_event_cb(c->slider,event_cb,LV_EVENT_DELETE,c);
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

        if (cJSON_IsNumber(min_item)) {
            ctx->min = min_item->valuedouble;
        }
        if (cJSON_IsNumber(max_item)) {
            ctx->max = max_item->valuedouble;
        }
        if (cJSON_IsNumber(step_item) && step_item->valuedouble > 0) {
            ctx->step = step_item->valuedouble;
        }
        cJSON_Delete(attrs);
    }

    if (ctx->max <= ctx->min) {
        ctx->max = ctx->min + 1;
    }

    char *end = NULL;
    double value = strtod(state->state, &end);
    if (end != state->state) {
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
