/**
 * @file gui.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 1.0.0
 * @date 2024-04-25
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "display/lv_display.h"
#include "draw/lv_draw_buf.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "indev/lv_indev.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "rtam.h"
#include "shell.h"
#include <stdint.h>
#include "gui.h"

static const char *TAG = "gui";

static SemaphoreHandle_t xGuiSemaphore;

static lv_obj_t *scr_stack[GUI_MAX_SCREEN_STACK];

static int rt_app_status = RTAPP_STATUS_STOPPED;

#if defined(CONFIG_LV_USE_LOG)
static void lv_log_cb(lv_log_level_t level, const char *buf)
{
    ESP_LOGI(TAG, "%s", buf);
}
#endif

static void lv_tick_task(void *arg)
{
   (void)arg;
   lv_tick_inc(10);
}

static uint32_t lv_get_tick_cb(void)
{
    return esp_timer_get_time() / 1000;
}

BaseType_t gui_lock(void)
{
    return xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
}

void gui_unlock(void)
{
    xSemaphoreGive(xGuiSemaphore);
}

void gui_push_screen(lv_obj_t *screen, lv_screen_load_anim_t anim_type)
{
    for (int i = 0; i < GUI_MAX_SCREEN_STACK; i++) {
        if (scr_stack[i] == NULL) {
            scr_stack[i] = screen;
            break;
        }
    }
    lv_scr_load_anim(screen, anim_type, anim_type == LV_SCR_LOAD_ANIM_NONE ? 0 : 200, 0, false);
}

bool gui_pop_screen(lv_screen_load_anim_t anim_type)
{
    for (int i = 0; i < GUI_MAX_SCREEN_STACK; i++) {
        if (scr_stack[i] == NULL && lv_screen_active() == scr_stack[i - 1]) {
            lv_scr_load_anim(scr_stack[i - 2], anim_type, anim_type == LV_SCR_LOAD_ANIM_NONE ? 0 : 200, 0, true);
            scr_stack[i - 1] = NULL;
            return true;
        }
    }
    return false;
}

bool gui_pop_to_frist(lv_screen_load_anim_t anim_type)
{
    if (scr_stack[0] != NULL && lv_screen_active() != scr_stack[0]) {
        lv_obj_t *current_screen = lv_screen_active();
        for (int i = 1; i < GUI_MAX_SCREEN_STACK; i++) {
            if (scr_stack[i] == NULL) {
                break;
            } else if (scr_stack[i] == current_screen) {
                scr_stack[i] = NULL;
                continue;
            } else {
                lv_obj_del(scr_stack[i]);
                scr_stack[i] = NULL;
            }
        }
        lv_scr_load_anim(scr_stack[0], anim_type, anim_type == LV_SCR_LOAD_ANIM_NONE ? 0 : 200, 0, true);
        return true;
    }
    return false;
}

static void gui_task(void *param)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    touch_calibration_t cal_data = {16, 258, 1, 278};
    
    lv_init();
    lvgl_driver_init();
    touch_driver_set_calibrate(&cal_data);

    lv_tick_set_cb(lv_get_tick_cb);
#if defined(CONFIG_LV_USE_LOG)
    lv_log_register_print_cb(lv_log_cb);
#endif

    lv_display_t *display = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_display_set_flush_cb(display, disp_driver_flush);
    lv_display_flush_ready(display);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_driver_read);

    void *buf1 = heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_DMA);
    void *buf2 = heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_DMA);
    lv_display_set_buffers(display, buf1, buf2, DISP_BUF_SIZE * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"
    };
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));

    while (1) {
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == gui_lock()) {
            lv_timer_handler();
            rt_app_status = RTAPP_STATUS_RUNNING;
            gui_unlock();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static int gui_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    xTaskCreatePinnedToCore(gui_task, "guiTask", 8192, NULL, 8, NULL, 0);
    return 0;
}

static int gui_get_status(void)
{
    return rt_app_status;
}
RTAPP_EXPORT(gui, gui_init, NULL, gui_get_status, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, NULL, NULL);

static lv_obj_t *lvgl_test_get_label(void)
{
    static lv_obj_t *label = NULL;
    if (label == NULL)
    {
        label = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
    return label;
}

static void lvgl_test(uint32_t color, char *text)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(color), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    lv_obj_t *label = lvgl_test_get_label();
    lv_label_set_text(label, text);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
lvglTest, lvgl_test, test lval display\nlvglTest <color> <text>);

static char buf[240 * 40 * 2];

static void lvgl_test2(uint32_t color)
{
    for (int i = 0; i < 240 * 40; i++)
    {
        buf[i * 2] = color >> 8;
        buf[i * 2 + 1] = color & 0xff;
    }
    lv_area_t area = {
        .x1 = 0,
        .y1 = 0,
        .x2 = 240,
        .y2 = 40,
    };
    disp_driver_flush(lv_display_get_default(), &area, (uint8_t *) buf);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
lvglTest2, lvgl_test2, test lval display\nlvglTest <color>);
