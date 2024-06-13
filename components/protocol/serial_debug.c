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

static int rt_app_status = RTAPP_STATUS_STOPPED;

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
                rtamTerminate("serial_debug");
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

    protocol_reset_pin_map();
    protocol_set_pin_map(42, "CS", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin_map(41, "MCLK", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin_map(40, "MOSI", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin_map(39, "MISO", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin_map(47, "SCL", lv_palette_main(LV_PALETTE_CYAN));
    protocol_set_pin_map(21, "SDA", lv_palette_main(LV_PALETTE_CYAN));
    protocol_set_pin_map(12, "TX1", lv_palette_main(LV_PALETTE_BLUE));
    protocol_set_pin_map(11, "RX1", lv_palette_main(LV_PALETTE_BLUE));
    protocol_set_pin_map(10, "RTS1", lv_palette_main(LV_PALETTE_BLUE));
    protocol_set_pin_map(9, "CTS1", lv_palette_main(LV_PALETTE_BLUE));
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

static void serial_debug_resume(void)
{
    gui_push_screen(serial_debug_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
}

static int serial_debug_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    serial_debug_init_screen();
    serial_debug_resume();
    
    serial_debug_uart_init(12, 11, 10, 9);
    serial_debug_i2c_init(21, 47);
    rt_app_status = RTAPP_STATUS_RUNNING;
    return 0;
}

static int serial_debug_stop(void)
{
    rt_app_status = RTAPP_STATUS_STOPPING;
    serial_debug_uart_deinit();
    serial_debug_i2c_deinit();

    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    rt_app_status = RTAPP_STATUS_STOPPED;
    return 0;
}

static int serial_debug_get_status(void)
{
    return rt_app_status;
}

static const char *required[] = {
    "gui",
    "launcher",
    NULL
};

extern const lv_image_dsc_t icon_app_serial_debug;
static RtamInfo serial_debug_info = {
    .label = "serial debug",
    .icon = (void *) GUI_APP_ICON(serial_debug),
};

RTAPP_EXPORT(serial_debug, serial_debug_init, serial_debug_stop, serial_debug_get_status, 0, required, &serial_debug_info);