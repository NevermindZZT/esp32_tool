/**
 * @file setting_about.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting about screen
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */

#include "core/lv_obj_pos.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "setting.h"
#include "battery.h"

lv_obj_t *setting_create_about(void)
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

    lv_obj_t *main_page = lv_menu_page_create(menu, "About");


    esp_chip_info_t chip_info;
    uint32_t flash_size;
    lv_obj_t *cont;

    esp_chip_info(&chip_info);

    cont = lv_menu_cont_create(main_page);
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "Core: %d", chip_info.cores);

#if defined(PROJECT_VER)
    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "Version: %s", PROJECT_VER);
#endif

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "Revision: %d", chip_info.revision);

    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        cont = lv_menu_cont_create(main_page);
        label = lv_label_create(cont);
        lv_label_set_text_fmt(label, "Flash: %luMB", (unsigned long) flash_size / (1024 * 1024));
    }
    
    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "Free: %lu", (unsigned long) esp_get_free_heap_size());

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "IDF: %d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text_fmt(label, "Battery: %dmV", battery_get_voltage());

    lv_menu_set_page(menu, main_page);

    return screen;
}
