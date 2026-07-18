/**
 * @file ble_sniffer_ll.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - Link Layer PDU parsing + connection following
 * @version 1.0.0
 * @date 2026-07-18
 *
 * Implements:
 *   - CONNECT_REQ PDU parsing (extract connection parameters)
 *   - CSA #1 hop sequence calculation
 *   - Connection event timing prediction
 */
#ifndef __BLE_SNIFFER_LL_H__
#define __BLE_SNIFFER_LL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BLE LL PDU types on advertising channels */
#define LL_PDU_ADV_IND          0x00
#define LL_PDU_ADV_DIRECT_IND   0x01
#define LL_PDU_ADV_NONCONN_IND  0x02
#define LL_PDU_SCAN_REQ         0x03
#define LL_PDU_SCAN_RSP         0x04
#define LL_PDU_CONNECT_REQ      0x05
#define LL_PDU_ADV_SCAN_IND     0x06
#define LL_PDU_ADV_EXT_IND      0x07
#define LL_PDU_AUX_SCAN_REQ     0x0D
#define LL_PDU_AUX_CONNECT_REQ  0x0E

/**
 * @brief Connection information extracted from CONNECT_REQ.
 */
typedef struct {
    uint8_t  initiator[6];     /* Initiator address */
    uint8_t  advertiser[6];    /* Advertiser address */
    uint32_t access_addr;      /* Connection Access Address */
    uint8_t  crc_init[3];      /* CRC initialization value */
    uint8_t  win_size;         /* Transmission window size (1.25ms units) */
    uint16_t win_offset;       /* Transmission window offset (1.25ms units) */
    uint16_t interval;         /* Connection interval (1.25ms units) */
    uint16_t latency;          /* Slave latency (number of events) */
    uint16_t timeout;          /* Supervision timeout (10ms units) */
    uint8_t  chm[5];           /* Channel map (37 bits, LSB first) */
    uint8_t  hop;              /* Hop increment (5 bits) */
    uint8_t  sca;              /* Sleep clock accuracy */
    /* Derived */
    uint32_t conn_timestamp_us;/* Time when CONNECT_REQ was received (us) */
} conn_info_t;

/**
 * @brief Parse a CONNECT_REQ PDU payload.
 *
 * @param pdu       Pointer to CONNECT_REQ PDU data (after 2-byte header: PDU type + length)
 * @param pdu_len   Length of remaining PDU data
 * @param info      [out] Parsed connection information
 * @param rx_time_us Timestamp when the PDU was received (for timing prediction)
 * @return true on success
 */
bool ll_parse_connect_req(const uint8_t *pdu, uint8_t pdu_len,
                          conn_info_t *info, uint64_t rx_time_us);

/**
 * @brief Check if a PDU contains a CONNECT_REQ on advertising channels.
 *
 * @param pdu_type The PDU type field (lower 4 bits of first byte)
 * @return true if this is CONNECT_REQ
 */
static inline bool ll_is_connect_req(uint8_t pdu_type)
{
    return (pdu_type & 0x0F) == LL_PDU_CONNECT_REQ;
}

/**
 * @brief Calculate number of used channels from channel map.
 * @param chm 5-byte channel map (37 bits)
 * @return Number of used channels
 */
int ll_count_used_channels(const uint8_t *chm);

/**
 * @brief Calculate the physical RF channel for a connection event.
 *
 * Implements CSA #1 (Channel Selection Algorithm #1):
 *   unmapped_ch = (last_unmapped_ch + hop) % 37
 *   phys_ch = remap_table[unmapped_ch % num_used]
 *
 * @param event_cnt Number of connection events since CONNECT_REQ (0 = first data channel PDU)
 * @param info      Connection parameters (must have valid chm, hop)
 * @return RF channel number (0-36 for data channels)
 */
int ll_calc_channel_csa1(uint16_t event_cnt, const conn_info_t *info);

/**
 * @brief Calculate the expected timestamp (in microseconds) of a connection event.
 *
 * @param event_cnt Number of connection events since CONNECT_REQ
 * @param info      Connection parameters (must have valid interval, conn_timestamp_us)
 * @return Absolute timestamp in microseconds (relative to esp_timer_get_time() base)
 */
uint64_t ll_calc_event_time_us(uint16_t event_cnt, const conn_info_t *info);

/**
 * @brief Check if a specific RF channel is used in the channel map.
 * @param chm   5-byte channel map
 * @param channel RF channel (0-39)
 * @return true if channel is used
 */
bool ll_is_channel_used(const uint8_t *chm, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_SNIFFER_LL_H__ */