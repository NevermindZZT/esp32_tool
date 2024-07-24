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
#include "misc/lv_types.h"


typedef enum {
    SETTING_ITEM_SUB_PAGE,
    SETTING_ITEM_SWITCH,
    SETTING_ITEM_RADIO,
    SETTING_ITEM_CUSTOM,
} setting_item_type_t;

typedef struct {
    const char *name;
    void *icon;
    setting_item_type_t type;
    const char *key;
    void *data;

    lv_obj_t *widget;
} setting_item_config_t;

typedef lv_obj_t *(*setting_item_custom_get_screen)(void);

extern const lv_image_dsc_t icon_app_setting;
extern const lv_image_dsc_t icon_app_setting_about;
extern const lv_image_dsc_t icon_app_setting_brightness;
extern const lv_image_dsc_t icon_app_setting_wifi;
extern const lv_image_dsc_t icon_app_setting_bt;
extern const lv_image_dsc_t icon_app_setting_screensaver;

lv_obj_t *setting_get_screen(void);
int setting_gesture_callback(lv_dir_t dir);

uint32_t setting_get_nvs_handle(void);

lv_obj_t *setting_create_page(lv_obj_t *screen, setting_item_config_t *item_configs, const char *title);

lv_obj_t *setting_create_brightness(void);
lv_obj_t *setting_create_about(void);

#endif /* __SETTING_H__ */
