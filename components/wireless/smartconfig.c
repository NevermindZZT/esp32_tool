/**
 * @file smartconfig.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief smartconfig app
 * @version 1.0.0
 * @date 2024-06-10
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "shell.h"
#include "rtam.h"
#include "gui.h"
#include "launcher.h"
#include "widgets/label/lv_label.h"


static const char *TAG = "smartconfig";

static int rt_app_status = RTAPP_STATUS_STOPPED;

static lv_obj_t *screen = NULL;

extern const lv_image_dsc_t icon_app_smartconfig;

lv_obj_t *smartconfig_get_screen(void);

void smartconfig_global_event_cb(lv_event_t *event)
{
    static bool gesture_enabled = false;
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target(event);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT && gesture_enabled) {
            gesture_enabled = false;
            if (obj == smartconfig_get_screen()) {
                rtamTerminate("smartconfig");
            } else {
                gui_back();
            }
            lv_indev_wait_release(lv_indev_active());
        }
    } else if (code == LV_EVENT_PRESSED) {
        lv_point_t point;
        lv_indev_get_point(lv_indev_active(), &point);
        if (point.x < 16) {
            gesture_enabled = true;
        }
    } else if (code == LV_EVENT_RELEASED) {
        gesture_enabled = false;
    }
}

lv_obj_t *smartconfig_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void smartconfig_init_screen(void)
{
    lv_obj_t *scr = smartconfig_get_screen();
    lv_obj_add_event_cb(scr, smartconfig_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, smartconfig_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, smartconfig_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "smart config is running\nplease config with your phone");
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_center(label);
}

static void smartconfig_resume(void)
{
    gui_push_screen(smartconfig_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
}

static int smartconfig_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    smartconfig_init_screen();
    smartconfig_resume();
    rt_app_status = RTAPP_STATUS_RUNNING;
    return 0;
}

static int smartconfig_stop(void)
{
    rt_app_status = RTAPP_STATUS_STOPPING;

    esp_smartconfig_stop();

    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    rt_app_status = RTAPP_STATUS_STOPPED;
    return 0;
}

static int smartconfig_get_status(void)
{
    return rt_app_status;
}

static const char *required[] = {
    "gui",
    "wifi_service",
    NULL
};

static RtamInfo smartconfig_info = {
    .icon = (void *) GUI_APP_ICON(smartconfig),
};

RTAPP_EXPORT(smartconfig, smartconfig_init, smartconfig_stop, smartconfig_get_status, 0, required, &smartconfig_info);