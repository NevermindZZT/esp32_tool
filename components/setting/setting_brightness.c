/**
 * @file setting_brightness.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting brightness screen
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */

#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "cpost.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "setting.h"
#include "gui.h"
#include "setting_provider.h"

static const char *TAG = "setting_brightness";

static void brightness_slider_event_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int32_t val = lv_slider_get_value(slider);
    lvgl_set_backlight(val);
    if (setting_set(SETTING_KEY_SCR_BRIGHT, val) != 0) {
        ESP_LOGE(TAG, "Failed to save screen brightness");
    }
}

lv_obj_t *setting_create_brightness(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t * slider = gui_create_slider(screen, LV_PALETTE_BLUE, 8);
    lv_obj_set_style_size(slider, 80, LV_PCT(90), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xcccccc), LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_center(slider);
    lv_slider_set_range(slider, 1, 100);
    lv_slider_set_value(slider, setting_get(SETTING_KEY_SCR_BRIGHT, 100), LV_ANIM_ON);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, slider);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_RELEASED, slider);

    lv_obj_t *img = lv_img_create(screen);
    lv_img_set_src(img, GUI_APP_RES_PNG(setting, brightness));
    lv_obj_set_style_size(img, 48, 48, LV_PART_MAIN);
    lv_obj_align_to(img, slider, LV_ALIGN_BOTTOM_MID, 0, -6);

    return screen;
}
