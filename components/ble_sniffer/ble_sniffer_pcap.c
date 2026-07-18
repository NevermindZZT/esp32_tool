/**
 * @file ble_sniffer_pcap.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - pcapng format writer
 * @version 1.0.0
 * @date 2026-07-18
 *
 * pcapng format:
 *   [SHB - Section Header Block]
 *   [IDB - Interface Description Block]
 *   [EPB - Enhanced Packet Block] x N
 *
 * Link type: BLUETOOTH_HCI_H4 (187)
 *   Each packet begins with 1-byte H4 type indicator.
 */
#include "ble_sniffer_pcap.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "ble_sniffer_pcap";

static void (*output_cb)(const uint8_t *data, uint32_t len) = NULL;

/* --- pcapng block types --- */
#define PCAPNG_SHB          0x0A0D0D0A
#define PCAPNG_IDB          0x00000001
#define PCAPNG_EPB          0x00000006

/* --- Link Layer Types --- */
#define LINKTYPE_BLUETOOTH_HCI_H4   187

/* Write a uint32 in little-endian */
static inline void write_u32_le(uint8_t *buf, uint32_t val)
{
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

/* Write a uint64 in little-endian */
static inline void write_u64_le(uint8_t *buf, uint64_t val)
{
    for (int i = 0; i < 8; i++) {
        buf[i] = (val >> (i * 8)) & 0xFF;
    }
}

static void write_block(const void *data, uint32_t len)
{
    if (output_cb) {
        output_cb((const uint8_t *)data, len);
    }
}

void ble_sniffer_pcap_init(void (*writer_cb)(const uint8_t *data, uint32_t len))
{
    output_cb = writer_cb;

    /* Build SHB (Section Header Block) - 28 bytes */
    uint8_t shb[28];
    memset(shb, 0, sizeof(shb));
    write_u32_le(&shb[0], PCAPNG_SHB);
    write_u32_le(&shb[4], 28);              /* block length */
    write_u32_le(&shb[8], 0x1A2B3C4D);      /* byte order magic */
    write_u32_le(&shb[12], 1);               /* major version */
    write_u32_le(&shb[16], 0);               /* minor version */
    /* section length = -1 (unknown) */
    memset(&shb[20], 0xFF, 8);
    write_u32_le(&shb[24], 28);              /* block length (repeat) */
    write_block(shb, sizeof(shb));

    /* Build IDB (Interface Description Block) - 20 bytes */
    uint8_t idb[20];
    memset(idb, 0, sizeof(idb));
    write_u32_le(&idb[0], PCAPNG_IDB);
    write_u32_le(&idb[4], 20);              /* block length */
    write_u32_le(&idb[8], LINKTYPE_BLUETOOTH_HCI_H4); /* link type */
    write_u32_le(&idb[12], 0);               /* reserved */
    write_u32_le(&idb[14], 0);              /* snap length (0 = unlimited) */
    write_u32_le(&idb[16], 20);             /* block length (repeat) */
    write_block(idb, sizeof(idb));

    ESP_LOGI(TAG, "pcapng header written (SHB+IDB)");
}

void ble_sniffer_pcap_write_packet(uint8_t h4_type, const uint8_t *data, uint32_t len)
{
    /* Total packet data = 1 byte H4 header + payload */
    uint32_t total_len = 1 + len;

    /* EPB header: block_type(4) + block_len(4) + iface_id(4) + timestamp(8)
     *            + captured_len(4) + packet_len(4) = 28 bytes
     * EPB padding to 4-byte boundary */
    uint32_t padded = (total_len + 3) & ~3;
    uint32_t epb_len = 28 + padded + 4;  /* header + data + epb_length2 */

    /* Allocate on stack (max BLE packet ~255 bytes, safe) */
    uint8_t buf[epb_len];
    memset(buf, 0, sizeof(buf));

    /* EPB header */
    write_u32_le(&buf[0], PCAPNG_EPB);
    write_u32_le(&buf[4], epb_len);
    write_u32_le(&buf[8], 0);               /* interface ID = 0 */
    write_u64_le(&buf[12], 0);              /* timestamp (0 = use Wireshark time) */
    write_u32_le(&buf[20], total_len);       /* captured length */
    write_u32_le(&buf[24], total_len);       /* original length */

    /* HCI data: H4 type + payload */
    buf[28] = h4_type;
    if (len > 0 && data) {
        memcpy(&buf[29], data, len);
    }

    /* Trailing block length */
    write_u32_le(&buf[28 + padded], epb_len);

    write_block(buf, sizeof(buf));
}
