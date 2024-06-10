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
#include "esp_lcd_backlight.h"
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
#include "key.h"

static const char *TAG = "gui";

static SemaphoreHandle_t xGuiSemaphore;

static lv_obj_t *scr_stack[GUI_MAX_SCREEN_STACK];

static int rt_app_status = RTAPP_STATUS_STOPPED;

static bool display_on = true;

static void power_key_press_callback(int key, enum key_action action)
{
    if (action == KEY_ACTION_SHORT_PRESS) {
        if (display_on) {
            disp_set_on(0);
            lvgl_set_backlight(0);
            display_on = false;
            gui_lock();
        } else {
            disp_set_on(1);
            lvgl_set_backlight(100);
            display_on = true;
            gui_unlock();
        }
    }
}

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

    gui_fs_init();

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

    key_register_callback("power", power_key_press_callback);

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

static const char *required[] = {
    "storage",
    NULL
};

RTAPP_EXPORT(gui, gui_init, NULL, gui_get_status, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, required, NULL);
