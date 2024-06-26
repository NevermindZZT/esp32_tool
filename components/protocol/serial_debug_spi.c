/**
 * @file serial_debug_spi.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug spi
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "freertos/projdefs.h"
#include "hal/uart_types.h"
#include "sdkconfig.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "shell_ext.h"
#include "string.h"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "esp_system.h"
#include "esp_log.h"
#include "layouts/lv_layout.h"
#include "lvgl.h"
#include "gui.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "protocol_common.h"
#include "stdlib/lv_sprintf.h"
#include "widgets/label/lv_label.h"

#define SERIAL_DEBUG_SPI_PORT CONFIG_PROTOCOL_SERIAL_DEBUG_SPI_PORT

static const char *TAG = "serial_debug_spi";

static lv_obj_t *spi_info_label = NULL;

struct spi_info {
    int cs_io;
    int sclk_io;
    int mosi_io;
    int miso_io;
    int mio2_io;
    int mio3_io;
    int speed;
    long data_num_sent;
    long data_num_received;
    int flags;
};

static spi_device_handle_t spi;

static struct spi_info info = {
    .speed = 1000000,
};

static void serial_debug_spi_update_info(void)
{
    if (spi_info_label) {
        lv_label_set_text_fmt(spi_info_label, "Speed: %d\n"
            "Data sent: %ld\n"
            "Data received: %ld\n",
            info.speed,
            info.data_num_sent,
            info.data_num_received);
    }
}

void serial_debug_spi_init_info(lv_obj_t *label)
{
    spi_info_label = label;
    serial_debug_spi_update_info();
}

void serial_debug_spi_deinit_info(void)
{
    spi_info_label = NULL;
}

void serial_debug_spi_init(int cs_pin, int sclk_pin, int mosi_pin, int miso_pin, int mio2_pin, int mio3_pin)
{
    info.cs_io = protocol_get_io(cs_pin);
    info.sclk_io = protocol_get_io(sclk_pin);
    info.mosi_io = protocol_get_io(mosi_pin);
    info.miso_io = protocol_get_io(miso_pin);
    info.mio2_io = protocol_get_io(mio2_pin);
    info.mio3_io = protocol_get_io(mio3_pin);
    ESP_LOGI(TAG, "cs: %d, sclk: %d, mosi: %d, miso: %d, mio2: %d, mio3: %d",
        info.cs_io, info.sclk_io, info.mosi_io, info.miso_io, info.mio2_io, info.mio3_io);
    protocol_set_pin(cs_pin, "CS", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin(sclk_pin, "SCLK", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin(mosi_pin, "MOSI", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin(miso_pin, "MISO", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin(mio2_pin, "MIO2", lv_palette_main(LV_PALETTE_GREEN));
    protocol_set_pin(mio3_pin, "MIO3", lv_palette_main(LV_PALETTE_GREEN));
    spi_bus_config_t bus_conf = {
        .sclk_io_num = info.sclk_io,
        .mosi_io_num = info.mosi_io,
        .miso_io_num = info.miso_io,
        .data2_io_num = info.mio2_io,
        .data3_io_num = info.mio3_io,
        .max_transfer_sz = 0,
        .intr_flags = ESP_INTR_FLAG_SHARED|ESP_INTR_FLAG_LEVEL3,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SERIAL_DEBUG_SPI_PORT, &bus_conf, SPI_DMA_DISABLED));
    spi_device_interface_config_t dev_conf = {
        .clock_speed_hz = info.speed,
        .mode = 0,
        .spics_io_num = info.cs_io,
        .input_delay_ns = 0,
        .queue_size = 10,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SERIAL_DEBUG_SPI_PORT, &dev_conf, &spi));

    serial_debug_spi_update_info();
}

void serial_debug_spi_deinit(void)
{
    if (spi != NULL) {
        ESP_ERROR_CHECK(spi_bus_remove_device(spi));
    }
    ESP_ERROR_CHECK(spi_bus_free(SERIAL_DEBUG_SPI_PORT));
    gpio_reset_pin(info.cs_io);
    gpio_reset_pin(info.sclk_io);
    gpio_reset_pin(info.mosi_io);
    gpio_reset_pin(info.miso_io);
    gpio_reset_pin(info.mio2_io);
    gpio_reset_pin(info.mio3_io);
}

esp_err_t serial_debug_spi_transmit(uint8_t *data, uint8_t *out, size_t length)
{
    spi_transaction_ext_t t = {0};

    t.base.length = length * 8;
    
    t.base.tx_buffer = data;

    if (out != NULL) {
        t.base.rx_buffer = out;
    }

    t.base.flags = info.flags;

    return spi_device_polling_transmit(spi, (spi_transaction_t *) &t);
}

void serial_debug_spi_write_read(uint8_t *data)
{
    int length = shellGetArrayParamSize(data);
    uint8_t *out = heap_caps_malloc(length, MALLOC_CAP_DEFAULT);
    if (out == NULL) {
        shellPrint(shellGetCurrent(), "malloc failed");
    }
    esp_err_t ret = serial_debug_spi_transmit(data, out, length);
    if (ret != ESP_OK) {
        shellPrint(shellGetCurrent(), "spi write read failed");
        heap_caps_free(out);
        return;
    }
    ESP_LOG_BUFFER_HEX("spi read", out, length);
    heap_caps_free(out);
    info.data_num_sent += length;
    info.data_num_received += length;
}


static ShellCommand spi_group[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, write_read, serial_debug_spi_write_read,
        write and read data for spi device\r\nspid write_read [data], .data.cmd.signature="[q"),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
spid, spi_group, spi debug tool);
