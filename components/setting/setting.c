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

static int rt_app_status = RTAPP_STATUS_STOPPED;

static lv_obj_t *screen = NULL;

static lv_obj_t* (*item_get_screen[SETTING_ID_MAX])(void) = {
    [SETTING_MECHANICE] = setting_create_mechanice,
    [SETTING_ABOUT] = setting_create_about,
};

static int setting_stop(void);

static void setting_item_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED) {
        setting_item_id_t id = (size_t) lv_event_get_user_data(event);
        if (id < SETTING_ID_MAX) {
            gui_push_screen(item_get_screen[id](), LV_SCR_LOAD_ANIM_MOVE_LEFT);
        }
    }
}

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
    lv_obj_add_event_cb(scr, setting_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, setting_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, setting_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *menu = lv_menu_create(scr);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(menu);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_EVENT_BUBBLE);


    // create main page
    lv_obj_t *main_page = lv_menu_page_create(menu, "Setting");

    lv_obj_t *mechanice = gui_create_menu_item(main_page, lv_obj_get_style_bg_color(menu, 0),
                                               (void *)&icon_app_setting_mechanice, "Mechanice");
    lv_obj_add_event_cb(mechanice, setting_item_event_cb, LV_EVENT_CLICKED, (void *)SETTING_MECHANICE);

    lv_obj_t *about = gui_create_menu_item(main_page, lv_obj_get_style_bg_color(menu, 0),
                                           (void *)&icon_app_setting_about, "About");
    lv_obj_add_event_cb(about, setting_item_event_cb, LV_EVENT_CLICKED, (void *)SETTING_ABOUT);

    lv_menu_set_page(menu, main_page);
}

static void setting_resume(void)
{
    // lv_scr_load_anim(setting_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    gui_push_screen(setting_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
}

static int setting_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    setting_init_screen();
    setting_resume();
    rt_app_status = RTAPP_STATUS_RUNNING;
    return 0;
}

static int setting_stop(void)
{
    rt_app_status = RTAPP_STATUS_STOPPING;
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    rt_app_status = RTAPP_STATUS_STOPPED;
    return 0;
}

static int setting_get_status(void)
{
    return rt_app_status;
}

static const char *required[] = {
    "gui",
    NULL
};

static RtamInfo setting_info = {
    .icon = (void *) &icon_app_setting
};

RTAPP_EXPORT(setting, setting_init, setting_stop, setting_get_status, 0, required, &setting_info);