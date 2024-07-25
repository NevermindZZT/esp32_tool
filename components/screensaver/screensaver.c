/**
 * @file screensaver.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief screensaver
 * @version 1.0.0
 * @date 2024-06-26
 * @copyright (c) 2024 Letter All rights reserved.
 */

#include "core/lv_obj_pos.h"
#include "disp_driver.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "lvgl.h"
#include "gui.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "rtam.h"
#include "setting_provider.h"
#include "time.h"
#include "sys/time.h"
#include "key.h"
#include "screensaver.h"

static const char *tag = "screensaver";

static lv_obj_t *screen = NULL;
static bool run = true;
static bool display_on = true;
struct screensaver_config *selected_saver;

extern const lv_image_dsc_t icon_app_screensaver;


extern struct screensaver_config simple_time_saver;
extern struct screensaver_config mech_watch_saver;

static struct screensaver_config *savers[] = {
    [0] = &simple_time_saver,
    [1] = &mech_watch_saver,
};


lv_obj_t *screensaver_get_screen(void);

static int key_press_callback(enum key_code code, enum key_action action)
{
    if (code == KEY_CODE_POWER) {
        if (action == KEY_ACTION_SHORT_PRESS) {
            if (display_on) {
                disp_set_on(0);
                lvgl_set_backlight(0);
                display_on = false;
                gui_lock();
            } else {
                gui_unlock();
                disp_set_on(1);
                lvgl_set_backlight(100);
                display_on = true;
            }
            return 0;
        }
    }
    return -1;
}

static int screensaver_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == screensaver_get_screen()) {
            rtamTerminate("screensaver");
        } else {
            gui_back();
        }
        return 0;
    }
    return -1;
}


lv_obj_t *screensaver_get_screen(void)
{
    return screen;
}


bool screensaver_running(void)
{
    return run;
}

static void screensaver_resume(void)
{
    gui_push_screen(screensaver_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    xTaskCreate(selected_saver->task,
                "screensaver_task", 2048, NULL, 10, NULL);
}

static RtAppErr screensaver_init(void)
{
    run = true;
    
    selected_saver = savers[0];
    char *saver_type = setting_get_str(SETTING_KEY_SCREENSAVER_TYPE, "Simple Time");

    for (int i = 0; i < sizeof(savers) / sizeof(savers[0]); i++) {
        if (strcmp(saver_type, savers[i]->name) == 0) {
            selected_saver = savers[i];
            break;
        }
    }

    screen = selected_saver->get_screen();
    gui_set_global_gesture_callback(screensaver_gesture_callback);
    screensaver_resume();
    
    setenv("TZ", "CST-8", 1);
    tzset();

    key_add_callback(KEY_CODE_POWER, key_press_callback);
    return RTAM_OK;
}

static RtAppErr screensaver_stop(void)
{
    run = false;
    gui_back();
    screen = NULL;

    key_remove_callback(KEY_CODE_POWER, key_press_callback);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = screensaver_init,
    .stop = screensaver_stop,
};

static const char *required[] = {
    "gui",
    NULL
};

static const RtAppDependencies dependencies = {
    .required = required,
};

RTAPP_EXPORT(screensaver, &interface, RTAPP_FLAG_SERVICE, &dependencies, NULL);