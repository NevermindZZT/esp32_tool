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

static lv_obj_t *screen = NULL;

extern const lv_image_dsc_t icon_app_smartconfig;

lv_obj_t *smartconfig_get_screen(void);

static int smartconfig_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == smartconfig_get_screen()) {
            rtamTerminate("smartconfig");
        } else {
            gui_back();
        }
        return 0;
    }
    return -1;
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
    gui_set_global_gesture_callback(smartconfig_gesture_callback);

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

static RtAppErr smartconfig_init(void)
{
    smartconfig_init_screen();
    smartconfig_resume();
    return RTAM_OK;
}

static RtAppErr smartconfig_stop(void)
{
    esp_smartconfig_stop();

    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = smartconfig_init,
    .stop = smartconfig_stop,
};

static const char *required[] = {
    "gui",
    "wifi_service",
    NULL
};

static const RtAppDependencies dependencies = {
    .required = required,
};

static const RtamInfo smartconfig_info = {
    .icon = (void *) GUI_APP_ICON(smartconfig),
};

RTAPP_EXPORT(smartconfig, &interface, 0, &dependencies, &smartconfig_info);