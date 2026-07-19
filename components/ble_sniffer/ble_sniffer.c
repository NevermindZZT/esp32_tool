/**
 * @file ble_sniffer.c
 * @brief BLE Sniffer RTAM App - VHCI / Controller-Only 全信道嗅探
 * @version 6.0.0
 * @date 2026-07-18
 *
 * 架构:
 *   start() -> bt_manager_init_sniffer() + hci_init() + hci_init_scan()
 *   resume() -> LVGL UI
 *   VHCI HCI events -> pcapng -> USB CDC -> Wireshark
 *                     -> btsnoop -> FAT (/spiflash/capture.btsnoop)
 *                     -> ring buffer -> shell "ble_sniff list"
 *
 *   stop() -> hci_deinit() + bt_manager_deinit()
 *
 *   Phase B: + esp_ble_internalTestFeaturesEnable() + VS commands
 *   Phase C: + CONNECT_REQ parsing + CSA #1 hop following
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "bt_manager.h"
#include "ble_sniffer_hci.h"
#include "ble_sniffer_vs.h"
#include "ble_sniffer_ll.h"
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

static volatile bool scanning = false;
static volatile bool sniffer_active = false;

static lv_obj_t *screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *count_label = NULL;
static lv_obj_t *btn_toggle = NULL;
static lv_obj_t *btn_record = NULL;
static lv_timer_t *sniffer_timer = NULL;
static uint32_t display_count = 0;

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

/* Ring buffer for Shell list */
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

/* ==================== HCI Packet Callback ==================== */

/**
 * @brief Called by HCI layer for each received packet (processing task context).
 *
 * For LE Advertising Reports, the payload is the reconstructed HCI event:
 *   0x3E, len, 0x02, num_reports, evt_type, addr_type, addr[6], data_len, data[N], rssi
 */
static void on_hci_packet(uint8_t h4_type, const uint8_t *data, uint16_t len)
{
    /* Currently we only handle LE Meta Events (advertising reports).
     * Future: handle ACL data (h4_type == 0x02) for data channel sniffing. */
    if (h4_type != H4_TYPE_EVT || len < 8 || data[2] != 0x02) {
        /* Vendor events (0xFF) might contain raw LL PDUs in Phase B */
        if (h4_type == H4_TYPE_EVT && data[0] == HCI_EVT_VENDOR) {
            ble_sniffer_pcap_write_packet(h4_type, data, len);
            if (btsnoop_recording) ble_sniffer_btsnoop_write_packet(h4_type, data, len);
        }
        return;
    }

    /* Parse LE Advertising Report fields */
    uint8_t evt_type  = data[4];   /* after event_code(1)+param_len(1)+sub(1)+num(1) */
    uint8_t addr_type = data[5];
    const uint8_t *addr = &data[6];
    uint8_t adv_len   = data[12];
    const uint8_t *adv_data = &data[13];
    int8_t rssi       = (int8_t)data[13 + adv_len];

    /* Extract name from AD structures */
    char name_buf[33];
    extract_name(adv_data, adv_len, name_buf, sizeof(name_buf));

    /* Reconstruct full HCI LE Advertising Report for pcapng/btsnoop */
    /* Format: 0x3E(1) + len(1) + sub(1) + num(1) + evt_type(1) + addr_type(1) + addr(6) + adv_len(1) + adv_data(N) + rssi(1) */
    uint8_t pkt[256];
    uint16_t pos = 0;
    pkt[pos++] = 0x3E;
    pkt[pos++] = 0;  /* length placeholder */
    pkt[pos++] = 0x02;  /* sub-event */
    pkt[pos++] = 1;  /* num reports */
    pkt[pos++] = evt_type;
    pkt[pos++] = addr_type;
    memcpy(&pkt[pos], addr, 6);
    pos += 6;
    pkt[pos++] = adv_len;
    if (adv_len > 0 && adv_data) {
        uint8_t copy_len = (adv_len > 31) ? 31 : adv_len;
        memcpy(&pkt[pos], adv_data, copy_len);
        pos += copy_len;
    }
    pkt[pos++] = (uint8_t)(rssi & 0xFF);
    pkt[1] = pos - 2;

    /* Output to pcapng (USB CDC) */
    ble_sniffer_pcap_write_packet(H4_TYPE_EVT, pkt, pos);

    /* Output to btsnoop (FAT) if recording */
    if (btsnoop_recording) {
        ble_sniffer_btsnoop_write_packet(H4_TYPE_EVT, pkt, pos);
    }

    /* Ring buffer for Shell list */
    int idx = pkt_ring_head;
    pkt_record_t *rec = &pkt_ring[idx];
    memcpy(rec->bda, addr, 6);
    rec->rssi = rssi;
    strncpy(rec->name, name_buf, sizeof(rec->name) - 1);
    rec->name[sizeof(rec->name) - 1] = '\0';
    rec->data_len = (pos > PKT_DATA_MAX) ? PKT_DATA_MAX : pos;
    memcpy(rec->data, pkt, rec->data_len);
    pkt_ring_head = (idx + 1) % PKT_RING_SIZE;
    if (pkt_ring_count < PKT_RING_SIZE) pkt_ring_count++;
}

/* ---- Shell commands ---- */

static void ble_sniffer_cmd_scan(void)
{
    if (scanning) { printf("Already scanning\n"); return; }
    hci_start_scan();
    scanning = true;
    printf("Scan started\n");
}

static void ble_sniffer_cmd_stop_scan(void)
{
    if (!scanning) return;
    hci_stop_scan();
    scanning = false;
    printf("Scan stopped\n");
}

static void ble_sniffer_cmd_status(void)
{
    printf("=== BLE Sniffer ===\n");
    printf("Mode: VHCI Controller-Only\n");
    printf("Scan: %s\n", scanning ? "Active" : "Stopped");
    printf("Packets: %lu (HCI adv reports)\n", (unsigned long)hci_get_packet_count());
    printf("\n");
    printf("-- VS Test Mode --\n");
    printf("Test mode: %s\n", vs_is_test_mode() ? "Enabled" : "Disabled");
    printf("AA: 0x%08lX (default broadcast)\n", (unsigned long)VS_AA_BROADCAST);
    printf("Note: VS test mode changes controller internal params.\n");
    printf("VHCI still receives standard HCI events only.\n");
    printf("Use 'counters' to poll VS counters.\n");
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
    pkt_ring_count = 0; pkt_ring_head = 0;
    printf("Cleared\n");
}

static void ble_sniffer_cmd_record(void)
{
    if (btsnoop_recording) { printf("Already recording\n"); return; }
    char path[64];
    snprintf(path, sizeof(path), "%s/capture.btsnoop", FAT_BASE);
    btsnoop_file = fopen(path, "wb");
    if (!btsnoop_file) {
        printf("Failed to open %s (FAT mounted?)\n", path);
        return;
    }
    ble_sniffer_btsnoop_init(btsnoop_file_writer);
    btsnoop_recording = true;
    printf("Recording to %s, use stoprec or UI to stop\n", path);
}

static void ble_sniffer_cmd_stoprec(void)
{
    if (!btsnoop_recording) { printf("Not recording\n"); return; }
    btsnoop_recording = false;
    if (btsnoop_file) { fclose(btsnoop_file); btsnoop_file = NULL; }
    printf("Recording stopped\n");
}

/* ---- VS/LL shell commands ---- */

static void ble_sniffer_cmd_vsenable(void)
{
    vs_enable_test_features();
    vs_set_scan_aa(VS_AA_BROADCAST);
    vs_set_scan_forever();
    printf("VS test mode enabled\n");
    printf("  - Scan AA set to broadcast (0x%08lX)\n", (unsigned long)VS_AA_BROADCAST);
    printf("  - Scan forever enabled (no timeout)\n");
    printf("  - VHCI still reports standard HCI events only\n");
    printf("  - Use 'counters' to read internal VS counters\n");
}

static void ble_sniffer_cmd_vsdisable(void)
{
    vs_disable_test_features();
    printf("VS test mode disabled\n");
}

static void ble_sniffer_cmd_scanchan(int argc, void *argv)
{
    if (argc < 1) { printf("Usage: ble_sniff scanchan <ch>\n"); return; }
    uint8_t ch = atoi((const char *)argv);
    if (ch != 37 && ch != 38 && ch != 39) {
        printf("Only advertising channels 37/38/39 supported\n");
        return;
    }
    /* Enable test mode if not already */
    if (!vs_is_test_mode()) {
        vs_enable_test_features();
        vs_set_scan_aa(VS_AA_BROADCAST);
    }
    vs_set_scan_channel(ch);
    /* Restart scan on the specific channel */
    printf("Scanning advertising channel %d only\n", ch);
}

static void ble_sniffer_cmd_counters(void)
{
    printf("Polling VS test mode counters...\n");
    printf("(Response appears in ESP log as HCI Command Complete events)\n");
    vs_get_scan_rxed_cnt();
    vs_get_adv_txed_cnt();
}

static ShellCommand ble_sniffer_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, scan, ble_sniffer_cmd_scan,
        scan\r\nstart capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stop, ble_sniffer_cmd_stop_scan,
        stop\r\nstop capture),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, status, ble_sniffer_cmd_status,
        status\r\nshow status & mode),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, list, ble_sniffer_cmd_list,
        list [n]\r\nlist recent n packets),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, clear, ble_sniffer_cmd_clear,
        clear\r\nclear counters),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, record, ble_sniffer_cmd_record,
        record\r\nrecord btsnoop to FAT),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stoprec, ble_sniffer_cmd_stoprec,
        stoprec\r\nstop btsnoop recording),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, vsenable, ble_sniffer_cmd_vsenable,
        vsenable\r\nenable VS test mode + scan forever),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, vsdisable, ble_sniffer_cmd_vsdisable,
        vsdisable\r\ndisable VS test mode),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, scanchan, ble_sniffer_cmd_scanchan,
        scanchan <37|38|39>\r\nscan specific advertising channel),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, counters, ble_sniffer_cmd_counters,
        counters\r\npoll VS test mode internal counters),
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
    if (scanning) {
        hci_stop_scan();
        scanning = false;
    } else {
        hci_start_scan();
        scanning = true;
    }
}

static void btn_record_cb(lv_event_t *e)
{
    (void)e;
    if (btsnoop_recording) {
        btsnoop_recording = false;
        if (btsnoop_file) { fclose(btsnoop_file); btsnoop_file = NULL; }
        ESP_LOGI(TAG, "Recording stopped");
    } else {
        char path[64];
        snprintf(path, sizeof(path), "%s/capture.btsnoop", FAT_BASE);
        btsnoop_file = fopen(path, "wb");
        if (!btsnoop_file) {
            ESP_LOGE(TAG, "Failed to open %s", path);
            return;
        }
        ble_sniffer_btsnoop_init(btsnoop_file_writer);
        btsnoop_recording = true;
        ESP_LOGI(TAG, "Recording to %s", path);
    }
}

static void ble_sniffer_update_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!sniffer_active) return;

    gui_lock();
    if (!screen || !count_label || !status_label) {
        gui_unlock();
        return;
    }

    uint32_t cnt = hci_get_packet_count();
    if (cnt != display_count) {
        display_count = cnt;
        lv_label_set_text_fmt(count_label, "Packets: %lu", (unsigned long)cnt);
    }
    if (btsnoop_recording) {
        lv_label_set_text(status_label, "Sniffing + REC");
        /* Record button text */
        if (btn_record) {
            lv_obj_t *r = lv_obj_get_child(btn_record, 0);
            if (r) lv_label_set_text(r, "Stop Rec");
            lv_obj_set_style_bg_color(btn_record, lv_color_hex(0xD32F2F), LV_PART_MAIN);
        }
    } else {
        lv_label_set_text(status_label, scanning ? "Sniffing..." : "Paused");
        if (btn_record) {
            lv_obj_t *r = lv_obj_get_child(btn_record, 0);
            if (r) lv_label_set_text(r, "Record");
            lv_obj_set_style_bg_color(btn_record, lv_color_hex(0x388E3C), LV_PART_MAIN);
        }
    }
    if (btn_toggle) {
        lv_obj_t *l = lv_obj_get_child(btn_toggle, 0);
        if (l) {
            lv_label_set_text(l, scanning ? "Stop" : "Scan");
        }
        lv_obj_set_style_bg_color(btn_toggle,
            scanning ? lv_color_hex(0xD32F2F) : lv_color_hex(0x1976D2),
            LV_PART_MAIN);
    }
    gui_unlock();
}

/* ---- Material Design style helpers ---- */

#define MD_MARGIN       16
#define MD_GAP          12

static void ble_sniffer_init_screen(void)
{
    if (screen) return;

    /* Screen — dark background */
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), LV_PART_MAIN);

    /* Status bar — single line, top center */
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, LV_PART_MAIN);

    /* Packet count — big number in center area */
    count_label = lv_label_create(screen);
    lv_label_set_text_fmt(count_label, "Packets: 0");
    lv_obj_align(count_label, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_24, LV_PART_MAIN);

    /* Bottom button row */
    int32_t btn_w = (LV_HOR_RES - MD_MARGIN * 2 - MD_GAP) / 2;

    btn_toggle = gui_create_md_button(screen, "Scan", btn_toggle_cb,
                                       lv_color_hex(0x1976D2), btn_w);
    lv_obj_align(btn_toggle, LV_ALIGN_BOTTOM_LEFT, MD_MARGIN, -MD_MARGIN);

    btn_record = gui_create_md_button(screen, "Record", btn_record_cb,
                                       lv_color_hex(0x388E3C), btn_w);
    lv_obj_align(btn_record, LV_ALIGN_BOTTOM_RIGHT, -MD_MARGIN, -MD_MARGIN);

    /* Timer for UI refresh */
    sniffer_active = true;
    sniffer_timer = lv_timer_create(ble_sniffer_update_cb, 500, NULL);
}

/* ---- RTAM Lifecycle ---- */

static RtAppErr ble_sniffer_init_app(void)
{
    /* Controller-Only 模式: 不启动 Bluedroid, 通过 VHCI 接收原始 HCI 包 */
    esp_err_t ret = bt_manager_init_sniffer(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT sniffer init failed: %s", esp_err_to_name(ret));
        return RTAM_OK;
    }

    /* 初始化 pcapng writer (输出到 USB CDC) */
    ble_sniffer_pcap_init(pcap_output_cb);

    /* 启动 VHCI 层 + HCI 事件处理任务 */
    hci_init(on_hci_packet);

    /* 发送 HCI 命令序列: Reset → SetEvtMask → SetScanParams → EnableScan */
    hci_init_scan();
    scanning = true;

    ESP_LOGI(TAG, "Sniffer started (VHCI controller-only mode)");
    return RTAM_OK;
}

static RtAppErr ble_sniffer_deinit_app(void)
{
    if (scanning) {
        hci_stop_scan();
        scanning = false;
    }
    if (btsnoop_recording) {
        btsnoop_recording = false;
        if (btsnoop_file) { fclose(btsnoop_file); btsnoop_file = NULL; }
    }
    /* 停止 HCI 处理任务并释放资源 */
    hci_deinit();
    /* 释放 BT 控制器 (完全 deinit, 下一个 app 会重新 init) */
    bt_manager_deinit();
    ESP_LOGI(TAG, "Sniffer stopped");
    return RTAM_OK;
}

static RtAppErr ble_sniffer_suspend(void)
{
    if (scanning) {
        hci_stop_scan();
        scanning = false;
    }
    if (btsnoop_recording) {
        btsnoop_recording = false;
        if (btsnoop_file) { fclose(btsnoop_file); btsnoop_file = NULL; }
    }

    gui_remove_global_gesture_callback(ble_sniffer_gesture_callback);

    /* 先置 flag 禁止定时器回调修改 UI。volatile 写是原子操作。
     * 定时器回调在 LVGL 任务中运行，已持有 GUI 锁，
     * 所以 suspend 中所有 LVGL 操作（launcher_go_home）也需要 gui_lock 保护。 */
    sniffer_active = false;
    screen = NULL;
    status_label = NULL;
    count_label = NULL;
    btn_toggle = NULL;
    btn_record = NULL;
    display_count = 0;

    gui_lock();
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    gui_unlock();
    return RTAM_OK;
}

static RtAppErr ble_sniffer_resume(void)
{
    ble_sniffer_init_screen();
    gui_push_screen(screen, LV_SCR_LOAD_ANIM_FADE_IN);
    gui_add_global_gesture_callback(ble_sniffer_gesture_callback);
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
