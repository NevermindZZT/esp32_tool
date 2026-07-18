/**
 * @file bt_manager.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BT controller lifecycle manager with reference counting
 * @version 1.0.0
 * @date 2026-07-18
 * @copyright (c) 2026 Letter All rights reserved.
 */
#include "bt_manager.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"

static const char *TAG = "bt_manager";

static int bt_ref_count = 0;
static bool bt_initialized = false;
static bool bt_sniffer_mode = false;

esp_err_t bt_manager_init(void)
{
    if (bt_initialized) {
        bt_ref_count++;
        ESP_LOGD(TAG, "BT already initialized, ref count: %d", bt_ref_count);
        return ESP_OK;
    }

    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        esp_bt_controller_deinit();
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        return ret;
    }

    bt_initialized = true;
    bt_sniffer_mode = false;
    bt_ref_count = 1;
    ESP_LOGI(TAG, "BLE initialized successfully");
    return ESP_OK;
}

esp_err_t bt_manager_init_sniffer(const esp_vhci_host_callback_t *callback)
{
    if (bt_initialized) {
        ESP_LOGE(TAG, "BT already initialized in mode %s", bt_sniffer_mode ? "sniffer" : "standard");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        esp_bt_controller_deinit();
        return ret;
    }

    /* Register VHCI callbacks for sniffer */
    if (callback) {
        esp_vhci_host_register_callback(callback);
    }

    bt_initialized = true;
    bt_sniffer_mode = true;
    bt_ref_count = 1;
    ESP_LOGI(TAG, "BLE sniffer initialized");
    return ESP_OK;
}

esp_err_t bt_manager_deinit(void)
{
    if (!bt_initialized) {
        ESP_LOGW(TAG, "BT not initialized, skip deinit");
        return ESP_ERR_INVALID_STATE;
    }

    bt_ref_count--;
    if (bt_ref_count > 0) {
        ESP_LOGD(TAG, "Ref count: %d, keep BT alive", bt_ref_count);
        return ESP_OK;
    }

    esp_err_t ret;

    if (!bt_sniffer_mode) {
        ret = esp_bluedroid_disable();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Bluedroid disable failed: %s", esp_err_to_name(ret));
        }

        ret = esp_bluedroid_deinit();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Bluedroid deinit failed: %s", esp_err_to_name(ret));
        }
    }

    ret = esp_bt_controller_disable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller disable failed: %s", esp_err_to_name(ret));
    }

    ret = esp_bt_controller_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller deinit failed: %s", esp_err_to_name(ret));
    }

    bt_initialized = false;
    bt_ref_count = 0;
    ESP_LOGI(TAG, "BLE deinitialized");
    return ESP_OK;
}

int bt_manager_get_ref(void)
{
    return bt_ref_count;
}

bool bt_manager_is_ble_ready(void)
{
    return bt_initialized;
}
