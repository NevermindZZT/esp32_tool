/**
 * @file ble_remote.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE HID Remote Control (RTAM App)
 * @version 1.0.0
 * @date 2026-07-18
 * @copyright (c) 2026 Letter All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "hid_dev.h"
#include "bt_manager.h"
#include "gui.h"
#include "launcher.h"
#include "rtam.h"
#include "shell.h"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style_gen.h"
#include "misc/lv_types.h"
#include "widgets/button/lv_button.h"
#include "widgets/label/lv_label.h"

static const char *TAG = "ble_remote";

#define HIDD_DEVICE_NAME            "ESP32-Remote"

static bool inited = false;
static uint16_t hid_conn_id = 0;
static bool sec_conn = false;

static lv_obj_t *screen = NULL;
static lv_obj_t *status_label = NULL;

/* Keyboard scan codes for D-pad */
#define KEY_UP      HID_KEY_UP_ARROW       /* 0x52 (82) */
#define KEY_DOWN    HID_KEY_DOWN_ARROW     /* 0x51 (81) */
#define KEY_LEFT    HID_KEY_LEFT_ARROW     /* 0x50 (80) */
#define KEY_RIGHT   HID_KEY_RIGHT_ARROW    /* 0x4F (79) */
#define KEY_OK      HID_KEY_RETURN         /* 0x28 (40) - Enter */

static uint8_t hidd_service_uuid128[] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x03c0,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void ble_remote_send_key(uint8_t key)
{
    if (!sec_conn) {
        ESP_LOGW(TAG, "Not connected, cannot send key");
        return;
    }
    ESP_LOGI(TAG, "Send key: 0x%02x", key);
    /* Press */
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &key, 1);
    vTaskDelay(pdMS_TO_TICKS(30));
    /* Release */
    uint8_t zero = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &zero, 0);
}

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch (event) {
    case ESP_HIDD_EVENT_REG_FINISH:
        if (param->init_finish.state == ESP_HIDD_INIT_OK) {
            esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
            esp_ble_gap_config_adv_data(&hidd_adv_data);
        }
        break;
    case ESP_BAT_EVENT_REG:
        break;
    case ESP_HIDD_EVENT_DEINIT_FINISH:
        break;
    case ESP_HIDD_EVENT_BLE_CONNECT:
        ESP_LOGI(TAG, "Connected");
        hid_conn_id = param->connect.conn_id;
        sec_conn = true;
        if (status_label) {
            gui_lock();
            lv_label_set_text(status_label, "Connected");
            gui_unlock();
        }
        break;
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
        ESP_LOGI(TAG, "Disconnected");
        sec_conn = false;
        esp_ble_gap_start_advertising(&hidd_adv_params);
        if (status_label) {
            gui_lock();
            lv_label_set_text(status_label, "Disconnected");
            gui_unlock();
        }
        break;
    case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT:
    case ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT:
        break;
    default:
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        ESP_LOGI(TAG, "Pair success: %d", param->ble_security.auth_cmpl.success);
        break;
    default:
        break;
    }
}

/* ---- Shell Command Group ---- */

#include "shell_cmd_group.h"

static void ble_remote_cmd_start(void)
{
    if (inited) {
        ESP_LOGW(TAG, "Already initialized");
        return;
    }
    bt_manager_init();

    esp_hidd_profile_init();
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    inited = true;
    ESP_LOGI(TAG, "BLE Remote started");
}

static void ble_remote_cmd_send(int key)
{
    ble_remote_send_key(key);
}

static ShellCommand ble_remote_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, start, ble_remote_cmd_start,
        start\r\nble_remote start - start BLE HID remote),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, send, ble_remote_cmd_send,
        send\r\nble_remote send <key> - send consumer key),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
ble_remote, ble_remote_group, ble_remote);

/* ---- GUI Button Callbacks ---- */

static void btn_up_cb(lv_event_t *e)
{
    (void)e;
    ble_remote_send_key(KEY_UP);
}
static void btn_down_cb(lv_event_t *e)
{
    (void)e;
    ble_remote_send_key(KEY_DOWN);
}
static void btn_left_cb(lv_event_t *e)
{
    (void)e;
    ble_remote_send_key(KEY_LEFT);
}
static void btn_right_cb(lv_event_t *e)
{
    (void)e;
    ble_remote_send_key(KEY_RIGHT);
}
static void btn_ok_cb(lv_event_t *e)
{
    (void)e;
    ble_remote_send_key(KEY_OK);
}

/* ---- GUI Screen ---- */

static int ble_remote_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == screen) {
            rtamTerminate("ble_remote");
        } else {
            gui_back();
        }
        return 0;
    }
    return -1;
}

static lv_obj_t *create_btn(lv_obj_t *parent, const char *text,
                            lv_event_cb_t cb, lv_align_t align, int x_ofs, int y_ofs)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 60, 60);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);
    return btn;
}

static void ble_remote_init_screen(void)
{
    if (screen) {
        return;
    }
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    /* Status bar */
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, sec_conn ? "Connected" : "Disconnected");
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, LV_PART_MAIN);

    /* D-Pad buttons */
    int offset = 65;

    create_btn(screen, LV_SYMBOL_UP, btn_up_cb,
               LV_ALIGN_CENTER, 0, -offset);
    create_btn(screen, LV_SYMBOL_DOWN, btn_down_cb,
               LV_ALIGN_CENTER, 0, offset);
    create_btn(screen, LV_SYMBOL_LEFT, btn_left_cb,
               LV_ALIGN_CENTER, -offset, 0);
    create_btn(screen, LV_SYMBOL_RIGHT, btn_right_cb,
               LV_ALIGN_CENTER, offset, 0);
    create_btn(screen, LV_SYMBOL_OK, btn_ok_cb,
               LV_ALIGN_CENTER, 0, 0);
}

/* ---- RTAM Lifecycle ---- */

static RtAppErr ble_remote_start(void)
{
    if (inited) {
        return RTAM_OK;
    }
    bt_manager_init();

    esp_hidd_profile_init();
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    inited = true;
    ESP_LOGI(TAG, "BLE Remote started");
    return RTAM_OK;
}

static RtAppErr ble_remote_stop(void)
{
    inited = false;
    sec_conn = false;
    bt_manager_deinit();
    return RTAM_OK;
}

static RtAppErr ble_remote_resume(void)
{
    ble_remote_init_screen();
    gui_push_screen(screen, LV_SCR_LOAD_ANIM_FADE_IN);
    gui_add_global_gesture_callback(ble_remote_gesture_callback);
    return RTAM_OK;
}

static RtAppErr ble_remote_suspend(void)
{
    gui_remove_global_gesture_callback(ble_remote_gesture_callback);
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    status_label = NULL;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .suspend = ble_remote_suspend,
    .resume  = ble_remote_resume,
    .start   = ble_remote_start,
    .stop    = ble_remote_stop,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]) {
        "gui",
        "launcher",
        NULL
    },
    .conflicted = (const char *[]) {
        "ble_scan",
        NULL
    },
};

extern const lv_image_dsc_t icon_app_ble_remote;
static const RtamInfo ble_remote_info = {
    .label = "BLE Remote",
    .icon  = (void *) GUI_APP_ICON(ble_remote),
};

RTAPP_EXPORT(ble_remote, &interface, RTAPP_FLAG_BACKGROUND, &dependencies, &ble_remote_info);
