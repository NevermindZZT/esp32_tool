/**
 * @file ble_sniffer_btsnoop.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - btsnoop format writer
 * @version 1.0.0
 * @date 2026-07-18
 *
 * btsnoop format:
 *   File header (16 bytes)
 *   Packet record (24 bytes header + data) x N
 *
 * Data link type: 0x03F2 (1002) = HCI H4
 */
#include "ble_sniffer_btsnoop.h"
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ble_sniffer_btsnoop";

static void (*output_cb)(const uint8_t *data, uint32_t len) = NULL;

/* btsnoop file header: 16 bytes */
static const uint8_t btsnoop_header[16] = {
    'b', 't', 's', 'n', 'o', 'o', 'p', 0x00,  /* identification */
    0x00, 0x00, 0x00, 0x01,                      /* version = 1 (big-endian) */
    0x00, 0x00, 0x03, 0xEA,                      /* data link = 1002 (BE) */
};

static inline void write_u32_be(uint8_t *buf, uint32_t val)
{
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

static inline void write_u64_be(uint8_t *buf, uint64_t val)
{
    for (int i = 7; i >= 0; i--) {
        buf[i] = val & 0xFF;
        val >>= 8;
    }
}

void ble_sniffer_btsnoop_init(void (*writer_cb)(const uint8_t *data, uint32_t len))
{
    output_cb = writer_cb;
    if (output_cb) {
        output_cb(btsnoop_header, sizeof(btsnoop_header));
    }
    ESP_LOGI(TAG, "btsnoop header written");
}

void ble_sniffer_btsnoop_write_packet(uint8_t h4_type, const uint8_t *data, uint32_t len)
{
    if (!output_cb) return;

    uint32_t total_len = 1 + len;  /* H4 type + payload */
    uint8_t rec_hdr[24];

    /* Original Length */
    write_u32_be(&rec_hdr[0], total_len);
    /* Included Length */
    write_u32_be(&rec_hdr[4], total_len);
    /* Packet Flags: 1 = host->controller, 2 = controller->host */
    write_u32_be(&rec_hdr[8], 0x02);
    /* Cumulative Drops */
    write_u32_be(&rec_hdr[12], 0);
    /* Timestamp: microseconds since 01/01/1970 (Unix epoch) */
    uint64_t us = esp_timer_get_time();
    write_u64_be(&rec_hdr[16], us);

    output_cb(rec_hdr, sizeof(rec_hdr));

    /* Packet data: H4 type + payload */
    uint8_t h4 = h4_type;
    output_cb(&h4, 1);
    if (len > 0) {
        output_cb(data, len);
    }
}
