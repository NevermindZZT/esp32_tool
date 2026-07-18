/**
 * @file ble_sniffer.c
 * @brief BLE Sniffer RTAM App - capture BLE advertising via GAP scan
 * @version 5.1.0
 * @date 2026-07-18
 *
 * Architecture:
 *   start() -> bt_manager_init() + pcap header
 *   resume() -> create UI, register gesture
 *   GAP scan -> pcapng -> CDC -> Wireshark
 *            -> btsnoop -> FAT file
 *            -> ring buffer -> shell "ble_sniff list"
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
#include "ble_sniffer_btsnoop.h"
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
static volatile bool scanning = false;

static lv_obj_t *screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *count_label = NULL;
static lv_obj_t *btn_toggle = NULL;
static lv_timer_t *sniffer_timer = NULL;
static uint32_t display_count = 0;
static volatile uint32_t packet_count = 0;

/* btsnoop recording */
static bool btsnoop_recording = false;
static FILE *btsnoop_file = NULL;

#define FAT_BASE "/spiflash"

static void btsnoop_file_writer(const uint8_t *data, uint32_t len)
{
    if (btsnoop_file) {
        fwrite(data, 1, len, btsnoop_file);
    }
}

/* Ring buffer */
#define PKT_RING_SIZE   16
#define PKT_DATA_MAX    64

typedef struct {
    uint8_t bda[6];
    int8_t rssi;
    char name[33];
    uint16_t data_len;
    uint8_t data[PKT_DATA_MAX];
} pkt_record_t;

static pkt_record_t pkt_ring[PKT_RING_SIZE];
static volatile int pkt_ring_head = 0;
static volatile int pkt_ring_count = 0;

/* ---- pcapng output via USB CDC ---- */

static void pcap_output_cb(const uint8_t *data, uint32_t len)
{
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, data, len);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}

/* ---- Extract name from adv data ---- */

static void extract_name(const uint8_t *adv, uint8_t len, char *name, uint8_t name_max)
{
    name[0] = 0;
    uint8_t idx = 0;
    while (idx < len) {
        uint8_t field_len = adv[idx];
        if (field_len == 0) break;
        uint8_t field_type = adv[idx + 1];
        if (field_type == 0x08 || field_type == 0x09) {
            uint8_t nlen = field_len - 1;
            if (nlen > name_max - 1) nlen = name_max - 1;
            memcpy(name, &adv[idx + 2], nlen);
            name[nlen] = 0;
            return;
        }
        idx += field_len + 1;
    }
}

/* ---- Build HCI event + output ---- */

static void send_advertising_report(esp_ble_gap_cb_param_t *param)
{
    const uint8_t adv_len = (param->scan_rst.adv_data_len > 31) ? 31 : param->scan_rst.adv_data_len;
    const uint8_t evt_len = 1 + 1 + 1 + 1 + 6 + 1 + adv_len + 1;
    const uint16_t pkt_len = 2 + 1 + evt_len;

    uint8_t pkt[pkt_len];
    uint16_t pos = 0;
    pkt[pos++] = 0x3E;
    pkt[pos++] = 0x00;
    pkt[pos++] = evt_len;
    pkt[pos++] = 0x02;
    pkt[pos++] = 1;
    pkt[pos++] = (param->scan_rst.scan_rsp_len > 0) ? 0x04 : (uint8_t)param->scan_rst.ble_evt_type;
    pkt[pos++] = param->scan_rst.ble_addr_type;
    memcpy(&pkt[pos], param->scan_rst.bda, 6);
    pos += 6;
    pkt[pos++] = adv_len;
    if (adv_len > 0) { memcpy(&pkt[pos], param->scan_rst.ble_adv, adv_len); pos += adv_len; }
    pkt[pos++] = (uint8_t)(param->scan_rst.rssi & 0xFF);
    pkt[1] = pos - 2;

    ble_sniffer_pcap_write_packet(0x04, pkt, pos);
    if (btsnoop_recording) ble_sniffer_btsnoop_write_packet(0x04, pkt, pos);

    /* Ring buffer */
    int idx = pkt_ring_head;
    pkt_record_t *rec = &pkt_ring[idx];
    memcpy(rec->bda, param->scan_rst.bda, 6);
    rec->rssi = param->scan_rst.rssi;
    extract_name(param->scan_rst.ble_adv, param->scan_rst.adv_data_len, rec->name, sizeof(rec->name));
    rec->data_len = (pos > PKT_DATA_MAX) ? PKT_DATA_MAX : pos;
    memcpy(rec->data, pkt, rec->data_len);
    pkt_ring_head = (idx + 1) % PKT_RING_SIZE;
    if (pkt_ring_count < PKT_RING_SIZE) pkt_ring_count++;
    packet_count++;
}

/* ---- GAP callback ---- */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
            send_advertising_report(param);
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

/* ---- Shell commands ---- */

static void ble_sniffer_cmd_scan(void)
{
    if (!bt_ready) { printf("BT not ready\n"); return; }
    if (scanning) { printf("Already scanning\n"); return; }
    esp_ble_gap_start_scanning(0);
    printf("Scan started\n");
}

static void ble_sniffer_cmd_stop_scan(void)
{
    if (scanning) { esp_ble_gap_stop_scanning(); printf("Scan stopped\n"); }
}

static void ble_sniffer_cmd_status(void)
{
    printf("=== BLE Sniffer ===\n");
    printf("BT: %s\n", bt_ready ? "Ready" : "Not ready");
    printf("Scan: %s\n", scanning ? "Active" : "Stopped");
    printf("Packets: %lu\n", (unsigned long)packet_count);
}

static void ble_sniffer_cmd_list(int argc, void *argv)
{
    int n = (argc >= 1) ? atoi((const char *)argv) : pkt_ring_count;
    if (n > pkt_ring_count) n = pkt_ring_count;
    printf("Recent %d packets:\n", n);
    int start = (pkt_ring_head - n + PKT_RING_SIZE) % PKT_RING_SIZE;
    int c = 0;
    for (int i = 0; i < pkt_ring_count && c < n; i++) {
        int idx = (start + i) % PKT_RING_SIZE;
        pkt_record_t *rec = &pkt_ring[idx];
        printf("  [%d] %02x:%02x:%02x:%02x:%02x:%02x  RSSI:%d  %s\n",
               c, rec->bda[0], rec->bda[1], rec->bda[2],
               rec->bda[3], rec->bda[4], rec->bda[5],
               rec->rssi, rec->name[0] ? rec->name : "(no name)");
        c++;
    }
}

static void ble_sniffer_cmd_clear(void)
{
    pkt_ring_count = 0; pkt_ring_head = 0; packet_count = 0;
    printf("Cleared\n");
}

static void ble_sniffer_cmd_record(void)
{
    if (btsnoop_recording) { printf("Already recording\n"); return; }
    if (!bt_ready) { printf("BT not ready\n"); return; }
    char path[64];
    snprintf(path, sizeof(path), "%s/capture.btsnoop", FAT_BASE);
    btsnoop_file = fopen(path, "wb");
    if (!btsnoop_file) {
        printf("Failed to open %s (FAT mounted?)\n", path);
        return;
    }
    ble_sniffer_btsnoop_init(btsnoop_file_writer);
    btsnoop_recording = true;
    printf("Recording to %s\n", path);
}

static void ble_sniffer_cmd_stoprec(void)
{
    if (!btsnoop_recording) { printf("Not recording\n"); return; }
    btsnoop_recording = false;
    if (btsnoop_file) { fclose(btsnoop_file); btsnoop_file = NULL; }
    printf("Recording stopped\n");
}

static ShellCommand ble_sniffer_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, scan, ble_sniffer_cmd_scan,
        scan\r\nstart capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stop, ble_sniffer_cmd_stop_scan,
        stop\r\nstop capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, status, ble_sniffer_cmd_status,
        status\r\nshow status),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, list, ble_sniffer_cmd_list,
        list\r\nlist recent packets),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, clear, ble_sniffer_cmd_clear,
        clear\r\nclear counters),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, record, ble_sniffer_cmd_record,
        record\r\nrecord btsnoop to FAT),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stoprec, ble_sniffer_cmd_stoprec,
        stoprec\r\nstop btsnoop recording),
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
    if (!screen || !count_label || !status_label) return;

    uint32_t cnt = packet_count;
    if (cnt != display_count) {
        display_count = cnt;
        lv_label_set_text_fmt(count_label, "Packets: %lu", (unsigned long)cnt);
    }
    lv_label_set_text(status_label, scanning ? "Sniffing..." : "Paused");
    if (btn_toggle) {
        lv_obj_t *l = lv_obj_get_child(btn_toggle, 0);
        if (l) lv_label_set_text(l, scanning ? "Stop" : "Scan");
    }
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
    esp_ble_gap_start_scanning(0);
    scanning = true;
    bt_ready = true;
    ESP_LOGI(TAG, "Sniffer started");
    return RTAM_OK;
}

static RtAppErr ble_sniffer_deinit_app(void)
{
    if (scanning) { esp_ble_gap_stop_scanning(); scanning = false; }
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
    if (scanning) { esp_ble_gap_stop_scanning(); scanning = false; }
    if (sniffer_timer) { lv_timer_del(sniffer_timer); sniffer_timer = NULL; }
    gui_remove_global_gesture_callback(ble_sniffer_gesture_callback);
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    status_label = NULL;
    count_label = NULL;
    btn_toggle = NULL;
    display_count = 0;
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .suspend = ble_sniffer_suspend,
    .resume  = ble_sniffer_resume,
    .start   = ble_sniffer_init_app,
    .stop    = ble_sniffer_deinit_app,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]) { "gui", "launcher", NULL },
    .conflicted = (const char *[]) { "ble_remote", NULL },
};

extern const lv_image_dsc_t icon_app_ble_sniffer;
static const RtamInfo ble_sniffer_info = { .label = "BLE Sniff", .icon = (void *) GUI_APP_ICON(ble_sniffer) };

RTAPP_EXPORT(ble_sniff, &interface, RTAPP_FLAG_BACKGROUND, &dependencies, &ble_sniffer_info);
