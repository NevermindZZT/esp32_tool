/**
 * @file mech_watch.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief mechanical watch
 * @version 1.0.0
 * @date 2024-07-24
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj_pos.h"
#include "gui.h"
#include "lvgl.h"
#include "misc/lv_types.h"
#include "time.h"
#include "sys/time.h"
#include "screensaver.h"

static lv_obj_t *date_label = NULL;
static lv_obj_t *time_label = NULL;

static lv_obj_t *hour_pointer = NULL;
static lv_obj_t *minute_pointer = NULL;
static lv_obj_t *second_pointer = NULL;

static void mech_watch_update(void)
{
    char date_buf[32];
    char time_buf[32];
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    int32_t width, height;
    
    width = lv_obj_get_width(hour_pointer);
    height = lv_obj_get_height(hour_pointer);
    if (width != 0 && height != 0) {
        lv_obj_set_style_transform_pivot_x(hour_pointer, width / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(hour_pointer, height / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(hour_pointer, 
                                            (timeinfo.tm_hour * 60 + timeinfo.tm_min) * 3600 / (12 *  60),
                                            LV_PART_MAIN);
    }

    width = lv_obj_get_width(minute_pointer);
    height = lv_obj_get_height(minute_pointer);
    if (width != 0 && height != 0) {
        lv_obj_set_style_transform_pivot_x(minute_pointer, width / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(minute_pointer, height / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(minute_pointer,
                                            (timeinfo.tm_min * 60 + timeinfo.tm_sec) * 3600 / (60 * 60),
                                            LV_PART_MAIN);
    }
    
    width = lv_obj_get_width(second_pointer);
    height = lv_obj_get_height(second_pointer);
    if (width != 0 && height != 0) {
        lv_obj_set_style_transform_pivot_x(second_pointer, width / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(second_pointer, height / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(second_pointer, timeinfo.tm_sec * 3600 / 60, LV_PART_MAIN);
    }
}

lv_obj_t *mech_watch_get_screen(void)
{
    gui_lock();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *background = lv_img_create(screen);
    lv_img_set_src(background, "S:/spiflash/data/screensaver/MechWatch/background.png");
    lv_obj_center(background);
    
    hour_pointer = lv_img_create(screen);
    lv_img_set_src(hour_pointer, "S:/spiflash/data/screensaver/MechWatch/hour_pointer.png");
    lv_obj_center(hour_pointer);

    minute_pointer = lv_img_create(screen);
    lv_img_set_src(minute_pointer, "S:/spiflash/data/screensaver/MechWatch/minute_pointer.png");
    lv_obj_center(minute_pointer);

    second_pointer = lv_img_create(screen);
    lv_img_set_src(second_pointer, "S:/spiflash/data/screensaver/MechWatch/second_pointer.png");
    lv_obj_center(second_pointer);

    mech_watch_update();

    gui_unlock();

    return screen;
}


static void screensaver_task(void *arg)
{
    while (screensaver_running())
    {
        gui_lock();
        mech_watch_update();
        gui_unlock();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

struct screensaver_config mech_watch_saver = {
    .name = "Mech Watch",
    .get_screen = mech_watch_get_screen,
    .task = screensaver_task,
};
