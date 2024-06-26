/**
 * @file launcher.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief launcher
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
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "layouts/lv_layout.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_event.h"
#include "misc/lv_palette.h"
#include "misc/lv_style.h"
#include "misc/lv_style_gen.h"
#include "misc/lv_types.h"
#include "rtam_cfg_user.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "rtam.h"
#include "shell.h"
#include <stdint.h>
#include "gui.h"
#include "widgets/button/lv_button.h"
#include "widgets/label/lv_label.h"
#include "widgets/menu/lv_menu.h"

static const char *TAG = "launcher";

static lv_obj_t *screen = NULL;

static lv_obj_t *launcher_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void launcher_app_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED) {
        RtApp *app = lv_event_get_user_data(event);
        rtamLaunch(app->name);
    } else if (code == LV_EVENT_GESTURE) {
        lv_indev_wait_release(lv_indev_active());
    }
}

static void launcher_init_screen(void)
{
    RtApp *apps;
    int apps_num;
    lv_obj_t *scr = launcher_get_screen();
    apps_num = rtamGetApps(&apps);
    lv_obj_add_event_cb(scr, launcher_app_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_t *status_bar = gui_create_status_bar(scr, true, "Apps");

    lv_obj_t *menu = lv_menu_create(scr);
    lv_obj_set_size(menu, 
                    lv_display_get_horizontal_resolution(NULL),
                    lv_display_get_vertical_resolution(NULL) - lv_obj_get_height(status_bar));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN);
    // lv_obj_center(menu);
    lv_obj_set_pos(menu, 0, lv_obj_get_height(status_bar));
    lv_obj_align_to(menu, status_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

    for (int i = 0; i < apps_num; i++) {
        if (apps[i].flags.value & RTAPP_FLAG_SERVICE) {
            ESP_LOGI(TAG, "Find service app: %s", apps[i].name);
            continue;
        }
        ESP_LOGI(TAG, "Find app: %s", apps[i].name);

        lv_obj_t *item = gui_create_menu_item(main_page, 
                                              lv_obj_get_style_bg_color(menu, 0), 
                                              ((RtamInfo *) (apps[i].info))->icon,
                                              ((RtamInfo *) apps[i].info)->label 
                                                    ? ((RtamInfo *) apps[i].info)->label : apps[i].name);

        lv_obj_add_event_cb(item, launcher_app_event_cb, LV_EVENT_CLICKED, &apps[i]);
    }

    lv_menu_set_page(menu, main_page);
}

void launcher_go_home(lv_screen_load_anim_t anim_type, bool auto_del)
{
    // lv_scr_load_anim(launcher_get_screen(), anim_type, anim_type == LV_SCR_LOAD_ANIM_NONE ? 0 : 200, 0, auto_del);
    gui_pop_to_frist(LV_SCR_LOAD_ANIM_FADE_OUT);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), home, launcher_go_home, go home);

static void launcher_resume(void)
{
    gui_push_screen(launcher_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    // lv_scr_load_anim(launcher_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

static RtAppErr launcher_init(void)
{
    launcher_init_screen();
    gui_unlock();
    launcher_resume();
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = launcher_init,
    .stop = NULL,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]){
        "gui",
        NULL
    },
};

RTAPP_EXPORT(launcher, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, &dependencies, NULL);
