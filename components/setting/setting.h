/**
 * @file setting.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SETTING_H__
#define __SETTING_H__

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

typedef enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
} lv_menu_builder_variant_t;

typedef enum {
    SETTING_MECHANICE,
    SETTING_ABOUT,

    SETTING_ID_MAX,
} setting_item_id_t;

extern const lv_image_dsc_t icon_app_setting;
extern const lv_image_dsc_t icon_app_setting_about;
extern const lv_image_dsc_t icon_app_setting_mechanice;
extern const lv_image_dsc_t icon_app_setting_brightness;

lv_obj_t *setting_get_screen(void);
void setting_global_event_cb(lv_event_t *event);
lv_obj_t *setting_create_text(lv_obj_t *parent, const void *icon, const char *txt,
                              lv_menu_builder_variant_t builder_variant);
lv_obj_t *setting_create_slider(lv_obj_t *parent, const void *icon, const char *txt,
                                int32_t min, int32_t max, int32_t val, lv_event_cb_t event_cb);

lv_obj_t *setting_create_mechanice(void);
lv_obj_t *setting_create_about(void);

#endif /* __SETTING_H__ */
