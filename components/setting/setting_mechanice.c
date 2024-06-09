/**
 * @file setting_mechanice.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting mechanice screen
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "setting.h"
#include "gui.h"

static const char *TAG = "setting_mechanice";

static void brightness_slider_event_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int32_t val = lv_slider_get_value(slider);
    lval_set_backlight(val);
}

lv_obj_t *setting_create_mechanice(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_event_cb(screen, setting_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(screen, setting_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, setting_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *menu = lv_menu_create(screen);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(menu);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    lv_obj_t *section;
    lv_obj_t *main_page = lv_menu_page_create(menu, "Mechanice");

    section = lv_menu_section_create(main_page);
    lv_obj_set_style_bg_color(section, lv_color_hex(0x000000), LV_PART_MAIN);
    setting_create_slider(section, (void *)GUI_APP_RES_PNG(setting, brightness), "Brightness", 
                          1, 100, 100, brightness_slider_event_cb);

    lv_menu_set_page(menu, main_page);

    return screen;
}
