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
#include "freertos/projdefs.h"
#include "hal/uart_types.h"
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

#define SERIAL_DEBUG_SPI_PORT SPI2_HOST

static const char *TAG = "serial_debug_spi";

static lv_obj_t *spi_info_label = NULL;

struct spi_info {
    int miso_pin;
    int mosi_pin;
    int sclk_pin;
    int cs_pin;
    int speed;
    long data_num_sent;
    long data_num_received;
};

static struct spi_info info = {
    .speed = 1000000,
};

static void serial_debug_spi_update_info(void)
{
    if (spi_info_label) {
        lv_label_set_text_fmt(spi_info_label, "SPI\n"
            "Speed: %d\n"
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
}

void serial_debug_spi_deinit_info(void)
{
    spi_info_label = NULL;
}

void serial_debug_spi_init(int miso_pin, int mosi_pin, int sclk_pin, int cs_pin)
{
    info.miso_pin = miso_pin;
    info.mosi_pin = mosi_pin;
    info.sclk_pin = sclk_pin;
    info.cs_pin = cs_pin;
    spi_bus_config_t bus_conf = {
        .miso_io_num = miso_pin,
        .mosi_io_num = mosi_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SERIAL_DEBUG_SPI_PORT, &bus_conf, 1));
    spi_device_interface_config_t dev_conf = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = info.speed,
        .input_delay_ns = 0,
        .spics_io_num = cs_pin,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SERIAL_DEBUG_SPI_PORT, &dev_conf, NULL));

    serial_debug_spi_update_info();
}

void serial_debug_spi_deinit(void)
{
    ESP_ERROR_CHECK(spi_bus_remove_device(NULL));
    ESP_ERROR_CHECK(spi_bus_free(SERIAL_DEBUG_SPI_PORT));
    gpio_reset_pin(info.miso_pin);
    gpio_reset_pin(info.mosi_pin);
    gpio_reset_pin(info.sclk_pin);
    gpio_reset_pin(info.cs_pin);
}


static ShellCommand spi_group[] =
{
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
spid, spi_group, spi debug tool);
