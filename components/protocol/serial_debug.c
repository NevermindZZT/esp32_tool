/**
 * @file serial_debug.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug
 * @version 1.0.0
 * @date 2024-06-11
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "display/lv_display.h"
#include "esp_intr_alloc.h"
#include "esp_system.h"
#include "esp_log.h"
#include "font/lv_font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "layouts/lv_layout.h"
#include "lvgl.h"
#include "gui.h"
#include "launcher.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "rtam.h"
#include "protocol_common.h"
#include "serial_debug_uart.h"
#include "serial_debug_i2c.h"
#include "serial_debug_spi.h"
#include "widgets/label/lv_label.h"

static const char *TAG = "serial_debug";

static lv_obj_t *screen = NULL;

static lv_obj_t* serial_debug_create_pin_map_screen(void);
static lv_obj_t* serial_debug_get_screen(void);

void serial_debug_global_event_cb(lv_event_t *event)
{
    // static bool gesture_enabled = false;
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target(event);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT /*&& gesture_enabled*/) {
            // gesture_enabled = false;
            if (obj == serial_debug_get_screen()) {
                rtamExit("serial_debug");
            } else {
                gui_back();
            }
            lv_indev_wait_release(lv_indev_active());
        } else if (dir == LV_DIR_BOTTOM) {
            if (obj == serial_debug_get_screen()) {
                gui_push_screen(serial_debug_create_pin_map_screen(), LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
            }
        } else if (dir == LV_DIR_TOP) {
            if (obj != serial_debug_get_screen()) {
                gui_pop_screen(LV_SCR_LOAD_ANIM_MOVE_TOP);
            }
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

static lv_obj_t* serial_debug_create_pin_map_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *pin_map = protocol_create_pin_map(scr);
    
    lv_obj_center(pin_map);

    return scr;
}

static lv_obj_t* serial_debug_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void serial_debug_init_screen(void)
{
    lv_obj_t *scr = serial_debug_get_screen();

    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, serial_debug_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Serial Debug");
    lv_obj_set_style_pad_ver(title, 4, LV_PART_MAIN);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_update_layout(title);

    lv_obj_t *tabview = lv_tabview_create(scr);
    lv_obj_set_size(tabview, 
                    lv_display_get_horizontal_resolution(NULL),
                    lv_display_get_vertical_resolution(NULL) - lv_obj_get_height(title));
    lv_obj_align_to(tabview, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_20, LV_PART_MAIN);
    
    lv_obj_t *uart_tab = lv_tabview_add_tab(tabview, "UART");
    lv_obj_t *i2c_tab = lv_tabview_add_tab(tabview, "I2C");
    lv_obj_t *spi_tab = lv_tabview_add_tab(tabview, "SPI");

    lv_obj_t *uart_label = lv_label_create(uart_tab);
    serial_debug_uart_init_info(uart_label);
    lv_obj_set_style_text_font(uart_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *i2c_label = lv_label_create(i2c_tab);
    serial_debug_i2c_init_info(i2c_label);
    lv_obj_set_style_text_font(i2c_label, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *spi_label = lv_label_create(spi_tab);
    serial_debug_spi_init_info(spi_label);
    lv_obj_set_style_text_font(spi_label, &lv_font_montserrat_16, LV_PART_MAIN);
}

static RtAppErr serial_debug_resume(void)
{
    serial_debug_init_screen();
    gui_push_screen(serial_debug_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    return RTAM_OK;
}

static RtAppErr serial_debug_suspend(void)
{
    serial_debug_uart_deinit_info();
    serial_debug_i2c_deinit_info();
    serial_debug_spi_deinit_info();

    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static RtAppErr serial_debug_init(void)
{
    protocol_reset_pin();
#if CONFIG_PROTOCOL_PIN20_IO == 43 /** old hardware */
    serial_debug_uart_init(15, 13, 11, 9);
    serial_debug_i2c_init(17, 19);
    serial_debug_spi_init(16, 14, 12, 10, 8, 3);
#else
    serial_debug_uart_init(9, 7, 5, 3);
    serial_debug_i2c_init(15, 17);
    serial_debug_spi_init(18, 16, 14, 12, 10, 8);
#endif
    return RTAM_OK;
}

#if CONFIG_PROTOCOL_SERIAL_DEBUG == 1
static RtAppErr serial_debug_stop(void)
{
    serial_debug_uart_deinit();
    serial_debug_i2c_deinit();
    serial_debug_spi_deinit();
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = serial_debug_init,
    .stop = serial_debug_stop,
    .resume = serial_debug_resume,
    .suspend = serial_debug_suspend,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]){
        "gui",
        "launcher",
        NULL
    },
};

extern const lv_image_dsc_t icon_app_serial_debug;
static const RtamInfo serial_debug_info = {
    .label = "serial debug",
    .icon = (void *) GUI_APP_ICON(serial_debug),
};

RTAPP_EXPORT(serial_debug, &interface, RTAPP_FLAG_BACKGROUND, &dependencies, &serial_debug_info);
#endif