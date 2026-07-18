/**
 * @file ble_sniffer_ll.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - Link Layer PDU parsing + connection following
 * @version 1.0.0
 * @date 2026-07-18
 *
 * CSA #1 hop calculation based on the Bluetooth Core Specification v5.x.
 */
#include "ble_sniffer_ll.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "ble_sniffer_ll";

/* ===================== CONNECT_REQ Parsing ===================== */

bool ll_parse_connect_req(const uint8_t *pdu, uint8_t pdu_len,
                          conn_info_t *info, uint64_t rx_time_us)
{
    if (!pdu || !info || pdu_len < 34) {
        ESP_LOGE(TAG, "Invalid CONNECT_REQ PDU (len=%d, need >= 34)", pdu_len);
        return false;
    }

    memset(info, 0, sizeof(conn_info_t));

    /* CONNECT_REQ payload format (34 bytes):
     *   InitA:    6 bytes  (offset 0)
     *   AdvA:     6 bytes  (offset 6)
     *   AA:       4 bytes  (offset 12)  - Access Address
     *   CRCInit:  3 bytes  (offset 16)
     *   WinSize:  1 byte   (offset 19)
     *   WinOffset:2 bytes  (offset 20, little-endian)
     *   Interval: 2 bytes  (offset 22, little-endian)
     *   Latency:  2 bytes  (offset 24, little-endian)
     *   Timeout:  2 bytes  (offset 26, little-endian)
     *   ChM:      5 bytes  (offset 28)  - 37 bits channel map
     *   Hop:      5 bits   (offset 33, bits 0-4)
     *   SCA:      3 bits   (offset 33, bits 5-7)
     */

    /* InitA (Initiator address) */
    memcpy(info->initiator, &pdu[0], 6);

    /* AdvA (Advertiser address) */
    memcpy(info->advertiser, &pdu[6], 6);

    /* Access Address */
    info->access_addr = (uint32_t)pdu[12] |
                        ((uint32_t)pdu[13] << 8) |
                        ((uint32_t)pdu[14] << 16) |
                        ((uint32_t)pdu[15] << 24);

    /* CRCInit */
    memcpy(info->crc_init, &pdu[16], 3);

    /* WinSize */
    info->win_size = pdu[19];

    /* WinOffset */
    info->win_offset = (uint16_t)pdu[20] | ((uint16_t)pdu[21] << 8);

    /* Interval (1.25ms units) */
    info->interval = (uint16_t)pdu[22] | ((uint16_t)pdu[23] << 8);

    /* Latency (number of connection events) */
    info->latency = (uint16_t)pdu[24] | ((uint16_t)pdu[25] << 8);

    /* Timeout (10ms units) */
    info->timeout = (uint16_t)pdu[26] | ((uint16_t)pdu[27] << 8);

    /* Channel Map (5 bytes = 40 bits, but only 37 are used) */
    memcpy(info->chm, &pdu[28], 5);

    /* Last byte: bits 0-4 = Hop increment, bits 5-7 = SCA */
    info->hop = pdu[33] & 0x1F;
    info->sca = (pdu[33] >> 5) & 0x07;

    /* Timestamp */
    info->conn_timestamp_us = (uint32_t)rx_time_us;

    ESP_LOGI(TAG, "CONNECT_REQ: AA=0x%08lX, Interval=%d, Latency=%d, Timeout=%d, Hop=%d, SCA=%d",
             (unsigned long)info->access_addr, info->interval, info->latency,
             info->timeout, info->hop, info->sca);
    ESP_LOGI(TAG, "  Initiator: %02X:%02X:%02X:%02X:%02X:%02X",
             info->initiator[5], info->initiator[4], info->initiator[3],
             info->initiator[2], info->initiator[1], info->initiator[0]);
    ESP_LOGI(TAG, "  Advertiser: %02X:%02X:%02X:%02X:%02X:%02X",
             info->advertiser[5], info->advertiser[4], info->advertiser[3],
             info->advertiser[2], info->advertiser[1], info->advertiser[0]);

    return true;
}

/* ===================== Channel Utilities ===================== */

int ll_count_used_channels(const uint8_t *chm)
{
    int count = 0;
    for (int i = 0; i < 37; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        if (chm[byte_idx] & (1 << bit_idx)) {
            count++;
        }
    }
    return count;
}

bool ll_is_channel_used(const uint8_t *chm, uint8_t channel)
{
    if (channel > 36) return false;  /* advertising channels 37-39 are not in channel map */
    int byte_idx = channel / 8;
    int bit_idx = channel % 8;
    return (chm[byte_idx] & (1 << bit_idx)) != 0;
}

/**
 * @brief Build remapping table from channel map.
 * @return Number of used channels
 */
static int build_remap_table(const uint8_t *chm, uint8_t *remap_table, int table_size)
{
    int count = 0;
    for (int i = 0; i < 37 && count < table_size; i++) {
        if (ll_is_channel_used(chm, i)) {
            remap_table[count++] = i;
        }
    }
    return count;
}

/* ===================== CSA #1 Hop Calculation ===================== */

int ll_calc_channel_csa1(uint16_t event_cnt, const conn_info_t *info)
{
    /* The first data channel PDU uses event_cnt = 0 (the connection event following CONNECT_REQ).
     * CSA #1: unmapped_ch = (last_unmapped_ch + hop) % 37
     *         phys_ch = remap_table[unmapped_ch % num_used]
     *
     * The initial unmapped channel is 0.
     */

    uint8_t remap_table[37];
    int num_used = build_remap_table(info->chm, remap_table, 37);

    if (num_used == 0) {
        ESP_LOGW(TAG, "No channels in channel map, using ch 0");
        return 0;
    }

    /* Calculate unmapped channel number */
    uint16_t unmapped_ch = ((uint16_t)event_cnt * info->hop) % 37;

    /* Map to physical channel */
    int phys_ch = remap_table[unmapped_ch % num_used];

    ESP_LOGV(TAG, "CSA1: event=%d, unmapped=%d, num_used=%d → phys_ch=%d",
             event_cnt, unmapped_ch, num_used, phys_ch);

    return phys_ch;
}

uint64_t ll_calc_event_time_us(uint16_t event_cnt, const conn_info_t *info)
{
    /* Connection interval in microseconds */
    uint64_t interval_us = (uint64_t)info->interval * 1250;  /* 1.25ms units */

    /* First data channel PDU is due at conn_timestamp_us + 1.25ms + winOffset */
    uint64_t base_us = info->conn_timestamp_us +
                       (uint64_t)1250 +                     /* T_IFS + 1.25ms */
                       (uint64_t)info->win_offset * 1250;  /* window offset */

    return base_us + (uint64_t)event_cnt * interval_us;
}
