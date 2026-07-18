/**
 * @file ble_sniffer_vs.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - Vendor-Specific HCI commands implementation
 * @version 1.0.0
 * @date 2026-07-18
 *
 * Manages ESP32 BLE controller internal test mode for full-packet sniffing.
 * Uses esp_ble_internalTestFeaturesEnable() + VS HCI commands.
 */
#include "ble_sniffer_vs.h"
#include "ble_sniffer_hci.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "ble_sniffer_vs";

static bool test_mode_enabled = false;

/* esp_ble_internalTestFeaturesEnable is in the pre-compiled controller library.
 * It's not declared in public headers, so we declare it here. */
extern void esp_ble_internalTestFeaturesEnable(bool enable);

/* ===================== Initialization ===================== */

void vs_enable_test_features(void)
{
    if (test_mode_enabled) {
        ESP_LOGW(TAG, "Test features already enabled");
        return;
    }

    /* This enables the VS test command set (OCF=0x0113) in the controller */
    esp_ble_internalTestFeaturesEnable(true);
    test_mode_enabled = true;
    ESP_LOGI(TAG, "Internal test features enabled");
}

void vs_disable_test_features(void)
{
    if (!test_mode_enabled) return;

    /* Disable test features by telling controller to exit test mode */
    uint8_t params[1] = {0x00};  /* subcmd ENABLE with value=0 = disable */
    vs_send_cfg_test(VS_SUBCMD_ENABLE, params, 1);

    esp_ble_internalTestFeaturesEnable(false);
    test_mode_enabled = false;
    ESP_LOGI(TAG, "Internal test features disabled");
}

/* ===================== VS Command Sending ===================== */

void vs_send_cfg_test(uint8_t subcmd, const uint8_t *params, uint8_t param_len)
{
    if (!test_mode_enabled) {
        ESP_LOGW(TAG, "Test features not enabled, call vs_enable_test_features() first");
        return;
    }

    /* Build VS command:
     *   H4(1) + OpCode(2) + Length(1) + SubCmd(1) + SubCmdParams(N)
     *
     * OpCode = OGF_VENDOR (0x3F) << 10 | VS_OCF_CFG_TEST_RELATED (0x0113)
     *        = 0xFC00 | 0x0113 = 0xFD13
     */
    uint8_t buf[4 + 1 + 255];  /* max params = 255 */
    uint16_t total_len = param_len + 1;  /* +1 for subcmd */

    buf[0] = 0x01;  /* H4: Command */
    buf[1] = 0x13;  /* OpCode low byte */
    buf[2] = 0xFD;  /* OpCode high byte */
    buf[3] = total_len;
    buf[4] = subcmd;

    if (params && param_len > 0) {
        memcpy(&buf[5], params, param_len);
    }

    hci_send_cmd(buf, 5 + param_len);
}

/* ===================== Convenience Functions ===================== */

void vs_set_scan_aa(uint32_t aa)
{
    uint8_t params[4];
    params[0] = aa & 0xFF;
    params[1] = (aa >> 8) & 0xFF;
    params[2] = (aa >> 16) & 0xFF;
    params[3] = (aa >> 24) & 0xFF;

    vs_send_cfg_test(VS_SUBCMD_SET_SCAN_AA, params, 4);
    ESP_LOGI(TAG, "Set scan AA to 0x%08lX", (unsigned long)aa);
}

void vs_set_scan_forever(void)
{
    uint8_t params[1] = {0x01};  /* 1 = enable */
    vs_send_cfg_test(VS_SUBCMD_SET_SCAN_FOREVER, params, 1);
    ESP_LOGI(TAG, "Scan forever enabled (scan timeout disabled)");
}

uint32_t vs_get_scan_rxed_cnt(void)
{
    /* Send GET_SCAN_RXED_CNT - the controller responds with a Command Complete event
     * containing the counter value. We can't easily extract it from HCI events,
     * so this sends the command and returns 0. The response appears in the log. */
    vs_send_cfg_test(VS_SUBCMD_GET_SCAN_RXED_CNT, NULL, 0);
    return 0;
}

uint32_t vs_get_adv_txed_cnt(void)
{
    vs_send_cfg_test(VS_SUBCMD_GET_ADV_TXED_CNT, NULL, 0);
    return 0;
}

void vs_set_scan_channel(uint8_t channel)
{
    uint8_t params[1] = {channel};
    vs_send_cfg_test(VS_SUBCMD_SET_SCAN_CHAN, params, 1);
    ESP_LOGI(TAG, "Set scan channel to %d", channel);
}

void vs_set_expected_peer(const uint8_t *addr, uint8_t addr_type)
{
    uint8_t params[7];
    params[0] = addr_type;
    memcpy(&params[1], addr, 6);

    vs_send_cfg_test(VS_SUBCMD_SET_EXPECTED_PEER, params, 7);
    ESP_LOGI(TAG, "Set expected peer");
}

bool vs_is_test_mode(void)
{
    return test_mode_enabled;
}
