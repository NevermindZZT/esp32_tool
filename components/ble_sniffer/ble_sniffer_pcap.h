/**
 * @file ble_sniffer_pcap.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - pcapng format writer
 * @version 1.0.0
 * @date 2026-07-18
 */
#ifndef __BLE_SNIFFER_PCAP_H__
#define __BLE_SNIFFER_PCAP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pcapng writer and generate file header.
 * @param writer_cb Function to output pcapng data bytes
 */
void ble_sniffer_pcap_init(void (*writer_cb)(const uint8_t *data, uint32_t len));

/**
 * @brief Write a BLE HCI packet as a pcapng Enhanced Packet Block.
 * @param h4_type H4 packet type (0x01=cmd, 0x02=ACL, 0x04=event)
 * @param data Payload data (without H4 header)
 * @param len Payload length
 */
void ble_sniffer_pcap_write_packet(uint8_t h4_type, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_SNIFFER_PCAP_H__ */
