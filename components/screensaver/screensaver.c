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
#include "time.h"
#include "sys/time.h"
#include "key.h"

static const char *tag = "screensaver";

static lv_obj_t *screen = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *time_label = NULL;
static bool run = true;
static bool display_on = true;

extern const lv_image_dsc_t icon_app_screensaver;

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

void screensaver_global_event_cb(lv_event_t *event)
{
    // static bool gesture_enabled = false;
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target(event);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT/* && gesture_enabled*/) {
            // gesture_enabled = false;
            if (obj == screensaver_get_screen()) {
                // gui_back();
                rtamTerminate("screensaver");
            }
            lv_indev_wait_release(lv_indev_active());
        }
    } else if (code == LV_EVENT_PRESSED) {
        // lv_point_t point;
        // lv_indev_get_point(lv_indev_active(), &point);
        // if (point.x < 16) {
        //     gesture_enabled = true;
        // }
    } else if (code == LV_EVENT_RELEASED) {
        // gesture_enabled = false;
    }
}

static void screensaver_update(void)
{
    char date_buf[32];
    char time_buf[32];
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &timeinfo);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);

    lv_label_set_text(date_label, date_buf);
    lv_label_set_text(time_label, time_buf);
}

lv_obj_t *screensaver_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void screensaver_init_screen(void)
{
    gui_lock();
    lv_obj_t *scr = screensaver_get_screen();
    lv_obj_add_event_cb(scr, screensaver_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, screensaver_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, screensaver_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *background = lv_img_create(scr);
    lv_img_set_src(background, "S:/spiflash/data/wallpaper.jpg");
    lv_obj_center(background);
    
    time_label = lv_label_create(scr);
    lv_obj_set_width(time_label, LV_PCT(90));
    lv_obj_set_height(time_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align_to(time_label, scr, LV_ALIGN_TOP_LEFT, 8, 48);

    date_label = lv_label_create(scr);
    lv_obj_set_width(date_label, LV_PCT(90));
    lv_obj_set_height(date_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(date_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    screensaver_update();

    gui_unlock();
}

static void screensaver_task(void *arg)
{
    while (run)
    {
        gui_lock();
        screensaver_update();
        gui_unlock();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

static void screensaver_resume(void)
{
    gui_push_screen(screensaver_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    xTaskCreate(screensaver_task, "screensaver_task", 2048, NULL, 10, NULL);
}

static RtAppErr screensaver_init(void)
{
    run = true;
    screensaver_init_screen();
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