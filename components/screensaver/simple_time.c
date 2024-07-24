/**
 * @file simple_time.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief simple time screensaver
 * @version 1.0.0
 * @date 2024-07-23
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "gui.h"
#include "lvgl.h"
#include "misc/lv_types.h"
#include "time.h"
#include "sys/time.h"
#include "screensaver.h"

static lv_obj_t *date_label = NULL;
static lv_obj_t *time_label = NULL;

static void simple_time_update(void)
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

lv_obj_t *simple_time_get_screen(void)
{
    gui_lock();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *background = lv_img_create(screen);
    lv_img_set_src(background, "S:/spiflash/data/wallpaper.jpg");
    lv_obj_center(background);
    
    time_label = lv_label_create(screen);
    lv_obj_set_width(time_label, LV_PCT(90));
    lv_obj_set_height(time_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align_to(time_label, screen, LV_ALIGN_TOP_LEFT, 8, 48);

    date_label = lv_label_create(screen);
    lv_obj_set_width(date_label, LV_PCT(90));
    lv_obj_set_height(date_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(date_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    simple_time_update();

    gui_unlock();

    return screen;
}


static void screensaver_task(void *arg)
{
    while (screensaver_running())
    {
        gui_lock();
        simple_time_update();
        gui_unlock();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

struct screensaver_config simple_time_saver = {
    .name = "Simple Time",
    .get_screen = simple_time_get_screen,
    .task = screensaver_task,
};
