/**
 * @file bt_manager.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BT controller lifecycle manager with reference counting
 * @version 1.0.0
 * @date 2026-07-18
 * @copyright (c) 2026 Letter All rights reserved.
 */
#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__

#include "esp_err.h"
#include "esp_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE controller and Bluedroid with reference counting.
 * 
 * First call allocates BLE memory, initializes BT controller and Bluedroid.
 * Subsequent calls only increment reference counter.
 * 
 * @return ESP_OK on success, otherwise error code
 */
esp_err_t bt_manager_init(void);

/**
 * @brief Initialize BLE controller ONLY (no Bluedroid), for sniffer mode.
 * 
 * Registers VHCI callbacks to capture raw HCI packets.
 * Does NOT initialize Bluedroid (sniffer doesn't need host stack).
 * 
 * @param callback VHCI host callback for receiving HCI packets
 * @return ESP_OK on success, otherwise error code
 */
esp_err_t bt_manager_init_sniffer(const esp_vhci_host_callback_t *callback);

/**
 * @brief Deinitialize BLE controller and Bluedroid with reference counting.
 * 
 * Decrements reference counter. When counter reaches zero, disables and
 * deinitializes Bluedroid and BT controller.
 * 
 * @return ESP_OK on success, otherwise error code
 */
esp_err_t bt_manager_deinit(void);

/**
 * @brief Get current reference count.
 * @return reference count
 */
int bt_manager_get_ref(void);

/**
 * @brief Check if BLE is ready (initialized and enabled).
 * @return true if BLE is ready
 */
bool bt_manager_is_ble_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_MANAGER_H__ */
