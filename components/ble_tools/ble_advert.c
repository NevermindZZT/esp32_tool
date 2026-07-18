/**
 * @file ble_advert.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Advertiser Shell Tool
 * @version 1.0.0
 * @date 2026-07-18
 * @copyright (c) 2026 Letter All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_device.h"
#include "bt_manager.h"
#include "shell.h"
#include "rtam.h"

static const char *TAG = "ble_advert";

static bool advert_running = false;
static bool gap_registered = false;

/* Advertising data buffers */
static uint8_t adv_data_buf[31] = {0};
static uint8_t adv_data_len = 0;

/* Advertising params */
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_config = {
    .set_scan_rsp       = false,
    .include_name       = false,
    .include_txpower    = true,
    .min_interval       = 0x0006,
    .max_interval       = 0x0010,
    .appearance         = 0x0000,
    .manufacturer_len   = 0,
    .p_manufacturer_data = NULL,
    .service_data_len   = 0,
    .p_service_data     = NULL,
    .service_uuid_len   = 0,
    .p_service_uuid     = NULL,
    .flag               = 0x06,
};

static bool is_app_running(const char *name)
{
    RtApp *apps;
    int apps_num = rtamGetApps(&apps);
    for (int i = 0; i < apps_num; i++) {
        if (strcmp(apps[i].name, name) == 0) {
            return true;
        }
    }
    return false;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        advert_running = true;
        ESP_LOGI(TAG, "Advertising started");
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        advert_running = false;
        ESP_LOGI(TAG, "Advertising stopped");
        break;
    default:
        break;
    }
}

static uint8_t hex_char_to_byte(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static int hex_str_to_bytes(const char *hex, uint8_t *bytes, int max_len)
{
    int len = strlen(hex);
    if (len % 2) return -1;
    int byte_len = len / 2;
    if (byte_len > max_len) byte_len = max_len;
    for (int i = 0; i < byte_len; i++) {
        bytes[i] = (hex_char_to_byte(hex[i * 2]) << 4) | hex_char_to_byte(hex[i * 2 + 1]);
    }
    return byte_len;
}

/* ---- Shell Command Group ---- */

#include "shell_cmd_group.h"

static void ble_advert_cmd_init(void)
{
    if (bt_manager_get_ref() > 0) {
        printf("Error: BT already in use by another app.\n");
        return;
    }
    if (is_app_running("ble_remote") || is_app_running("ble_scan")) {
        printf("Warning: BLE app registered, may conflict.\n");
    }

    bt_manager_init();
    if (!gap_registered) {
        esp_ble_gap_register_callback(gap_event_handler);
        gap_registered = true;
    }
    printf("BLE Advertiser initialized\n");
}

static void ble_advert_cmd_deinit(void)
{
    if (advert_running) {
        esp_ble_gap_stop_advertising();
        advert_running = false;
    }
    bt_manager_deinit();
    gap_registered = false;
    printf("BLE Advertiser deinitialized\n");
}

static void ble_advert_cmd_start(void)
{
    if (advert_running) {
        printf("Already advertising\n");
        return;
    }
    if (!bt_manager_is_ble_ready()) {
        printf("BLE not initialized, run init first\n");
        return;
    }

    esp_err_t ret;

    if (adv_data_len > 0) {
        ret = esp_ble_gap_config_adv_data_raw(adv_data_buf, adv_data_len);
        if (ret != ESP_OK) {
            printf("Failed to set adv data: %s\n", esp_err_to_name(ret));
            return;
        }
    } else {
        esp_ble_gap_set_device_name("ESP32-Tool");
        adv_config.include_name = true;
        ret = esp_ble_gap_config_adv_data(&adv_config);
        if (ret != ESP_OK) {
            printf("Failed to config adv: %s\n", esp_err_to_name(ret));
            return;
        }
    }

    printf("Starting advertising...\n");
}

static void ble_advert_cmd_stop(void)
{
    if (!advert_running) {
        printf("Not advertising\n");
        return;
    }
    esp_ble_gap_stop_advertising();
    printf("Stopping advertising...\n");
}

static void ble_advert_cmd_data(int argc, void *argv)
{
    if (argc < 1) {
        printf("Usage: ble_advert data <hex> or ble_advert data clear\n");
        return;
    }

    const char *arg = (const char *)argv;
    if (strcmp(arg, "clear") == 0) {
        adv_data_len = 0;
        memset(adv_data_buf, 0, sizeof(adv_data_buf));
        printf("Advertising data cleared\n");
        return;
    }

    int len = hex_str_to_bytes(arg, adv_data_buf, 31);
    if (len < 0) {
        printf("Invalid hex string (must be even length)\n");
        return;
    }
    adv_data_len = len;
    printf("Set advertising data (%d bytes): ", len);
    for (int i = 0; i < len; i++) {
        printf("%02x", adv_data_buf[i]);
    }
    printf("\n");
}

static void ble_advert_cmd_name(int argc, void *argv)
{
    if (argc < 1) {
        printf("Usage: ble_advert name <name>\n");
        return;
    }
    const char *name = (const char *)argv;
    int name_len = strlen(name);
    if (name_len > 28) name_len = 28;

    adv_data_buf[0] = name_len + 1;
    adv_data_buf[1] = 0x09;
    memcpy(&adv_data_buf[2], name, name_len);
    adv_data_len = name_len + 2;

    printf("Set device name: %s\n", name);
}

static void ble_advert_cmd_interval(int argc, void *argv)
{
    if (argc < 2) {
        printf("Usage: ble_advert interval <min> <max>\n");
        printf("  units: 0.625ms, typical: 32(20ms) 160(100ms) 320(200ms)\n");
        return;
    }
    adv_params.adv_int_min = atoi((const char *)argv);
    adv_params.adv_int_max = atoi((const char *)(argv + sizeof(void *)));
    printf("Set interval: min=%d max=%d (x0.625ms)\n",
           adv_params.adv_int_min, adv_params.adv_int_max);
}

static void ble_advert_cmd_type(int argc, void *argv)
{
    if (argc < 1) {
        printf("Usage: ble_advert type <type>\n");
        printf("  0=ADV_IND (connectable scannable)\n");
        printf("  1=ADV_NONCONN_IND (non-connectable)\n");
        printf("  2=ADV_SCAN_IND (scannable)\n");
        return;
    }
    int t = atoi((const char *)argv);
    switch (t) {
    case 0: adv_params.adv_type = ADV_TYPE_IND; break;
    case 1: adv_params.adv_type = ADV_TYPE_NONCONN_IND; break;
    case 2: adv_params.adv_type = ADV_TYPE_SCAN_IND; break;
    default:
        printf("Invalid type\n");
        return;
    }
    printf("Set adv type: %d\n", t);
}

static void ble_advert_show(void)
{
    printf("=== BLE Advertiser Config ===\n");
    printf("Status: %s\n", advert_running ? "Advertising" : "Stopped");
    printf("BT ready: %s\n", bt_manager_is_ble_ready() ? "Yes" : "No");
    printf("Interval: min=%d max=%d (x0.625ms)\n",
           adv_params.adv_int_min, adv_params.adv_int_max);
    printf("Adv type: ");
    switch (adv_params.adv_type) {
    case ADV_TYPE_IND: printf("ADV_IND\n"); break;
    case ADV_TYPE_NONCONN_IND: printf("ADV_NONCONN_IND\n"); break;
    case ADV_TYPE_SCAN_IND: printf("ADV_SCAN_IND\n"); break;
    default: printf("%d\n", adv_params.adv_type);
    }
    printf("Channel: ");
    if (adv_params.channel_map == ADV_CHNL_ALL) printf("37+38+39\n");
    else printf("0x%02x\n", adv_params.channel_map);
    printf("Adv data (%d bytes): ", adv_data_len);
    for (int i = 0; i < adv_data_len; i++) printf("%02x", adv_data_buf[i]);
    printf("\n");
}

static ShellCommand ble_advert_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, init, ble_advert_cmd_init,
        init\r\nble_advert init - init BLE for advertising),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, deinit, ble_advert_cmd_deinit,
        deinit\r\nble_advert deinit - deinit BLE advertising),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, start, ble_advert_cmd_start,
        start\r\nble_advert start - start BLE advertising),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stop, ble_advert_cmd_stop,
        stop\r\nble_advert stop - stop BLE advertising),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, data, ble_advert_cmd_data,
        data\r\nble_advert data <hex|clear> - set/clear adv data),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, name, ble_advert_cmd_name,
        name\r\nble_advert name <name> - set device name),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, interval, ble_advert_cmd_interval,
        interval\r\nble_advert interval <min> <max> - set adv interval),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, type, ble_advert_cmd_type,
        type\r\nble_advert type <0|1|2> - set adv type),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, show, ble_advert_show,
        show\r\nble_advert show - show current config),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
ble_advert, ble_advert_group, ble_advert);
