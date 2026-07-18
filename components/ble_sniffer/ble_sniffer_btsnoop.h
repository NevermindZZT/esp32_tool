/**
 * @file ble_sniffer_btsnoop.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - btsnoop format writer
 * @version 1.0.0
 * @date 2026-07-18
 *
 * btsnoop is the standard Bluetooth packet capture format used by
 * Apple, Frontline, Ellisys, and other analysis tools.
 *
 * Format: https://www.fte.com/webhelp/lea/Content/Glossary/btsnoop.htm
 */
#ifndef __BLE_SNIFFER_BTSNOOP_H__
#define __BLE_SNIFFER_BTSNOOP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize btsnoop writer and write file header.
 * @param writer_cb Function to output btsnoop data bytes (e.g. to file)
 */
void ble_sniffer_btsnoop_init(void (*writer_cb)(const uint8_t *data, uint32_t len));

/**
 * @brief Write a BLE HCI packet as a btsnoop record.
 * @param h4_type H4 packet type (0x01=cmd, 0x02=ACL, 0x04=event)
 * @param data Payload data (without H4 header)
 * @param len Payload length
 */
void ble_sniffer_btsnoop_write_packet(uint8_t h4_type, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_SNIFFER_BTSNOOP_H__ */
