/**
 * @file ble_sniffer_hci.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief BLE Sniffer - VHCI / HCI command layer implementation
 * @version 1.0.0
 * @date 2026-07-18
 *
 * VHCI receive callback → event queue → processing task → packet callback
 */
#include "ble_sniffer_hci.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "ble_sniffer_hci";

/* ===================== Internal State ===================== */

static TaskHandle_t hci_task_handle = NULL;
static QueueHandle_t hci_evt_queue = NULL;
static volatile bool hci_scanning = false;
static volatile uint32_t hci_pkt_count = 0;
static hci_packet_cb_t user_packet_cb = NULL;

#define HCI_EVT_QUEUE_LEN   32
#define HCI_PROC_TASK_STACK 4096
#define HCI_PROC_TASK_PRIO  5

/* ===================== HCI Command Building ===================== */

static inline uint16_t make_opcode(uint8_t ogf, uint16_t ocf)
{
    return (ogf << 10) | ocf;
}

uint16_t hci_build_cmd_reset(uint8_t *buf)
{
    uint16_t opcode = make_opcode(OGF_HOST_CTRL, OCF_RESET);
    buf[0] = H4_TYPE_CMD;
    buf[1] = opcode & 0xFF;
    buf[2] = (opcode >> 8) & 0xFF;
    buf[3] = 0;  /* no parameters */
    return 4;
}

uint16_t hci_build_cmd_set_event_mask(uint8_t *buf, const uint8_t *mask)
{
    uint16_t opcode = make_opcode(OGF_HOST_CTRL, OCF_SET_EVT_MASK);
    buf[0] = H4_TYPE_CMD;
    buf[1] = opcode & 0xFF;
    buf[2] = (opcode >> 8) & 0xFF;
    buf[3] = 8;  /* parameter length */
    memcpy(&buf[4], mask, 8);
    return 12;
}

uint16_t hci_build_le_set_scan_params(uint8_t *buf, uint8_t scan_type,
                                       uint16_t scan_interval, uint16_t scan_window,
                                       uint8_t own_addr_type, uint8_t filter_policy)
{
    uint16_t opcode = make_opcode(OGF_LE_CTRL, OCF_LE_SET_SCAN_PARAMS);
    buf[0] = H4_TYPE_CMD;
    buf[1] = opcode & 0xFF;
    buf[2] = (opcode >> 8) & 0xFF;
    buf[3] = 7;  /* parameter length */
    buf[4] = scan_type;
    buf[5] = scan_interval & 0xFF;
    buf[6] = (scan_interval >> 8) & 0xFF;
    buf[7] = scan_window & 0xFF;
    buf[8] = (scan_window >> 8) & 0xFF;
    buf[9] = own_addr_type;
    buf[10] = filter_policy;
    return 11;
}

uint16_t hci_build_le_set_scan_enable(uint8_t *buf, uint8_t enable, uint8_t filter_dups)
{
    uint16_t opcode = make_opcode(OGF_LE_CTRL, OCF_LE_SET_SCAN_ENABLE);
    buf[0] = H4_TYPE_CMD;
    buf[1] = opcode & 0xFF;
    buf[2] = (opcode >> 8) & 0xFF;
    buf[3] = 2;  /* parameter length */
    buf[4] = enable;
    buf[5] = filter_dups;
    return 6;
}

/* ===================== VHCI Callbacks ===================== */

static void vhci_send_available_cb(void)
{
    /* Not used - VHCI always reports available */
}

static int vhci_recv_cb(uint8_t *data, uint16_t len)
{
    if (!hci_evt_queue) return 0;

    /* Copy data and enqueue for processing task */
    uint8_t *copy = (uint8_t *)malloc(len);
    if (!copy) return 0;
    memcpy(copy, data, len);

    if (xQueueSend(hci_evt_queue, &copy, 0) != pdPASS) {
        free(copy);
    }
    return 0;
}

static const esp_vhci_host_callback_t vhci_callbacks = {
    .notify_host_send_available = vhci_send_available_cb,
    .notify_host_recv = vhci_recv_cb,
};

/* ===================== HCI Event Processing ===================== */

/**
 * @brief Process an LE Advertising Report event.
 * Extracts data and notifies user callback.
 */
static void process_adv_report(const uint8_t *data, uint16_t len)
{
    /* data points to sub-event parameters (after sub-event code) */
    /* Format: NumReports(1) + [EvtType(1)+AddrType(1)+Addr(6)+DataLen(1)+Data(N)+RSSI(1)] x N */
    if (len < 2) return;

    uint8_t num_reports = data[0];
    uint16_t offset = 1;

    for (int r = 0; r < num_reports && offset + 8 < len; r++) {
        /* Save start of this report for backtracking */
        uint16_t report_start = offset;

        /* skip evt_type, addr_type, addr, data_len */
        offset += 1 + 1 + 6 + 1;  /* evt_type(1) + addr_type(1) + addr(6) + data_len(1) */
        uint8_t data_len = data[offset - 1];

        if (offset + data_len + 1 > len) break;

        /* Build HCI LE Advertising Report event */
        uint8_t pkt[256];
        uint16_t pos = 0;

        /* Re-read from saved position */
        offset = report_start;

        pkt[pos++] = 0x3E;         /* LE Meta event */
        pkt[pos++] = 0x00;         /* len placeholder */
        pkt[pos++] = 0x02;         /* sub-event */
        pkt[pos++] = 1;            /* num reports */
        pkt[pos++] = data[offset++]; /* evt_type */
        pkt[pos++] = data[offset++]; /* addr_type */
        memcpy(&pkt[pos], &data[offset], 6); /* addr */
        pos += 6;
        offset += 6;
        pkt[pos++] = data[offset++]; /* data_len */
        if (data_len > 0) {
            memcpy(&pkt[pos], &data[offset], data_len);
            pos += data_len;
            offset += data_len;
        }
        pkt[pos++] = data[offset++]; /* RSSI */
        pkt[1] = pos - 2;

        if (user_packet_cb) {
            user_packet_cb(H4_TYPE_EVT, pkt, pos);
        }
        hci_pkt_count++;
    }
}

/**
 * @brief Process a single HCI event from the queue.
 */
static void process_hci_event(const uint8_t *data, uint16_t len)
{
    if (len < 2) return;

    uint8_t h4_type = data[0];      /* Should be 0x04 for events */
    uint8_t evt_code = data[1];

    if (h4_type != H4_TYPE_EVT) return;

    switch (evt_code) {
    case HCI_EVT_CMD_COMPLETE:
        /* Command Complete - command acknowledged */
        break;

    case HCI_EVT_CMD_STATUS:
        /* Command Status - command accepted */
        break;

    case HCI_EVT_LE_META: {
        /* LE Meta Event */
        if (len < 4) break;
        uint8_t sub_evt = data[3];

        switch (sub_evt) {
        case LE_SUB_ADV_REPORT:
            process_adv_report(&data[4], len - 4);
            break;

        case LE_SUB_EXT_ADV_REPORT:
        case LE_SUB_PERIODIC_ADV_REPORT:
            /* Extended advertising - not yet handled */
            break;

        default:
            break;
        }
        break;
    }

    case HCI_EVT_VENDOR:
        /* Vendor-specific event - pass through to callback */
        if (user_packet_cb) {
            user_packet_cb(h4_type, &data[1], len - 1);
        }
        break;

    default:
        break;
    }
}

/* ===================== Processing Task ===================== */

static void hci_proc_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "HCI processing task started");

    while (1) {
        uint8_t *evt = NULL;

        if (xQueueReceive(hci_evt_queue, &evt, portMAX_DELAY) == pdTRUE && evt) {
            /* Determine data length from H4 header type */
            uint16_t data_len = 0;

            if (evt[0] == H4_TYPE_EVT && evt[2] > 0) {
                /* Event: H4(1) + EvtCode(1) + ParamLen(1) + Params(N) */
                data_len = 3 + evt[2];
            } else if (evt[0] == H4_TYPE_ACL) {
                /* ACL:  H4(1) + Handle(2) + DataLen(2) + Data(N) */
                data_len = 5 + (evt[3] | (evt[4] << 8));
            } else {
                data_len = 256; /* fallback */
            }

            process_hci_event(evt, data_len);
            free(evt);
        }
    }
}

/* ===================== Public API ===================== */

void hci_init(hci_packet_cb_t packet_cb)
{
    user_packet_cb = packet_cb;

    /* Create event queue */
    if (!hci_evt_queue) {
        hci_evt_queue = xQueueCreate(HCI_EVT_QUEUE_LEN, sizeof(uint8_t *));
    }

    /* Register VHCI callbacks */
    esp_vhci_host_register_callback(&vhci_callbacks);

    /* Start processing task */
    if (!hci_task_handle) {
        xTaskCreatePinnedToCore(hci_proc_task, "hci_proc",
                                HCI_PROC_TASK_STACK, NULL,
                                HCI_PROC_TASK_PRIO, &hci_task_handle, 0);
    }

    ESP_LOGI(TAG, "HCI layer initialized");
}

void hci_send_cmd(const uint8_t *buf, uint16_t len)
{
    if (buf && len > 0) {
        esp_vhci_host_send_packet((uint8_t *)buf, len);
    }
}

void hci_start_scan(void)
{
    uint8_t buf[6];
    uint16_t len = hci_build_le_set_scan_enable(buf, 1, 0);
    hci_send_cmd(buf, len);
    hci_scanning = true;
    ESP_LOGI(TAG, "Scan started");
}

void hci_stop_scan(void)
{
    uint8_t buf[6];
    uint16_t len = hci_build_le_set_scan_enable(buf, 0, 0);
    hci_send_cmd(buf, len);
    hci_scanning = false;
    ESP_LOGI(TAG, "Scan stopped");
}

void hci_init_scan(void)
{
    uint8_t buf[32];
    uint16_t len;

    ESP_LOGI(TAG, "Sending HCI init sequence...");

    /* 1. Reset */
    len = hci_build_cmd_reset(buf);
    hci_send_cmd(buf, len);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 2. Set event mask - enable LE Meta Events (bit 61) */
    uint8_t evt_mask[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20};
    len = hci_build_cmd_set_event_mask(buf, evt_mask);
    hci_send_cmd(buf, len);
    vTaskDelay(pdMS_TO_TICKS(30));

    /* 3. Set scan parameters */
    /* 0x30 = 48 slots = 30ms scan window, 0x50 = 80 slots = 50ms interval */
    len = hci_build_le_set_scan_params(buf, 1, 0x50, 0x30, 0, 0);
    hci_send_cmd(buf, len);
    vTaskDelay(pdMS_TO_TICKS(30));

    /* 4. Enable scanning */
    hci_start_scan();
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "HCI init sequence complete, scanning");
}

bool hci_is_scanning(void)
{
    return hci_scanning;
}

uint32_t hci_get_packet_count(void)
{
    return hci_pkt_count;
}

void hci_deinit(void)
{
    if (hci_scanning) {
        hci_stop_scan();
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    /* Stop processing task */
    if (hci_task_handle) {
        vTaskDelete(hci_task_handle);
        hci_task_handle = NULL;
    }

    /* Flush queue */
    if (hci_evt_queue) {
        uint8_t *evt;
        while (xQueueReceive(hci_evt_queue, &evt, 0) == pdTRUE) {
            if (evt) free(evt);
        }
        vQueueDelete(hci_evt_queue);
        hci_evt_queue = NULL;
    }

    user_packet_cb = NULL;
    hci_pkt_count = 0;
    ESP_LOGI(TAG, "HCI layer deinitialized");
}
