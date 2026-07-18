/**
 * @file ble_sniffer.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer RTAM App - capture BLE advertising via GAP scan
 * @version 4.0.0
 * @date 2026-07-18
 * @copyright (c) 2026 Letter All rights reserved.
 *
 * Architecture:
 *   start() -> bt_manager_init() + pcap header (safe RTAM context)
 *   resume() -> create UI
 *
 *   GAP scan callback receives scan results -> builds synthetic HCI
 *   LE Advertising Report events -> pcapng -> USB CDC -> Wireshark
 *
 * NOTE: VHCI conflicts with Bluedroid, so we use GAP API directly
 *       and construct HCI-level events for Wireshark compatibility.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "bt_manager.h"
#include "ble_sniffer_pcap.h"
#include "gui.h"
#include "launcher.h"
#include "rtam.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style_gen.h"
#include "misc/lv_types.h"
#include "widgets/button/lv_button.h"
#include "widgets/label/lv_label.h"

static const char *TAG = "ble_sniffer";

static bool bt_ready = false;
static bool scanning = false;

static lv_obj_t *screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *count_label = NULL;
static lv_obj_t *btn_toggle = NULL;
static lv_timer_t *sniffer_timer = NULL;
static uint32_t display_count = 0;
static uint32_t packet_count = 0;

/* ---- pcapng data output via USB CDC ---- */

static void pcap_output_cb(const uint8_t *data, uint32_t len)
{
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, data, len);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}

/* ---- Send an advertising report as pcapng HCI event ---- */

static void send_advertising_report(esp_ble_gap_cb_param_t *param)
{
    uint8_t adv_len = param->scan_rst.adv_data_len;
    if (adv_len > 31) adv_len = 31;

    /* Build HCI LE Advertising Report event
     * Event Code (2) = 0x3E, Subevent (1) = 0x02
     */
    uint8_t evt_body_len = 1 + 1 + 1 + 1 + 6 + 1 + adv_len + 1;
    uint8_t hci_pkt_len = 2 + 1 + evt_body_len;

    uint8_t pkt[hci_pkt_len];
    int pos = 0;

    /* Event Code: HCI_LE_Meta */
    pkt[pos++] = 0x3E;
    pkt[pos++] = 0x00; /* placeholder length */
    /* Event Length */
    pkt[pos++] = evt_body_len;
    /* Subevent: LE_Advertising_Report */
    pkt[pos++] = 0x02;
    /* Num Reports */
    pkt[pos++] = 1;
    /* Event Type */
    if (param->scan_rst.scan_rsp_len > 0) {
        pkt[pos++] = 0x04; /* SCAN_RSP */
    } else {
        pkt[pos++] = param->scan_rst.ble_evt_type;
    }
    /* Address Type */
    pkt[pos++] = param->scan_rst.ble_addr_type;
    /* Address */
    memcpy(&pkt[pos], param->scan_rst.bda, 6);
    pos += 6;
    /* Data Length */
    pkt[pos++] = adv_len;
    /* Adv Data */
    if (adv_len > 0) {
        memcpy(&pkt[pos], param->scan_rst.ble_adv, adv_len);
        pos += adv_len;
    }
    /* RSSI */
    pkt[pos++] = (uint8_t)(param->scan_rst.rssi & 0xFF);

    /* Fix event code length field (at pos 1, after the 2-byte opcode) */
    pkt[1] = pos - 2;

    /* Write as HCI Event (H4 type 0x04) */
    ble_sniffer_pcap_write_packet(0x04, pkt, pos);
    packet_count++;
}

/* ---- GAP callback ---- */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            send_advertising_report(param);
        }
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            scanning = true;
            ESP_LOGI(TAG, "Scan started");
        }
        break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        scanning = false;
        ESP_LOGI(TAG, "Scan stopped");
        break;
    default:
        break;
    }
}

/* ---- Shell command group ---- */

static void ble_sniffer_cmd_scan(void)
{
    if (!bt_ready) {
        printf("BT not ready\n");
        return;
    }
    if (scanning) {
        printf("Already scanning\n");
        return;
    }
    esp_ble_gap_start_scanning(0);
}

static void ble_sniffer_cmd_stop_scan(void)
{
    if (scanning) {
        esp_ble_gap_stop_scanning();
    }
}

static void ble_sniffer_cmd_status(void)
{
    printf("=== BLE Sniffer ===\n");
    printf("BT: %s\n", bt_ready ? "Ready" : "Not ready");
    printf("Scan: %s\n", scanning ? "Active" : "Stopped");
    printf("Packets: %lu\n", (unsigned long)packet_count);
}

static ShellCommand ble_sniffer_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, scan, ble_sniffer_cmd_scan,
        scan\r\nble_sniff scan - start capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stop, ble_sniffer_cmd_stop_scan,
        stop\r\nble_sniff stop - stop capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, status, ble_sniffer_cmd_status,
        status\r\nble_sniff status - show status),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
ble_sniff, ble_sniffer_group, ble_sniff);

/* ---- GUI ---- */

static int ble_sniffer_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == screen) {
            rtamTerminate("ble_sniff");
        } else {
            gui_back();
        }
        return 0;
    }
    return -1;
}

static void btn_toggle_cb(lv_event_t *e)
{
    (void)e;
    if (!bt_ready) return;
    if (scanning) {
        esp_ble_gap_stop_scanning();
    } else {
        esp_ble_gap_start_scanning(0);
    }
}

static void ble_sniffer_update_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!screen) return;

    gui_lock();
    if (packet_count != display_count) {
        display_count = packet_count;
        if (count_label) {
            lv_label_set_text_fmt(count_label, "Packets: %lu", (unsigned long)packet_count);
        }
    }
    if (status_label) {
        lv_label_set_text(status_label, scanning ? "Sniffing..." : "Paused");
    }
    if (btn_toggle) {
        lv_obj_t *l = lv_obj_get_child(btn_toggle, 0);
        if (l) lv_label_set_text(l, scanning ? "Stop" : "Scan");
    }
    gui_unlock();
}

static void ble_sniffer_init_screen(void)
{
    if (screen) return;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    count_label = lv_label_create(screen);
    lv_label_set_text_fmt(count_label, "Packets: 0");
    lv_obj_align(count_label, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN);

    btn_toggle = lv_button_create(screen);
    lv_obj_set_size(btn_toggle, 100, 40);
    lv_obj_align(btn_toggle, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn_toggle, btn_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(btn_toggle);
    lv_label_set_text(btn_label, "Scan");
    lv_obj_center(btn_label);

    sniffer_timer = lv_timer_create(ble_sniffer_update_cb, 500, NULL);
}

/* ---- RTAM Lifecycle ---- */

static RtAppErr ble_sniffer_init_app(void)
{
    esp_err_t ret = bt_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT init failed: %s", esp_err_to_name(ret));
        return RTAM_OK;
    }

    ble_sniffer_pcap_init(pcap_output_cb);
    esp_ble_gap_register_callback(gap_event_handler);

    /* Auto-start continuous scan */
    esp_ble_gap_start_scanning(0);
    scanning = true;

    bt_ready = true;
    ESP_LOGI(TAG, "Sniffer started");
    return RTAM_OK;
}

static RtAppErr ble_sniffer_deinit_app(void)
{
    if (scanning) {
        esp_ble_gap_stop_scanning();
        scanning = false;
    }
    bt_ready = false;
    bt_manager_deinit();
    return RTAM_OK;
}

static RtAppErr ble_sniffer_resume(void)
{
    ble_sniffer_init_screen();
    gui_push_screen(screen, LV_SCR_LOAD_ANIM_FADE_IN);
    gui_add_global_gesture_callback(ble_sniffer_gesture_callback);
    return RTAM_OK;
}

static RtAppErr ble_sniffer_suspend(void)
{
    if (scanning) {
        esp_ble_gap_stop_scanning();
        scanning = false;
    }
    if (sniffer_timer) {
        lv_timer_del(sniffer_timer);
        sniffer_timer = NULL;
    }
    gui_remove_global_gesture_callback(ble_sniffer_gesture_callback);
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    status_label = NULL;
    count_label = NULL;
    btn_toggle = NULL;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .suspend = ble_sniffer_suspend,
    .resume  = ble_sniffer_resume,
    .start   = ble_sniffer_init_app,
    .stop    = ble_sniffer_deinit_app,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]) {
        "gui",
        "launcher",
        NULL
    },
    .conflicted = (const char *[]) {
        "ble_remote",
        NULL
    },
};

extern const lv_image_dsc_t icon_app_ble_sniffer;
static const RtamInfo ble_sniffer_info = {
    .label = "BLE Sniff",
    .icon  = (void *) GUI_APP_ICON(ble_sniffer),
};

RTAPP_EXPORT(ble_sniff, &interface, RTAPP_FLAG_BACKGROUND, &dependencies, &ble_sniffer_info);
