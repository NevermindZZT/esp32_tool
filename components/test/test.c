/**
 * @file test.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief test
 * @version 1.0.0
 * @date 2024-05-12
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
#include "launcher.h"
#include "gui.h"


static void slider_event_cb(lv_event_t * e);
static lv_obj_t * slider_label;
static lv_obj_t *test_get_screen(void);

/**
 * A default slider with a label displaying the current value
 */
void lv_example_slider_1(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(test_get_screen());
    lv_obj_center(slider);
    lv_obj_set_width(slider, 200);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_set_style_anim_duration(slider, 2000, 0);
    /*Create a label below the slider*/
    slider_label = lv_label_create(test_get_screen());
    lv_label_set_text(slider_label, "0%");

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

/**
 * Slider with opposite direction
 */
void lv_example_slider_4(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(test_get_screen());
    lv_obj_center(slider);
    lv_obj_set_width(slider, 200);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*Reverse the direction of the slider*/
    lv_slider_set_range(slider, 100, 0);
    /*Create a label below the slider*/
    slider_label = lv_label_create(test_get_screen());
    lv_label_set_text(slider_label, "0%");

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}



static const char *TAG = "test";

static lv_obj_t *screen = NULL;

static void test_global_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT) {
            rtamTerminate("test");
            lv_indev_wait_release(lv_indev_active());
        }
    }
}

static lv_obj_t *test_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void test_init_screen(void)
{
    lv_obj_t *scr = test_get_screen();
    lv_obj_add_event_cb(scr, test_global_event_cb, LV_EVENT_GESTURE, NULL);

    lv_example_slider_1();
}

static void test_resume(void)
{
    lv_scr_load_anim(test_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

static RtAppErr test_init(void)
{
    test_init_screen();
    test_resume();
    return RTAM_OK;
}

static RtAppErr test_stop(void)
{
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = test_init,
    .stop = test_stop,
};

static const char *required[] = {
    "gui",
    NULL
};

static const RtAppDependencies dependencies = {
    .required = required,
};

extern const lv_image_dsc_t icon_app_test;

static const RtamInfo test_info = {
    .icon = (void *) GUI_APP_ICON(test),
};

RTAPP_EXPORT(test, &interface, 0, &dependencies, &test_info);