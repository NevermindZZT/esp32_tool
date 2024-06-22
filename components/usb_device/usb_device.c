/**
 * @file usb_device.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief usb device
 * @version 1.0.0
 * @date 2024-06-01
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/dirent.h>
#include "esp_partition.h"
#include "esp_check.h"
#include "esp_vfs.h"
#include "esp_vfs_cdcacm.h"
#include "tinyusb.h"
#include "tusb_console.h"
#include "tusb_msc_storage.h"
#include "tusb_cdc_acm.h"

#include "freertos/FreeRTOS.h"
#include "rtam.h"
#include "shell.h"
#include "storage.h"

#define USB_DEVICE_USER_TINYUSB

static const char *TAG = "usb_device";

static Shell cdc_shell = {0};
static char cdc_shell_buffer[512];
#if defined(USB_DEVICE_USER_TINYUSB)
static SemaphoreHandle_t cdc_shell_semaphore = NULL;
struct {
    uint8_t buffer[CONFIG_TINYUSB_CDC_RX_BUFSIZE];
    size_t read;
    size_t write;
} cdc_ringbuf;
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
int cdc_handle = -1;
#endif

static signed short cdc_shell_write(char *data, unsigned short len)
{
#if defined(USB_DEVICE_USER_TINYUSB)
    signed short ret;
    ret = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (uint8_t *)data, len);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    return ret;
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    if (cdc_handle >= 0) {
        int ret = esp_vfs_write(__getreent(), cdc_handle, data, len);
        struct stat st;
        esp_vfs_fstat(__getreent(), cdc_handle, &st);
        return ret;
    } else {
        return 0;
    }
#endif
}

static signed short cdc_shell_read(char *data, unsigned short len)
{
#if defined(USB_DEVICE_USER_TINYUSB)
    // xSemaphoreTake(cdc_shell_semaphore, portMAX_DELAY);
    size_t remain = (cdc_ringbuf.write >= cdc_ringbuf.read) ?
                    cdc_ringbuf.write - cdc_ringbuf.read :
                    CONFIG_TINYUSB_CDC_RX_BUFSIZE - cdc_ringbuf.read + cdc_ringbuf.write;
    if (remain == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        return 0;
    }
    size_t read = len > remain ? remain : len;
    for (size_t i = 0; i < read; i++) {
        data[i] = cdc_ringbuf.buffer[cdc_ringbuf.read++];
        if (cdc_ringbuf.read > CONFIG_TINYUSB_CDC_RX_BUFSIZE) {
            cdc_ringbuf.read = 0;
        }
    }
    return read;
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    if (cdc_handle >= 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        return esp_vfs_read(__getreent(), cdc_handle, data, len);
    } else {
        return 0;
    }
#endif
}

static void cdc_shell_init(void)
{
#if defined(USB_DEVICE_USER_TINYUSB)
    // cdc_shell_semaphore = xSemaphoreCreateMutex();

    cdc_ringbuf.read = 0;
    cdc_ringbuf.write = 0;

    cdc_shell.write = cdc_shell_write;
    cdc_shell.read = cdc_shell_read;
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    ESP_ERROR_CHECK(esp_vfs_dev_cdcacm_register());
    const int flags = O_RDWR;
    cdc_handle = esp_vfs_open(__getreent(), "/dev/cdcacm", flags, 0);
    if (cdc_handle < 0) {
        ESP_LOGE(TAG, "Failed to open cdcacm device");
        return;
    }
    cdc_shell.write = cdc_shell_write;
    cdc_shell.read = cdc_shell_read;
#endif
    shellInit(&cdc_shell, cdc_shell_buffer, 512);
    xTaskCreate(shellTask, "cdc shell", 8192, &cdc_shell, 10, NULL);
}

#if defined(USB_DEVICE_USER_TINYUSB)
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    static uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
    size_t rx_size = 0;

    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret != ESP_OK || rx_size <= 0) {
        return;
    }
    for (size_t i = 0; i < rx_size; i++) {
        cdc_ringbuf.buffer[cdc_ringbuf.write++] = buf[i];
        if (cdc_ringbuf.write > CONFIG_TINYUSB_CDC_RX_BUFSIZE) {
            cdc_ringbuf.write = 0;
        }
    }
    // if (cdc_shell_semaphore != NULL) {
    //     xSemaphoreGive(cdc_shell_semaphore);
    // }
}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}
#endif

RtAppErr usb_device_init(void)
{
#if defined(USB_DEVICE_USER_TINYUSB)
    const tinyusb_config_t tusb_cfg = { 0 };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = tinyusb_cdc_line_state_changed_callback,
        .callback_line_coding_changed = NULL
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

#endif
    cdc_shell_init();

    tinyusb_msc_spiflash_config_t config_spi = {
        .wl_handle = storage_get_internal_handle(),
    };
    tinyusb_msc_storage_init_spiflash(&config_spi);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = usb_device_init,
};

static const char *required[] = {
    "storage",
    NULL
};

static const RtAppDependencies dependencies = {
    .required = required,
};

RTAPP_EXPORT(usb_device, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, &dependencies, NULL);

void usb_switch_log(char cdc)
{
    if (cdc) {
        ESP_ERROR_CHECK(esp_tusb_init_console(TINYUSB_CDC_ACM_0));
    } else {
        ESP_ERROR_CHECK(esp_tusb_deinit_console(TINYUSB_CDC_ACM_0));
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
swtich_log, usb_switch_log, switch log to usb or uart);
