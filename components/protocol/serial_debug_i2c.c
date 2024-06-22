/**
 * @file serial_debug_i2c.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug i2c
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
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

#define SERIAL_DEBUG_I2C_PORT I2C_NUM_0

static const char *TAG = "serial_debug_i2c";

static lv_obj_t *i2c_info_label = NULL;

struct i2c_info {
    int sda_pin;
    int scl_pin;
    bool pullup_en;
    int speed;
    long data_num_sent;
    long data_num_received;
};

static struct i2c_info info = {
    .pullup_en = true,
    .speed = 100000,
};

static void serial_debug_i2c_update_info(void)
{
    if (i2c_info_label) {
        lv_label_set_text_fmt(i2c_info_label, "Pullup: %s\n"
            "Speed: %d\n"
            "Data sent: %ld\n"
            "Data received: %ld",
            info.pullup_en ? "Enabled" : "Disabled",
            info.speed, info.data_num_sent, info.data_num_received);
    }
}

void serial_debug_i2c_init_info(lv_obj_t *label)
{
    i2c_info_label = label;
}

void serial_debug_i2c_deinit_info(void)
{
    i2c_info_label = NULL;
}

void serial_debug_i2c_init(int sda_pin, int scl_pin)
{
    info.sda_pin = sda_pin;
    info.scl_pin = scl_pin;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = info.pullup_en,
        .scl_pullup_en = info.pullup_en,
        .master.clk_speed = info.speed,
    };
    ESP_ERROR_CHECK(i2c_param_config(SERIAL_DEBUG_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(SERIAL_DEBUG_I2C_PORT, conf.mode, 0, 0, 0));
    
    serial_debug_i2c_update_info();
}

void serial_debug_i2c_deinit(void)
{
    ESP_ERROR_CHECK(i2c_driver_delete(SERIAL_DEBUG_I2C_PORT));
    gpio_reset_pin(info.sda_pin);
    gpio_reset_pin(info.scl_pin);
}

void serial_debug_i2c_set_speed(int speed)
{
    info.speed = speed;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = info.sda_pin,
        .scl_io_num = info.scl_pin,
        .sda_pullup_en = info.pullup_en,
        .scl_pullup_en = info.pullup_en,
        .master.clk_speed = info.speed,
    };
    ESP_ERROR_CHECK(i2c_param_config(SERIAL_DEBUG_I2C_PORT, &conf));
    serial_debug_i2c_update_info();
}

void serial_debug_i2c_set_pullup(char pullup)
{
    info.pullup_en = pullup ? true : false;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = info.sda_pin,
        .scl_io_num = info.scl_pin,
        .sda_pullup_en = info.pullup_en,
        .scl_pullup_en = info.pullup_en,
        .master.clk_speed = info.speed,
    };
    ESP_ERROR_CHECK(i2c_param_config(SERIAL_DEBUG_I2C_PORT, &conf));
    serial_debug_i2c_update_info();
}

void serial_debug_i2c_send(uint8_t addr, uint8_t *data)
{
    size_t len = shellGetArrayParamSize(data);
    i2c_master_write_to_device(SERIAL_DEBUG_I2C_PORT, addr, data, len, pdMS_TO_TICKS(100));
    ESP_LOG_BUFFER_HEX("i2c send", data, len);

    info.data_num_sent += len;
    serial_debug_i2c_update_info();
}

void serial_debug_i2c_receive(uint8_t addr, size_t len)
{
    uint8_t *data = heap_caps_malloc(len, MALLOC_CAP_DEFAULT);
    if (!data) {
        ESP_LOGE(TAG, "malloc failed");
        return;
    }
    i2c_master_read_from_device(SERIAL_DEBUG_I2C_PORT, addr, data, len, pdMS_TO_TICKS(100));
    ESP_LOG_BUFFER_HEX("i2c receive", data, len);
    heap_caps_free(data);
    info.data_num_received += len;
    serial_debug_i2c_update_info();
}

static ShellCommand i2c_group[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, speed, serial_debug_i2c_set_speed, 
        set i2c speed\r\ni2cd speed [speed]),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, pullup, serial_debug_i2c_set_pullup,
        set i2c internal pullup enabled\r\ni2cd pullup [enabled]\r\n
        0 - Disabled\r\n
        1 - Enabled),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, send, serial_debug_i2c_send,
        send data to i2c device\r\ni2cd send [addr] [data], .data.cmd.signature="q[q"),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, receive, serial_debug_i2c_receive,
        receive data from i2c device\r\ni2cd receive [len]),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
i2cd, i2c_group, i2c debug tool);
