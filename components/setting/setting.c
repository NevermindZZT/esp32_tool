/**
 * @file setting.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting
 * @version 1.0.0
 * @date 2024-05-10
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "display/lv_display.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "font/lv_symbol_def.h"
#include "freertos/projdefs.h"
#include "indev/lv_indev.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include "rtam_cfg_user.h"
#include "setting_provider.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_idf_version.h"
#include "esp_flash.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "rtam.h"
#include "shell.h"
#include "widgets/label/lv_label.h"
#include "widgets/menu/lv_menu.h"
#include <stdint.h>
#include "gui.h"
#include "launcher.h"
#include "setting.h"

static const char *TAG = "setting";

static lv_obj_t *screen = NULL;

static setting_item_config_t setting_main_config[] = {
    {"WiFi",       (void *)GUI_APP_RES_PNG(setting, wifi),       SETTING_ITEM_SWITCH,   SETTING_KEY_WIFI_ENABLED, NULL},
    {"Bluetooth",  (void *)GUI_APP_RES_PNG(setting, bt),         SETTING_ITEM_SWITCH,   SETTING_KEY_BT_ENABLED,   NULL},
    {"Brightness", (void *)GUI_APP_RES_PNG(setting, brightness), SETTING_ITEM_CUSTOM,   SETTING_KEY_SCR_BRIGHT,   setting_create_brightness},
    {"About",      (void *)GUI_APP_RES_PNG(setting, about),      SETTING_ITEM_CUSTOM,   NULL,                     setting_create_about},

    {NULL, NULL, 0, NULL, NULL},
};

static RtAppErr setting_stop(void);

lv_obj_t *setting_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void setting_init_screen(void)
{
    lv_obj_t *scr = setting_get_screen();
    gui_set_global_gesture_callback(setting_gesture_callback);

    setting_create_page(scr, setting_main_config, "Setting");
}

static void setting_resume(void)
{
    // lv_scr_load_anim(setting_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    gui_push_screen(setting_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
}

static RtAppErr setting_init(void)
{
    setting_init_screen();
    setting_resume();
    return RTAM_OK;
}

static RtAppErr setting_stop(void)
{
    gui_set_global_gesture_callback(NULL);
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = setting_init,
    .stop = setting_stop,
};

static const char *required[] = {
    "gui",
    NULL
};

static const RtAppDependencies dependencies = {
    .required = required,
};

static const RtamInfo setting_info = {
    .icon = (void *) GUI_APP_ICON(setting),
    // .label = "设置",
};

RTAPP_EXPORT(setting, &interface, 0, &dependencies, &setting_info);