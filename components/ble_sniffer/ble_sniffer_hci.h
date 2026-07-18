/**
 * @file ble_sniffer_hci.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - VHCI / HCI command layer
 * @version 1.0.0
 * @date 2026-07-18
 *
 * Provides:
 *   - VHCI callback installation
 *   - HCI command building helpers
 *   - HCI event queue + processing task
 *   - Callback registration for packet events
 */
#ifndef __BLE_SNIFFER_HCI_H__
#define __BLE_SNIFFER_HCI_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== HCI Constants ===================== */

/* H4 packet types */
#define H4_TYPE_CMD     0x01
#define H4_TYPE_ACL     0x02
#define H4_TYPE_SCO     0x03
#define H4_TYPE_EVT     0x04
#define H4_TYPE_ISO     0x05

/* HCI Event codes */
#define HCI_EVT_CMD_COMPLETE        0x0E
#define HCI_EVT_CMD_STATUS          0x0F
#define HCI_EVT_LE_META             0x3E
#define HCI_EVT_VENDOR              0xFF

/* LE Meta sub-events */
#define LE_SUB_ADV_REPORT           0x02
#define LE_SUB_EXT_ADV_REPORT       0x0D
#define LE_SUB_PERIODIC_ADV_REPORT  0x0E

/* OGF (Opcode Group Field) */
#define OGF_LINK_CTRL       0x01
#define OGF_HOST_CTRL       0x03
#define OGF_LE_CTRL         0x08
#define OGF_VENDOR          0x3F

/* OCF for Host Controller */
#define OCF_RESET           0x0003
#define OCF_SET_EVT_MASK    0x0001

/* OCF for LE Controller */
#define OCF_LE_SET_SCAN_PARAMS  0x000B
#define OCF_LE_SET_SCAN_ENABLE  0x000C

/* ===================== HCI Command Building ===================== */

/**
 * @brief Build HCI_Reset command.
 * @param buf Output buffer (at least 4 bytes)
 * @return Total command length (including H4 type)
 */
uint16_t hci_build_cmd_reset(uint8_t *buf);

/**
 * @brief Build HCI_Set_Event_Mask command.
 * @param buf Output buffer
 * @param mask 8-byte event mask
 * @return Total command length
 */
uint16_t hci_build_cmd_set_event_mask(uint8_t *buf, const uint8_t *mask);

/**
 * @brief Build LE_Set_Scan_Parameters command.
 * @param buf Output buffer
 * @param scan_type 0=passive, 1=active
 * @param scan_interval Scan interval (N * 0.625ms)
 * @param scan_window Scan window (N * 0.625ms)
 * @param own_addr_type 0=public, 1=random
 * @param filter_policy 0=accept all
 * @return Total command length
 */
uint16_t hci_build_le_set_scan_params(uint8_t *buf, uint8_t scan_type,
                                       uint16_t scan_interval, uint16_t scan_window,
                                       uint8_t own_addr_type, uint8_t filter_policy);

/**
 * @brief Build LE_Set_Scan_Enable command.
 * @param buf Output buffer
 * @param enable 0=disable, 1=enable
 * @param filter_dups 0=disable duplicate filtering
 * @return Total command length
 */
uint16_t hci_build_le_set_scan_enable(uint8_t *buf, uint8_t enable, uint8_t filter_dups);

/* ===================== VHCI Interface ===================== */

/**
 * @brief Packet received callback type.
 * Called from processing task context for each sniffed packet.
 * @param h4_type H4 packet type (0x02=ACL, 0x04=Event, etc.)
 * @param data    Payload (without H4 header)
 * @param len     Payload length
 */
typedef void (*hci_packet_cb_t)(uint8_t h4_type, const uint8_t *data, uint16_t len);

/**
 * @brief Initialize VHCI and start processing task.
 *
 * Registers VHCI callbacks, creates event queue and processing task.
 * Must be called after bt_manager_init_sniffer().
 *
 * @param packet_cb Callback for received HCI packets (can be NULL)
 */
void hci_init(hci_packet_cb_t packet_cb);

/**
 * @brief Send an HCI command via VHCI.
 * @param buf Command buffer (including H4 type prefix)
 * @param len Command length
 */
void hci_send_cmd(const uint8_t *buf, uint16_t len);

/**
 * @brief Start scanning (send LE_Set_Scan_Enable).
 */
void hci_start_scan(void);

/**
 * @brief Stop scanning.
 */
void hci_stop_scan(void);

/**
 * @brief Initialize scanning sequence.
 * Sends Reset → Set Event Mask → Set Scan Params → Enable Scan.
 */
void hci_init_scan(void);

/**
 * @brief Check if scanning is active.
 */
bool hci_is_scanning(void);

/**
 * @brief Get current packet count.
 */
uint32_t hci_get_packet_count(void);

/**
 * @brief Deinitialize VHCI task and queue.
 * Must be called before bt_manager_deinit().
 */
void hci_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_SNIFFER_HCI_H__ */