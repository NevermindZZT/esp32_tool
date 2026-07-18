/**
 * @file ble_sniffer_vs.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - Vendor-Specific HCI commands
 * @version 1.0.0
 * @date 2026-07-18
 *
 * Provides access to ESP32 BLE Controller internal test features
 * via Vendor-Specific HCI commands (OCF=0x0113).
 *
 * These require esp_ble_internalTestFeaturesEnable(true) to be called first.
 */
#ifndef __BLE_SNIFFER_VS_H__
#define __BLE_SNIFFER_VS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VS OCF for internal test features */
#define VS_OCF_CFG_TEST_RELATED      0x0113

/* Sub-commands under VS_OCF_CFG_TEST_RELATED */
#define VS_SUBCMD_ENABLE             0x00
#define VS_SUBCMD_ENABLE_ADV_DELAY   0x01
#define VS_SUBCMD_SET_SCAN_FOREVER   0x04
#define VS_SUBCMD_SET_EXPECTED_PEER  0x05
#define VS_SUBCMD_GET_ADV_TXED_CNT   0x06
#define VS_SUBCMD_GET_SCAN_RXED_CNT  0x07
#define VS_SUBCMD_SET_TXPWR_LVL      0x08
#define VS_SUBCMD_SET_SCAN_AA        0x14
#define VS_SUBCMD_SET_ADV_AA         0x15
#define VS_SUBCMD_SET_SCAN_CHAN      0x16
#define VS_SUBCMD_GET_CTRL_STATUS    0x1A

/* Default AA for broadcast channels */
#define VS_AA_BROADCAST              0x8E89BED6

/* BLE advertising channel frequencies */
#define VS_CHAN_37                   37
#define VS_CHAN_38                   38
#define VS_CHAN_39                   39

/**
 * @brief Enable internal test features.
 * Must be called before any other VS commands.
 * Calls esp_ble_internalTestFeaturesEnable(true) which is
 * linked from libbtdm_app.a (pre-compiled controller library).
 */
void vs_enable_test_features(void);

/**
 * @brief Disable internal test features.
 */
void vs_disable_test_features(void);

/**
 * @brief Send a VS config test command.
 *
 * @param subcmd  Sub-command code (e.g. VS_SUBCMD_SET_SCAN_AA)
 * @param params  Sub-command parameters
 * @param param_len Length of parameters in bytes
 */
void vs_send_cfg_test(uint8_t subcmd, const uint8_t *params, uint8_t param_len);

/**
 * @brief Set scanner Access Address.
 *
 * Set to VS_AA_BROADCAST (0x8E89BED6) to capture all broadcast channel PDUs
 * including CONNECT_REQ. Set to a connection's AA to follow data channel.
 *
 * @param aa 32-bit Access Address
 */
void vs_set_scan_aa(uint32_t aa);

/**
 * @brief Set scanner to specific RF channel.
 *
 * @param channel RF channel (0-39, or 37/38/39 for advertising ch)
 */
void vs_set_scan_channel(uint8_t channel);

/**
 * @brief Set expected peer device address.
 *
 * @param addr 6-byte BLE device address
 * @param addr_type Address type (0=public, 1=random)
 */
void vs_set_expected_peer(const uint8_t *addr, uint8_t addr_type);

/**
 * @brief Set scanner to scan forever (disable scan timeout).
 */
void vs_set_scan_forever(void);

/**
 * @brief Get the number of received scan packets (from test mode counter).
 * @return Number of packets received since test mode was enabled
 */
uint32_t vs_get_scan_rxed_cnt(void);

/**
 * @brief Get the number of transmitted advertising packets (from test mode counter).
 * @return Number of packets transmitted
 */
uint32_t vs_get_adv_txed_cnt(void);

/**
 * @brief Check if test features are enabled.
 */
bool vs_is_test_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_SNIFFER_VS_H__ */