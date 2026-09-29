/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Cpt_Kirk
 */
#include "ui/ui_pages.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "app_config.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/ha_client.h"
#include "drivers/display_init.h"
#include "settings/runtime_settings.h"
#include "net/wifi_mgr.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

typedef struct {
    char id[APP_MAX_PAGE_ID_LEN];
    char title[APP_MAX_NAME_LEN];
    lv_obj_t *container;
} ui_page_entry_t;

static ui_page_entry_t s_pages[APP_MAX_PAGES];
static uint16_t s_page_count = 0;
static int16_t s_current_index = -1;
static ui_pages_show_cb_t s_show_cb = NULL;

void ui_pages_set_show_callback(ui_pages_show_cb_t cb)
{
    s_show_cb = cb;
}

static lv_obj_t *s_background = NULL;
static lv_obj_t *s_topbar = NULL;
static lv_obj_t *s_content_box = NULL;
static lv_obj_t *s_date_label = NULL;
static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_wifi_icon = NULL;
static lv_obj_t *s_api_icon = NULL;
static lv_obj_t *s_system_overlay = NULL;
static lv_obj_t *s_system_details = NULL;
static lv_obj_t *s_system_log = NULL;
static lv_obj_t *s_restart_confirm = NULL;
static lv_obj_t *s_brightness_slider = NULL;
static lv_obj_t *s_brightness_value = NULL;
static lv_obj_t *s_night_mode_dropdown = NULL;
static lv_obj_t *s_idle_timeout_dropdown = NULL;
static lv_timer_t *s_system_timeout_timer = NULL;
#define UI_SYSTEM_TIMEOUT_MS 60000U
static bool s_status_gesture_armed = false;
static uint32_t s_status_gesture_started_ms = 0U;
#define UI_SYSTEM_HOLD_MS 3000U
static lv_obj_t *s_nav_bar = NULL;
static lv_obj_t *s_nav_home_button = NULL;
static lv_obj_t *s_nav_home_label = NULL;
static lv_obj_t *s_nav_extra_buttons[APP_MAX_PAGES - 1] = {0};
static lv_obj_t *s_nav_extra_labels[APP_MAX_PAGES - 1] = {0};
static uint16_t s_nav_extra_page_index[APP_MAX_PAGES - 1] = {0};
static ui_pages_geometry_t s_geometry = {
    .screen_w = APP_SCREEN_WIDTH,
    .screen_h = APP_SCREEN_HEIGHT,
    .content_x = APP_CONTENT_BOX_X,
    .content_y = APP_CONTENT_BOX_Y,
    .content_w = APP_CONTENT_BOX_WIDTH,
    .content_h = APP_CONTENT_BOX_HEIGHT,
    .nav_h = 60,
};

const ui_pages_geometry_t *ui_pages_geometry(void)
{
    return &s_geometry;
}

static void ui_pages_refresh_geometry(lv_obj_t *screen)
{
    lv_coord_t screen_w = screen != NULL ? lv_obj_get_width(screen) : 0;
    lv_coord_t screen_h = screen != NULL ? lv_obj_get_height(screen) : 0;
    if (screen_w <= 0) screen_w = APP_SCREEN_WIDTH;
    if (screen_h <= 0) screen_h = APP_SCREEN_HEIGHT;

    s_geometry.screen_w = screen_w;
    s_geometry.screen_h = screen_h;
    s_geometry.content_x = APP_CONTENT_BOX_X;
    s_geometry.content_y = APP_CONTENT_BOX_Y;
    s_geometry.nav_h = 60;
    s_geometry.content_w = screen_w - s_geometry.content_x;
    s_geometry.content_h = screen_h - s_geometry.content_y - s_geometry.nav_h;
    if (s_geometry.content_w < 0) s_geometry.content_w = 0;
    if (s_geometry.content_h < 0) s_geometry.content_h = 0;
}

/* ---- Easter egg: 7 taps on the home nav button reveal a swimming Betta. */
#if LV_USE_LOTTIE && APP_UI_BETTA_LOTTIE_ASSET
extern const uint8_t betta_lottie_start[] asm("_binary_betta_json_start");
extern const uint8_t betta_lottie_end[]   asm("_binary_betta_json_end");
#define UI_BETTA_TAP_TARGET    7U
#define UI_BETTA_TAP_WINDOW_MS 2500U

static lv_obj_t *s_betta_overlay = NULL;
static lv_obj_t *s_betta_lottie  = NULL;
static void     *s_betta_buf     = NULL;
static uint8_t   s_betta_taps    = 0;
static uint32_t  s_betta_last_ms = 0;
#endif

#define TOPBAR_TIME_FONT APP_FONT_TEXT_34

#define TOPBAR_DATE_FONT APP_FONT_TEXT_22

#if LV_FONT_MONTSERRAT_24
#define TOPBAR_ICON_FONT (&lv_font_montserrat_24)
#elif LV_FONT_MONTSERRAT_20
#define TOPBAR_ICON_FONT (&lv_font_montserrat_20)
#else
#define TOPBAR_ICON_FONT LV_FONT_DEFAULT
#endif

#define NAV_TEXT_FONT APP_FONT_TEXT_16

static void ui_pages_style_nav_button(lv_obj_t *btn, lv_obj_t *label, bool selected, bool is_home)
{
    if (btn == NULL || label == NULL) {
        return;
    }

    const lv_color_t chip_bg = lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BG);
    const lv_color_t chip_border = lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BORDER);
    lv_obj_set_style_bg_color(btn, chip_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, selected ? LV_OPA_80 : LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn, selected ? LV_OPA_COVER : LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, chip_border, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(btn, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (is_home) {
        lv_obj_set_style_text_color(
            label,
            selected ? lv_color_hex(APP_UI_COLOR_NAV_HOME_ACTIVE) : lv_color_hex(APP_UI_COLOR_NAV_HOME_IDLE),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(
            label,
            selected ? lv_color_hex(APP_UI_COLOR_NAV_TAB_ACTIVE) : lv_color_hex(APP_UI_COLOR_NAV_TAB_IDLE),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void ui_pages_apply_tab_style(uint16_t selected_index)
{
    if (s_nav_bar == NULL || s_nav_home_button == NULL || s_nav_home_label == NULL) {
        return;
    }

    const lv_coord_t nav_btn_h = 42;
    const lv_coord_t nav_home_w = 72;
    const lv_coord_t nav_btn_y = 9;
    const lv_coord_t nav_outer_margin = 14;
    const lv_coord_t nav_home_gap = 12;
    const lv_coord_t nav_side_gap = 8;
    const lv_coord_t nav_min_side_btn_w = 64;
    const lv_coord_t nav_home_x = (s_geometry.screen_w - nav_home_w) / 2;

    lv_obj_set_size(s_nav_home_button, nav_home_w, nav_btn_h);
    lv_obj_set_pos(s_nav_home_button, nav_home_x, nav_btn_y);
    ui_pages_style_nav_button(s_nav_home_button, s_nav_home_label, (selected_index == 0), true);
    lv_obj_clear_flag(s_nav_home_button, LV_OBJ_FLAG_HIDDEN);

    uint16_t extra_count = (s_page_count > 1) ? (uint16_t)(s_page_count - 1U) : 0U;
    uint16_t left_count = (uint16_t)((extra_count + 1U) / 2U);
    uint16_t right_count = (uint16_t)(extra_count / 2U);

    lv_coord_t left_start = nav_outer_margin;
    lv_coord_t left_end = nav_home_x - nav_home_gap;
    lv_coord_t right_start = nav_home_x + nav_home_w + nav_home_gap;
    lv_coord_t right_end = s_geometry.screen_w - nav_outer_margin;

    lv_coord_t left_region_w = (left_end > left_start) ? (left_end - left_start) : 0;
    lv_coord_t right_region_w = (right_end > right_start) ? (right_end - right_start) : 0;

    lv_coord_t left_btn_w = 0;
    lv_coord_t right_btn_w = 0;
    if (left_count > 0) {
        left_btn_w = (left_region_w - ((lv_coord_t)left_count - 1) * nav_side_gap) / (lv_coord_t)left_count;
        if (left_btn_w < nav_min_side_btn_w) {
            left_btn_w = nav_min_side_btn_w;
        }
    }
    if (right_count > 0) {
        right_btn_w = (right_region_w - ((lv_coord_t)right_count - 1) * nav_side_gap) / (lv_coord_t)right_count;
        if (right_btn_w < nav_min_side_btn_w) {
            right_btn_w = nav_min_side_btn_w;
        }
    }

    uint16_t left_slot = 0;
    uint16_t right_slot = 0;
    uint16_t slot = 0;
    for (uint16_t page_index = 1; page_index < s_page_count && slot < (APP_MAX_PAGES - 1); page_index++, slot++) {
        lv_obj_t *btn = s_nav_extra_buttons[slot];
        lv_obj_t *label = s_nav_extra_labels[slot];
        if (btn == NULL || label == NULL) {
            continue;
        }

        bool place_left = (((page_index - 1U) & 0x1U) == 0U);
        lv_coord_t w = place_left ? left_btn_w : right_btn_w;
        lv_coord_t x = 0;
        if (place_left) {
            uint16_t pos = (uint16_t)((left_count - 1U) - left_slot);
            x = left_start + (lv_coord_t)pos * (w + nav_side_gap);
            left_slot++;
        } else {
            uint16_t pos = right_slot;
            x = right_start + (lv_coord_t)pos * (w + nav_side_gap);
            right_slot++;
        }

        s_nav_extra_page_index[slot] = page_index;
        lv_label_set_text(label, s_pages[page_index].title);
        lv_obj_set_size(btn, w, nav_btn_h);
        lv_obj_set_pos(btn, x, nav_btn_y);
        lv_obj_set_width(label, w - 20);
        lv_obj_center(label);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);
        ui_pages_style_nav_button(btn, label, (page_index == selected_index), false);
    }

    for (; slot < (APP_MAX_PAGES - 1); slot++) {
        s_nav_extra_page_index[slot] = APP_MAX_PAGES;
        if (s_nav_extra_buttons[slot] != NULL) {
            lv_obj_add_flag(s_nav_extra_buttons[slot], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_pages_style_topbar_chip(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(obj, lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(obj, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_left(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, TOPBAR_ICON_FONT, LV_PART_MAIN);
}

#if LV_USE_LOTTIE && APP_UI_BETTA_LOTTIE_ASSET
static void ui_pages_betta_hide(void);

static void ui_pages_betta_dismiss_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ui_pages_betta_hide();
}

static void ui_pages_betta_show(void)
{
    if (s_betta_overlay != NULL) {
        return;
    }
    lv_obj_t *screen = lv_scr_act();
    if (screen == NULL) {
        return;
    }

    lv_coord_t side = APP_SCREEN_HEIGHT < APP_SCREEN_WIDTH ? APP_SCREEN_HEIGHT : APP_SCREEN_WIDTH;
    side -= 120;
    /* ThorVG renders the lottie on the CPU. Pixel cost scales quadratically;
     * the weather tiles run smoothly at ~130 px, so keep the betta close to that
     * even though the screen is much larger. */
    if (side > 220) {
        side = 220;
    }
    if (side < 160) {
        side = 160;
    }

    size_t buf_bytes = (size_t)side * (size_t)side * 4U + (size_t)LV_DRAW_BUF_ALIGN;
    s_betta_buf = lv_malloc(buf_bytes);
    if (s_betta_buf == NULL) {
        return;
    }
    memset(s_betta_buf, 0, buf_bytes);

    s_betta_overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(s_betta_overlay);
    lv_obj_set_size(s_betta_overlay, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT);
    lv_obj_set_pos(s_betta_overlay, 0, 0);
    lv_obj_clear_flag(s_betta_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_betta_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_betta_overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_add_flag(s_betta_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_betta_overlay, ui_pages_betta_dismiss_cb, LV_EVENT_CLICKED, NULL);

    s_betta_lottie = lv_lottie_create(s_betta_overlay);
    lv_lottie_set_buffer(s_betta_lottie, side, side, s_betta_buf);
    lv_lottie_set_src_data(s_betta_lottie, betta_lottie_start,
                           (size_t)(betta_lottie_end - betta_lottie_start));
    lv_obj_set_size(s_betta_lottie, side, side);
    lv_obj_center(s_betta_lottie);

    lv_obj_move_foreground(s_betta_overlay);
}

static void ui_pages_betta_hide(void)
{
    if (s_betta_overlay != NULL) {
        lv_obj_del(s_betta_overlay);
        s_betta_overlay = NULL;
        s_betta_lottie  = NULL;
    }
    if (s_betta_buf != NULL) {
        lv_free(s_betta_buf);
        s_betta_buf = NULL;
    }
    s_betta_taps    = 0U;
    s_betta_last_ms = 0U;
}
#endif

static void ui_nav_home_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_page_count == 0) {
        return;
    }
    ui_pages_show_index(0);
#if LV_USE_LOTTIE && APP_UI_BETTA_LOTTIE_ASSET
    /* Easter egg: count taps on the home button. */
    uint32_t now = lv_tick_get();
    if (s_betta_last_ms != 0U && (now - s_betta_last_ms) > UI_BETTA_TAP_WINDOW_MS) {
        s_betta_taps = 0U;
    }
    s_betta_last_ms = now;
    s_betta_taps++;
    if (s_betta_taps >= UI_BETTA_TAP_TARGET) {
        s_betta_taps = 0U;
        ui_pages_betta_show();
    }
#endif
}

static void ui_nav_extra_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    uintptr_t slot = (uintptr_t)lv_event_get_user_data(event);
    if (slot >= (APP_MAX_PAGES - 1)) {
        return;
    }

    uint16_t page_index = s_nav_extra_page_index[slot];
    if (page_index >= s_page_count) {
        return;
    }
    ui_pages_show_index(page_index);
}

static void ui_system_timeout_delete(void)
{
    if (s_system_timeout_timer != NULL) {
        lv_timer_del(s_system_timeout_timer);
        s_system_timeout_timer = NULL;
    }
}

static void ui_system_overlay_close(void)
{
    ui_system_timeout_delete();
    if (s_system_overlay != NULL) {
        lv_obj_del(s_system_overlay);
        s_system_overlay = NULL;
        s_system_details = NULL;
        s_system_log = NULL;
        s_restart_confirm = NULL;
        s_brightness_slider = NULL;
        s_brightness_value = NULL;
        s_night_mode_dropdown = NULL;
        s_idle_timeout_dropdown = NULL;
    }
}

static void ui_system_overlay_show(void);
static void ui_system_display_show(void);

static void ui_system_overlay_close_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ui_system_overlay_close();
}

static void ui_system_display_open_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ui_system_overlay_close();
    ui_system_display_show();
}

static void ui_system_diagnostics_open_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ui_system_overlay_close();
    ui_system_overlay_show();
}

static void ui_system_timeout_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    ui_system_overlay_close();
    if (s_page_count > 0U) {
        (void)ui_pages_show_index(0);
    }
}

static void ui_system_activity_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    if (s_system_timeout_timer != NULL) {
        lv_timer_reset(s_system_timeout_timer);
    }
}

static void ui_system_overlay_refresh(void)
{
    if (s_system_details == NULL || s_system_log == NULL) {
        return;
    }

    const esp_app_desc_t *desc = esp_app_get_description();
    char ip[48] = "Unavailable";
    if (wifi_mgr_get_sta_ip(ip, sizeof(ip)) != ESP_OK) {
        snprintf(ip, sizeof(ip), "%s", "Unavailable");
    }

    wifi_mgr_sta_ap_info_t ap_info = {0};
    bool have_ap = (wifi_mgr_get_sta_ap_info(&ap_info) == ESP_OK);

    ha_client_diagnostics_t diag = {0};
    ha_client_get_diagnostics(&diag);

    char summary[1024] = {0};
    snprintf(
        summary,
        sizeof(summary),
        "SYSTEM\nFirmware: %s\nProject: %s\n\n"
        "WI-FI\nStatus: %s\nIP: %s\nSSID: %s\nSignal: %s\n\n"
        "HOME ASSISTANT\nStatus: %s\nInitial sync: %s",
        (desc != NULL && desc->version[0] != '\\0') ? desc->version : "unknown",
        (desc != NULL && desc->project_name[0] != '\\0') ? desc->project_name : APP_NAME,
        wifi_mgr_is_connected() ? "Connected" : "Disconnected",
        ip,
        have_ap ? ap_info.ssid : "-",
        have_ap ? "available" : "-",
        ha_client_is_connected() ? "Connected" : "Disconnected",
        ha_client_is_initial_sync_done() ? "Complete" : "Waiting");
    lv_label_set_text(s_system_details, summary);

    char log_text[3072] = {0};
    size_t used = 0U;
    if (have_ap) {
        int written = snprintf(log_text, sizeof(log_text), "Wi-Fi signal: %d dBm\n", (int)ap_info.rssi);
        if (written > 0) {
            used = (size_t)written < sizeof(log_text) ? (size_t)written : sizeof(log_text) - 1U;
        }
    }

    uint16_t start = diag.connection_log_count > 10U ? (uint16_t)(diag.connection_log_count - 10U) : 0U;
    if (diag.connection_log_count == 0U && used < sizeof(log_text) - 1U) {
        snprintf(log_text + used, sizeof(log_text) - used, "No connection events recorded yet.");
    } else {
        for (uint16_t i = start; i < diag.connection_log_count && used < sizeof(log_text) - 1U; i++) {
            int written = snprintf(
                log_text + used,
                sizeof(log_text) - used,
                "%lld ms  %s\n",
                (long long)diag.connection_log[i].elapsed_ms,
                diag.connection_log[i].message);
            if (written <= 0) {
                break;
            }
            used += ((size_t)written < sizeof(log_text) - used) ? (size_t)written : sizeof(log_text) - used - 1U;
        }
    }
    lv_label_set_text(s_system_log, log_text);
}

static void ui_restart_cancel_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    if (s_restart_confirm != NULL) {
        lv_obj_del(s_restart_confirm);
        s_restart_confirm = NULL;
    }
}

static void ui_restart_now_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ha_client_stop();
#if defined(CONFIG_APP_PANEL_VARIANT_S3_480)
    if (wifi_mgr_force_transport_recover() != ESP_OK) {
        (void)wifi_mgr_force_reconnect();
    }
#endif
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
}

static void ui_restart_request_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    if (s_system_overlay == NULL || s_restart_confirm != NULL) {
        return;
    }

    s_restart_confirm = lv_obj_create(s_system_overlay);
    lv_obj_set_size(s_restart_confirm, LV_PCT(70), 210);
    lv_obj_center(s_restart_confirm);
    lv_obj_set_style_bg_color(s_restart_confirm, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_restart_confirm, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_restart_confirm, 16, LV_PART_MAIN);

    lv_obj_t *message = lv_label_create(s_restart_confirm);
    lv_label_set_text(message, "Restart the BETTA panel?\nSaved settings will be retained.");
    lv_obj_set_width(message, LV_PCT(90));
    lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(message, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *cancel = lv_btn_create(s_restart_confirm);
    lv_obj_set_size(cancel, 130, 50);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 24, -20);
    lv_obj_add_event_cb(cancel, ui_restart_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_label = lv_label_create(cancel);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);

    lv_obj_t *restart = lv_btn_create(s_restart_confirm);
    lv_obj_set_size(restart, 150, 50);
    lv_obj_align(restart, LV_ALIGN_BOTTOM_RIGHT, -24, -20);
    lv_obj_add_event_cb(restart, ui_restart_now_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *restart_label = lv_label_create(restart);
    lv_label_set_text(restart_label, "Restart");
    lv_obj_center(restart_label);

    lv_obj_move_foreground(s_restart_confirm);
}

static void ui_brightness_changed_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int value = (int)lv_slider_get_value(slider);
    (void)display_set_brightness_percent(value);

    if (s_brightness_value != NULL) {
        char label[16];
        snprintf(label, sizeof(label), "%d%%", value);
        lv_label_set_text(s_brightness_value, label);
    }

    if (lv_event_get_code(event) == LV_EVENT_RELEASED) {
        runtime_settings_t settings = {0};
        if (runtime_settings_load(&settings) == ESP_OK) {
            settings.display_brightness_percent = value;
            (void)runtime_settings_save(&settings);
            display_configure_night_mode(
                settings.display_brightness_percent,
                settings.display_night_brightness_percent,
                settings.display_night_mode,
                settings.display_night_start_hour,
                settings.display_night_start_minute,
                settings.display_day_start_hour,
                settings.display_day_start_minute);
        }
    }
}

static void ui_display_mode_changed_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    runtime_settings_t settings = {0};
    if (runtime_settings_load(&settings) != ESP_OK) {
        return;
    }

    if (s_night_mode_dropdown != NULL) {
        settings.display_night_mode = (int)lv_dropdown_get_selected(s_night_mode_dropdown);
        settings.display_night_mode_auto = (settings.display_night_mode == 2);
    }
    if (s_idle_timeout_dropdown != NULL) {
        static const int idle_seconds[] = {0, 30, 60, 120, 300, 600};
        uint32_t index = lv_dropdown_get_selected(s_idle_timeout_dropdown);
        if (index < (sizeof(idle_seconds) / sizeof(idle_seconds[0]))) {
            settings.display_idle_timeout_seconds = idle_seconds[index];
        }
    }

    (void)runtime_settings_save(&settings);
    display_configure_night_mode(
        settings.display_brightness_percent,
        settings.display_night_brightness_percent,
        settings.display_night_mode,
        settings.display_night_start_hour,
        settings.display_night_start_minute,
        settings.display_day_start_hour,
        settings.display_day_start_minute);
    display_configure_idle(settings.display_idle_timeout_seconds, settings.display_idle_brightness_percent);
}

static lv_obj_t *ui_system_header_button(
    lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t width, lv_event_cb_t cb)
{
    lv_obj_t *button = lv_obj_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_set_size(button, width, 42);
    lv_obj_set_pos(button, x, 9);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(button, 8);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, NAV_TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_width(label, width - 12);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(label);
    ui_pages_style_nav_button(button, label, false, false);
    return button;
}

static lv_obj_t *ui_system_create_shell(const char *title)
{
    lv_obj_t *screen = lv_scr_act();
    if (screen == NULL) {
        return NULL;
    }

    const lv_coord_t header_h = s_geometry.content_y > 64 ? s_geometry.content_y : 64;
    const lv_coord_t footer_h = s_geometry.nav_h > 0 ? s_geometry.nav_h : 60;

    s_system_overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(s_system_overlay);
    lv_obj_add_flag(s_system_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_system_overlay, ui_system_activity_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_set_size(s_system_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_system_overlay, lv_color_hex(APP_UI_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_system_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(s_system_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_obj_create(s_system_overlay);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, s_geometry.screen_w, header_h);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    LV_UNUSED(title);

    lv_obj_t *footer = lv_obj_create(s_system_overlay);
    lv_obj_remove_style_all(footer);
    lv_obj_set_size(footer, s_geometry.screen_w, footer_h);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(footer, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *home = lv_obj_create(footer);
    lv_obj_remove_style_all(home);
    lv_obj_set_size(home, 92, 42);
    lv_obj_center(home);
    lv_obj_add_flag(home, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(home, ui_system_overlay_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *home_label = lv_label_create(home);
    lv_label_set_text(home_label, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_label, TOPBAR_ICON_FONT, LV_PART_MAIN);
    lv_obj_center(home_label);
    ui_pages_style_nav_button(home, home_label, true, true);

    lv_obj_t *content = lv_obj_create(s_system_overlay);
    lv_obj_remove_style_all(content);
    lv_obj_set_pos(content, 0, header_h);
    lv_obj_set_size(content, s_geometry.screen_w, s_geometry.screen_h - header_h - footer_h);
    lv_obj_set_style_bg_color(content, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    ui_system_timeout_delete();
    s_system_timeout_timer = lv_timer_create(ui_system_timeout_cb, UI_SYSTEM_TIMEOUT_MS, NULL);
    if (s_system_timeout_timer != NULL) {
        lv_timer_set_repeat_count(s_system_timeout_timer, 1);
    }

    return content;
}

static void ui_system_overlay_show(void)
{
    if (s_system_overlay != NULL) {
        return;
    }

    lv_obj_t *content = ui_system_create_shell("System / Diagnostics");
    if (content == NULL) {
        return;
    }

    const lv_coord_t w = s_geometry.screen_w;
    const lv_coord_t margin = w <= 520 ? 14 : 24;
    const lv_coord_t gap = 10;
    const lv_coord_t back_w = w <= 520 ? 92 : 110;
    const lv_coord_t display_w = w <= 520 ? 110 : 130;
    const lv_coord_t restart_w = w <= 520 ? 110 : 130;

    ui_system_header_button(s_system_overlay, "Back", margin, back_w, ui_system_overlay_close_cb);
    ui_system_header_button(
        s_system_overlay, "Display",
        w - margin - restart_w - gap - display_w, display_w, ui_system_display_open_cb);
    ui_system_header_button(
        s_system_overlay, "Restart",
        w - margin - restart_w, restart_w, ui_restart_request_cb);

    /* Diagnostics belongs to the content page, not the action bar. */
    lv_obj_t *page_title = lv_label_create(content);
    lv_label_set_text(page_title, "System / Diagnostics");
    lv_obj_set_style_text_font(page_title, w <= 520 ? APP_FONT_TEXT_22 : APP_FONT_TEXT_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(page_title, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_opa(page_title, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_pos(page_title, margin, 10);
    lv_obj_set_width(page_title, w - (margin * 2));
    lv_label_set_long_mode(page_title, LV_LABEL_LONG_DOT);

    s_system_details = lv_label_create(content);
    lv_obj_set_pos(s_system_details, margin, w <= 520 ? 48 : 58);
    lv_obj_set_width(s_system_details, w - (margin * 2));
    lv_label_set_long_mode(s_system_details, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_system_details, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_system_details, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_opa(s_system_details, LV_OPA_COVER, LV_PART_MAIN);

    s_system_log = lv_label_create(content);
    lv_obj_set_width(s_system_log, w - (margin * 2) - 20);
    lv_label_set_long_mode(s_system_log, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_system_log, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_system_log, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_opa(s_system_log, LV_OPA_COVER, LV_PART_MAIN);

    /* Populate first, then measure.  The previous layout measured the empty
     * label, which placed the log on top of the status text. */
    ui_system_overlay_refresh();
    lv_obj_update_layout(s_system_details);

    lv_coord_t log_y = lv_obj_get_y(s_system_details) + lv_obj_get_height(s_system_details) + 12;
    lv_coord_t content_h = s_geometry.screen_h
        - (s_geometry.content_y > 64 ? s_geometry.content_y : 64)
        - (s_geometry.nav_h > 0 ? s_geometry.nav_h : 60);

    lv_obj_t *log_title = lv_label_create(content);
    lv_label_set_text(log_title, "RECENT HA CONNECTION LOG");
    lv_obj_set_style_text_font(log_title, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(log_title, lv_color_hex(APP_UI_COLOR_TOPBAR_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_opa(log_title, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_pos(log_title, margin, log_y);

    lv_obj_t *log_box = lv_obj_create(content);
    lv_obj_set_pos(log_box, margin, log_y + 24);
    lv_coord_t log_h = content_h - log_y - 30;
    if (log_h < 56) {
        log_h = 56;
    }
    lv_obj_set_size(log_box, w - (margin * 2), log_h);
    lv_obj_set_style_bg_color(log_box, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(log_box, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(log_box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(log_box, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_radius(log_box, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(log_box, 8, LV_PART_MAIN);
    lv_obj_add_flag(log_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(log_box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(log_box, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_parent(s_system_log, log_box);
    lv_obj_set_pos(s_system_log, 0, 0);
    lv_obj_set_width(s_system_log, LV_PCT(100));
    if (w <= 520) {
        lv_obj_set_style_text_font(s_system_log, APP_FONT_TEXT_14, LV_PART_MAIN);
    }

    lv_obj_move_foreground(s_system_overlay);
}

static void ui_system_display_show(void)
{
    if (s_system_overlay != NULL) {
        return;
    }

    lv_obj_t *content = ui_system_create_shell("Display Settings");
    if (content == NULL) {
        return;
    }

    const lv_coord_t w = s_geometry.screen_w;
    const lv_coord_t margin = w <= 520 ? 18 : 28;
    const lv_coord_t back_w = w <= 520 ? 92 : 110;

    ui_system_header_button(s_system_overlay, "Back", margin, back_w, ui_system_diagnostics_open_cb);

    lv_obj_t *page_title = lv_label_create(content);
    lv_label_set_text(page_title, "Display Settings");
    lv_obj_set_style_text_font(page_title, w <= 520 ? APP_FONT_TEXT_22 : APP_FONT_TEXT_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(page_title, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_opa(page_title, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_pos(page_title, margin, 10);

    const lv_coord_t header_h = s_geometry.content_y > 64 ? s_geometry.content_y : 64;
    const lv_coord_t footer_h = s_geometry.nav_h > 0 ? s_geometry.nav_h : 60;
    const lv_coord_t content_h = s_geometry.screen_h - header_h - footer_h;
    const lv_coord_t body_y = w <= 520 ? 50 : 62;
    const lv_coord_t body_h = content_h - body_y - 8;

    lv_obj_t *body = lv_obj_create(content);
    lv_obj_remove_style_all(body);
    lv_obj_set_pos(body, margin, body_y);
    lv_obj_set_size(body, w - (margin * 2), body_h);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);

    runtime_settings_t settings = {0};
    runtime_settings_set_defaults(&settings);
    (void)runtime_settings_load(&settings);

    /* The body must always exist.  Persisted settings override defaults when
     * available, but a transient NVS/read failure must never leave a blank
     * Display Settings page. */
    {
        lv_coord_t y = 4;
        lv_coord_t body_w = w - (margin * 2);
        lv_coord_t control_w = w <= 520 ? 150 : 180;

        lv_obj_t *brightness_label = lv_label_create(body);
        lv_label_set_text(brightness_label, "Brightness");
        lv_obj_set_style_text_font(brightness_label, APP_FONT_TEXT_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(brightness_label, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_opa(brightness_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_pos(brightness_label, 0, y);

        s_brightness_value = lv_label_create(body);
        char value[16];
        snprintf(value, sizeof(value), "%d%%", settings.display_brightness_percent);
        lv_label_set_text(s_brightness_value, value);
        lv_obj_set_style_text_font(s_brightness_value, APP_FONT_TEXT_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_brightness_value, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_opa(s_brightness_value, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_pos(s_brightness_value, body_w - 54, y);
        y += 38;

        s_brightness_slider = lv_slider_create(body);
        lv_slider_set_range(s_brightness_slider, 1, 100);
        lv_slider_set_value(s_brightness_slider, settings.display_brightness_percent, LV_ANIM_OFF);
        lv_obj_set_pos(s_brightness_slider, 0, y);
        lv_obj_set_size(s_brightness_slider, body_w - 12, 22);
        lv_obj_add_event_cb(s_brightness_slider, ui_brightness_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_event_cb(s_brightness_slider, ui_brightness_changed_cb, LV_EVENT_RELEASED, NULL);
        y += 62;

        lv_obj_t *night_label = lv_label_create(body);
        lv_label_set_text(night_label, "Night mode");
        lv_obj_set_style_text_font(night_label, APP_FONT_TEXT_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(night_label, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_opa(night_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_pos(night_label, 0, y + 10);

        s_night_mode_dropdown = lv_dropdown_create(body);
        lv_dropdown_set_options(s_night_mode_dropdown, "Day\nNight\nAuto");
        lv_dropdown_set_selected(s_night_mode_dropdown, (uint32_t)settings.display_night_mode);
        lv_obj_set_size(s_night_mode_dropdown, control_w, 46);
        lv_obj_set_pos(s_night_mode_dropdown, body_w - control_w - 4, y);
        lv_obj_add_event_cb(s_night_mode_dropdown, ui_display_mode_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
        y += 64;

        lv_obj_t *idle_label = lv_label_create(body);
        lv_label_set_text(idle_label, "Auto dim");
        lv_obj_set_style_text_font(idle_label, APP_FONT_TEXT_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(idle_label, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_opa(idle_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_pos(idle_label, 0, y + 10);

        s_idle_timeout_dropdown = lv_dropdown_create(body);
        lv_dropdown_set_options(s_idle_timeout_dropdown, "Off\n30 sec\n1 min\n2 min\n5 min\n10 min");
        uint32_t idle_index = 0;
        const int idle_seconds[] = {0, 30, 60, 120, 300, 600};
        for (uint32_t i = 0; i < (sizeof(idle_seconds) / sizeof(idle_seconds[0])); i++) {
            if (settings.display_idle_timeout_seconds == idle_seconds[i]) {
                idle_index = i;
                break;
            }
        }
        lv_dropdown_set_selected(s_idle_timeout_dropdown, idle_index);
        lv_obj_set_size(s_idle_timeout_dropdown, control_w, 46);
        lv_obj_set_pos(s_idle_timeout_dropdown, body_w - control_w - 4, y);
        lv_obj_add_event_cb(s_idle_timeout_dropdown, ui_display_mode_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
        y += 70;

        lv_obj_t *note = lv_label_create(body);
        lv_label_set_text(note, "Changes are saved to the panel and are also visible in the web admin settings.");
        lv_obj_set_width(note, body_w - 8);
        lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(note, APP_FONT_TEXT_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(note, lv_color_hex(APP_UI_COLOR_TOPBAR_MUTED), LV_PART_MAIN);
        lv_obj_set_style_text_opa(note, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_pos(note, 0, y);
    }

    lv_obj_move_foreground(s_system_overlay);
}

static void ui_status_gesture_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED) {
        /* Treat the adjacent HA/Wi-Fi chips as one protected status area.
         * Use our own timer so entry always requires a deliberate 3-second
         * hold rather than LVGL's shorter platform long-press threshold. */
        s_status_gesture_armed = true;
        s_status_gesture_started_ms = lv_tick_get();
    } else if (code == LV_EVENT_PRESSING && s_status_gesture_armed) {
        uint32_t now = lv_tick_get();
        if ((uint32_t)(now - s_status_gesture_started_ms) >= UI_SYSTEM_HOLD_MS) {
            s_status_gesture_armed = false;
            s_status_gesture_started_ms = 0U;
            ui_system_overlay_show();
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_status_gesture_armed = false;
        s_status_gesture_started_ms = 0U;
    }
}

static void ui_pages_create_topbar(lv_obj_t *screen)
{
    const lv_coord_t topbar_h = s_geometry.content_y;

    s_topbar = lv_obj_create(screen);
    lv_obj_remove_style_all(s_topbar);
    lv_obj_set_size(s_topbar, s_geometry.screen_w, topbar_h);
    lv_obj_set_pos(s_topbar, 0, 0);
    lv_obj_clear_flag(s_topbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_topbar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_topbar, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_topbar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_topbar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(s_topbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_topbar, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(s_topbar, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_topbar, 0, LV_PART_MAIN);

    s_date_label = lv_label_create(s_topbar);
    lv_obj_set_width(s_date_label, 220);
    lv_obj_align(s_date_label, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_text_color(s_date_label, lv_color_hex(APP_UI_COLOR_TOPBAR_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_date_label, TOPBAR_DATE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_date_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(s_date_label, "--.--.----");

    s_time_label = lv_label_create(s_topbar);
    lv_obj_set_width(s_time_label, 220);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(s_time_label, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_time_label, TOPBAR_TIME_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(s_time_label, "--:--");

    s_api_icon = lv_label_create(s_topbar);
    lv_obj_set_width(s_api_icon, 86);
    lv_obj_align(s_api_icon, LV_ALIGN_RIGHT_MID, -72, 0);
    ui_pages_style_topbar_chip(s_api_icon);
        char api_text[32] = {0};
    snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_CLOSE);
    lv_label_set_text(s_api_icon, api_text);
    lv_obj_add_flag(s_api_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(s_api_icon, 16);
    lv_obj_add_event_cb(s_api_icon, ui_status_gesture_cb, LV_EVENT_ALL, NULL);

        s_wifi_icon = lv_label_create(s_topbar);
    lv_obj_set_width(s_wifi_icon, 48);
    lv_obj_align(s_wifi_icon, LV_ALIGN_RIGHT_MID, -12, 0);
    ui_pages_style_topbar_chip(s_wifi_icon);
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_CLOSE);
    lv_obj_add_flag(s_wifi_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(s_wifi_icon, 16);
    lv_obj_add_event_cb(s_wifi_icon, ui_status_gesture_cb, LV_EVENT_ALL, NULL);
    }

static void ui_pages_create_nav(lv_obj_t *screen)
{
    s_nav_bar = lv_obj_create(screen);
    lv_obj_remove_style_all(s_nav_bar);
    lv_obj_set_size(s_nav_bar, s_geometry.screen_w, s_geometry.nav_h);
    lv_obj_set_pos(s_nav_bar, 0, s_geometry.screen_h - s_geometry.nav_h);
    lv_obj_clear_flag(s_nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_nav_bar, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_nav_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_nav_bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(s_nav_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_nav_bar, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(s_nav_bar, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_nav_home_button = lv_obj_create(s_nav_bar);
    lv_obj_remove_style_all(s_nav_home_button);
    lv_obj_set_ext_click_area(s_nav_home_button, 14);
    lv_obj_add_flag(s_nav_home_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_nav_home_button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_nav_home_button, ui_nav_home_button_event_cb, LV_EVENT_CLICKED, NULL);

    s_nav_home_label = lv_label_create(s_nav_home_button);
    lv_label_set_text(s_nav_home_label, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(s_nav_home_label, TOPBAR_ICON_FONT, LV_PART_MAIN);
    lv_obj_center(s_nav_home_label);

    for (uint16_t i = 0; i < (APP_MAX_PAGES - 1); i++) {
        lv_obj_t *btn = lv_obj_create(s_nav_bar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_ext_click_area(btn, 10);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, ui_nav_extra_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, "");
        lv_obj_set_style_text_font(label, NAV_TEXT_FONT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_center(label);

        s_nav_extra_buttons[i] = btn;
        s_nav_extra_labels[i] = label;
        s_nav_extra_page_index[i] = APP_MAX_PAGES;
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_pages_init(void)
{
    memset(s_pages, 0, sizeof(s_pages));
    s_page_count = 0;
    s_current_index = -1;
    s_background = NULL;
    s_topbar = NULL;
    s_content_box = NULL;
    s_date_label = NULL;
    s_time_label = NULL;
    s_wifi_icon = NULL;
    s_api_icon = NULL;
    s_system_overlay = NULL;
    s_system_details = NULL;
    s_system_log = NULL;
    s_restart_confirm = NULL;
    s_brightness_slider = NULL;
    s_brightness_value = NULL;
    s_night_mode_dropdown = NULL;
    s_idle_timeout_dropdown = NULL;
    ui_system_timeout_delete();
    s_status_gesture_armed = false;
    s_status_gesture_started_ms = 0U;
    s_nav_bar = NULL;
    s_nav_home_button = NULL;
    s_nav_home_label = NULL;
    memset(s_nav_extra_buttons, 0, sizeof(s_nav_extra_buttons));
    memset(s_nav_extra_labels, 0, sizeof(s_nav_extra_labels));
    memset(s_nav_extra_page_index, 0, sizeof(s_nav_extra_page_index));

#if LV_USE_LOTTIE && APP_UI_BETTA_LOTTIE_ASSET
    /* Screen will be cleaned below; the overlay is owned by the active screen.
     * Drop our handles and free the lottie render buffer to avoid a leak. */
    s_betta_overlay = NULL;
    s_betta_lottie  = NULL;
    if (s_betta_buf != NULL) {
        lv_free(s_betta_buf);
        s_betta_buf = NULL;
    }
    s_betta_taps    = 0U;
    s_betta_last_ms = 0U;
#endif

    lv_obj_t *screen = lv_scr_act();
    ui_pages_refresh_geometry(screen);
    lv_obj_clean(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(APP_UI_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    s_background = lv_obj_create(screen);
    lv_obj_remove_style_all(s_background);
    lv_obj_set_size(s_background, s_geometry.screen_w, s_geometry.screen_h);
    lv_obj_set_pos(s_background, 0, 0);
    lv_obj_clear_flag(s_background, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_background, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_background, lv_color_hex(APP_UI_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_background, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_background, 0, LV_PART_MAIN);

    s_content_box = lv_obj_create(screen);
    lv_obj_remove_style_all(s_content_box);
    lv_obj_set_size(s_content_box, s_geometry.content_w, s_geometry.content_h);
    lv_obj_set_pos(s_content_box, s_geometry.content_x, s_geometry.content_y);
    lv_obj_clear_flag(s_content_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_content_box, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_content_box, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_content_box, LV_OPA_COVER, LV_PART_MAIN);
#if APP_UI_REWORK_V2
    lv_obj_set_style_border_width(s_content_box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_content_box, lv_color_hex(APP_UI_COLOR_CONTENT_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(s_content_box, LV_OPA_70, LV_PART_MAIN);
#else
    lv_obj_set_style_border_width(s_content_box, 0, LV_PART_MAIN);
#endif
    lv_obj_set_style_pad_all(s_content_box, 0, LV_PART_MAIN);

    ui_pages_create_topbar(screen);
    ui_pages_create_nav(screen);

    time_t now = time(NULL);
    struct tm info = {0};
    localtime_r(&now, &info);
    ui_pages_set_topbar_datetime(&info);
    ui_pages_set_topbar_status(false, false, false, false);
    ui_pages_apply_tab_style(0);
}

void ui_pages_reset(void)
{
    ui_pages_init();
}

lv_obj_t *ui_pages_add(const char *page_id, const char *title)
{
    if (s_page_count >= APP_MAX_PAGES || page_id == NULL || page_id[0] == '\0' || s_content_box == NULL) {
        return NULL;
    }

    uint16_t index = s_page_count;
    snprintf(s_pages[index].id, sizeof(s_pages[index].id), "%s", page_id);
    snprintf(s_pages[index].title, sizeof(s_pages[index].title), "%s", (title && title[0]) ? title : page_id);

    lv_obj_t *container = lv_obj_create(s_content_box);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, APP_CONTENT_BOX_WIDTH, APP_CONTENT_BOX_HEIGHT);
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    s_pages[index].container = container;

    s_page_count++;
    ui_pages_apply_tab_style((uint16_t)(s_current_index >= 0 ? s_current_index : 0));
    return container;
}

bool ui_pages_show_index(uint16_t index)
{
    if (index >= s_page_count) {
        return false;
    }

    for (uint16_t i = 0; i < s_page_count; i++) {
        if (s_pages[i].container == NULL) {
            continue;
        }
        if (i == index) {
            lv_obj_clear_flag(s_pages[i].container, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_pages[i].container, LV_OBJ_FLAG_HIDDEN);
        }
    }
    s_current_index = (int16_t)index;
    ui_pages_apply_tab_style(index);
    if (s_show_cb != NULL) {
        s_show_cb(s_pages[index].id, index);
    }
    return true;
}

bool ui_pages_show(const char *page_id)
{
    if (page_id == NULL) {
        return false;
    }
    for (uint16_t i = 0; i < s_page_count; i++) {
        if (strncmp(page_id, s_pages[i].id, APP_MAX_PAGE_ID_LEN) == 0) {
            return ui_pages_show_index(i);
        }
    }
    return false;
}

bool ui_pages_next(void)
{
    if (s_page_count == 0) {
        return false;
    }
    uint16_t next = (uint16_t)(((s_current_index < 0 ? 0 : s_current_index) + 1) % s_page_count);
    return ui_pages_show_index(next);
}

const char *ui_pages_current_id(void)
{
    if (s_current_index < 0 || (uint16_t)s_current_index >= s_page_count) {
        return "";
    }
    return s_pages[s_current_index].id;
}

uint16_t ui_pages_count(void)
{
    return s_page_count;
}

void ui_pages_set_topbar_status(
    bool wifi_connected, bool wifi_setup_ap_active, bool api_connected, bool api_initial_sync_done)
{
    lv_color_t on = lv_color_hex(APP_UI_COLOR_TOPBAR_STATUS_ON);
    lv_color_t off = lv_color_hex(APP_UI_COLOR_TOPBAR_STATUS_OFF);

    if (s_wifi_icon != NULL) {
        char wifi_text[32] = {0};
        snprintf(wifi_text, sizeof(wifi_text), "%s", LV_SYMBOL_CLOSE);
        lv_color_t wifi_color = off;
        if (wifi_setup_ap_active) {
            snprintf(wifi_text, sizeof(wifi_text), "%s %s", ui_i18n_get("topbar.ap", "AP"), LV_SYMBOL_WIFI);
            wifi_color = on;
        } else if (wifi_connected) {
            snprintf(wifi_text, sizeof(wifi_text), "%s", LV_SYMBOL_WIFI);
            wifi_color = on;
        }
        lv_label_set_text(s_wifi_icon, wifi_text);
        lv_obj_set_style_text_color(s_wifi_icon, wifi_color, LV_PART_MAIN);
    }

    if (s_api_icon != NULL) {
        char api_text[32] = {0};
        snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_CLOSE);
        lv_color_t api_color = off;
        if (api_connected) {
            if (api_initial_sync_done) {
                snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_OK);
            } else {
                snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_REFRESH);
            }
            api_color = on;
        }
        lv_label_set_text(s_api_icon, api_text);
        lv_obj_set_style_text_color(s_api_icon, api_color, LV_PART_MAIN);
    }
}

void ui_pages_set_topbar_datetime(const struct tm *timeinfo)
{
    if (timeinfo == NULL) {
        return;
    }

    char date_buf[32] = {0};
    char time_buf[16] = {0};
    snprintf(
        date_buf, sizeof(date_buf), "%02d.%02d.%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

    if (s_date_label != NULL) {
        lv_label_set_text(s_date_label, date_buf);
    }
    if (s_time_label != NULL) {
        lv_label_set_text(s_time_label, time_buf);
    }
}
