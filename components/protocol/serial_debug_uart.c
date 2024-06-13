/**
 * @file serial_debug_uart.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug uart
 * @version 1.0.0
 * @date 2024-06-12
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "driver/gpio.h"
#include "esp_task_wdt.h"
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
#include "driver/uart.h"
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
#include "cpost.h"

#define SERIAL_DEBUG_UART_PORT UART_NUM_1

#define SERIAL_DEBUG_UART_CONSOLE_EXIT_KEY 0x04 // Ctrl D

#define SERIAL_DEBUG_UART_MODE_NORMAL 0
#define SERIAL_DEBUG_UART_MODE_CONSOLE 1

static const char *TAG = "serial_debug_uart";

static lv_obj_t *uart_info_label = NULL;

struct uart_info {
    bool run;
    char uart_mode;
    Shell *active_shell;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    int cts_pin;
    long data_num_sent;
    long data_num_received;
};

static struct uart_info info = {0};

static void serial_debug_uart_update_info(void)
{
    uint32_t baudrate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;

    uart_get_baudrate(SERIAL_DEBUG_UART_PORT, &baudrate);
    uart_get_word_length(SERIAL_DEBUG_UART_PORT, &data_bits);
    uart_get_parity(SERIAL_DEBUG_UART_PORT, &parity);
    uart_get_stop_bits(SERIAL_DEBUG_UART_PORT, &stop_bits);
    uart_get_hw_flow_ctrl(SERIAL_DEBUG_UART_PORT, &flow_ctrl);

    if (uart_info_label) {
        lv_label_set_text_fmt(uart_info_label,
                              "Baudrate: %d\n"
                              "Data bits: %s\n"
                              "Parity: %s\n"
                              "Stop bits: %s\n"
                              "Flow control: %s\n"
                              "Mode: %s\n"
                              "Send: %ld\n"
                              "Receive: %ld",
                              (int) baudrate,
                              data_bits == UART_DATA_5_BITS ? "5"
                                    : (data_bits == UART_DATA_6_BITS) ? "6"
                                    : (data_bits == UART_DATA_7_BITS) ? "7"
                                    : (data_bits == UART_DATA_8_BITS) ? "8" : "Max",
                              parity == UART_PARITY_DISABLE ? "None" : (parity == UART_PARITY_EVEN ? "Even" : "Odd"),
                              stop_bits == UART_STOP_BITS_1 ? "1" : (stop_bits == UART_STOP_BITS_1_5) ? "1.5" : "2",
                              flow_ctrl == UART_HW_FLOWCTRL_DISABLE ? "None"
                                    : (flow_ctrl == UART_HW_FLOWCTRL_RTS ? "RTS"
                                    : (flow_ctrl == UART_HW_FLOWCTRL_CTS) ? "CTS" : "RTS/CTS"),
                              info.uart_mode == SERIAL_DEBUG_UART_MODE_NORMAL ? "Normal" : "Console",
                              info.data_num_sent,
                              info.data_num_received);
    }
}

void serial_debug_uart_init_info(lv_obj_t *label)
{
    uart_info_label = label;
}

void serial_debug_uart_deinit_info(void)
{
    uart_info_label = NULL;
}

static void serial_debug_uart_task(void *param)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_set_pin(SERIAL_DEBUG_UART_PORT, 
                 info.tx_pin, info.rx_pin, info.rts_pin, info.cts_pin);
    uart_param_config(SERIAL_DEBUG_UART_PORT, &uart_config);
    uart_driver_install(SERIAL_DEBUG_UART_PORT, 512, 512, 0, NULL, 0);

    serial_debug_uart_update_info();

    uint8_t *data = (uint8_t *)heap_caps_malloc(128, MALLOC_CAP_DEFAULT);
    while (info.run) {
        int len = uart_read_bytes(SERIAL_DEBUG_UART_PORT, data, 128, 
                                  pdMS_TO_TICKS(info.uart_mode == SERIAL_DEBUG_UART_MODE_NORMAL ? 100 : 20));
        if (len > 0) {
            info.data_num_received += len;
            if (info.uart_mode == SERIAL_DEBUG_UART_MODE_NORMAL) {
                ESP_LOG_BUFFER_HEX("uart received", data, len);
            } else {
                if (info.active_shell) {
                    info.active_shell->write((char *)data, len);
                }
            }
            serial_debug_uart_update_info();
        }
    };
    heap_caps_free(data);
    
    uart_driver_delete(SERIAL_DEBUG_UART_PORT);
    gpio_reset_pin(info.tx_pin);
    gpio_reset_pin(info.rx_pin);
    gpio_reset_pin(info.rts_pin);
    gpio_reset_pin(info.cts_pin);

    vTaskDelete(NULL);
}

void serial_debug_uart_init(int tx_pin, int rx_pin, int rts_pin, int cts_pin)
{
    info.run = true;
    info.tx_pin = tx_pin;
    info.rx_pin = rx_pin;
    info.rts_pin = rts_pin;
    info.cts_pin = cts_pin;
    xTaskCreate(serial_debug_uart_task, "serial debug uart", 4096, NULL, 1, NULL);
}

void serial_debug_uart_deinit(void)
{
    info.run = false;
}

static void serial_debug_uart_set_baudrate(int baudrate)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    uart_set_baudrate(SERIAL_DEBUG_UART_PORT, baudrate);
    serial_debug_uart_update_info();
}

static void serial_debug_uart_set_data_bits(int data_bits)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    if (data_bits < UART_DATA_5_BITS || data_bits > UART_DATA_BITS_MAX) {
        shellPrint(shellGetCurrent(), "data bits param error\r\n");
        return;
    }
    uart_set_word_length(SERIAL_DEBUG_UART_PORT, data_bits);
    serial_debug_uart_update_info();
}

static void serial_debug_uart_set_parity(int parity)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    if (parity < UART_PARITY_DISABLE || parity > UART_PARITY_ODD) {
        shellPrint(shellGetCurrent(), "parity param error\r\n");
        return;
    }
    uart_set_parity(SERIAL_DEBUG_UART_PORT, parity);
    serial_debug_uart_update_info();
}

static void serial_debug_uart_set_stop_bits(int stop_bits)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    if (stop_bits < UART_STOP_BITS_1 || stop_bits > UART_STOP_BITS_MAX) {
        shellPrint(shellGetCurrent(), "stop bits param error\r\n");
        return;
    }
    uart_set_stop_bits(SERIAL_DEBUG_UART_PORT, stop_bits);
    serial_debug_uart_update_info();
}

static void serial_debug_uart_set_flow_ctrl(int flow_ctrl, uint8_t rx_thresh)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    if (flow_ctrl < UART_HW_FLOWCTRL_DISABLE || flow_ctrl > UART_HW_FLOWCTRL_MAX) {
        shellPrint(shellGetCurrent(), "flow ctrl param error\r\n");
        return;
    }
    uart_set_hw_flow_ctrl(SERIAL_DEBUG_UART_PORT, flow_ctrl, rx_thresh);
    serial_debug_uart_update_info();
}

static void serial_debug_uart_set_mode(int mode)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    if (mode < SERIAL_DEBUG_UART_MODE_NORMAL || mode > SERIAL_DEBUG_UART_MODE_CONSOLE) {
        shellPrint(shellGetCurrent(), "mode param error\r\n");
        return;
    }
    info.uart_mode = mode;
    serial_debug_uart_update_info();
}

static void serial_debug_uart_send_data(char *data, int len)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    uart_write_bytes(SERIAL_DEBUG_UART_PORT, data, len);
    info.data_num_sent += len;
    serial_debug_uart_update_info();
}

static void serial_debug_uart_send(char *data)
{
    int len = shellGetArrayParamSize(data);
    serial_debug_uart_send_data(data, len);
    ESP_LOG_BUFFER_HEX("uart sent", data, len);
}

static void serial_debug_uart_send_string(char *str)
{
    serial_debug_uart_send_data(str, strlen(str) + 1);
    ESP_LOGI("uart sent", "%s", str);
}

static void serial_debug_uart_console(void)
{
    if (!info.run) {
        shellPrint(shellGetCurrent(), "uartd not running\r\n");
        return;
    }
    info.uart_mode = SERIAL_DEBUG_UART_MODE_CONSOLE;
    serial_debug_uart_update_info();
    info.active_shell = shellGetCurrent();
    char data;
    while (info.run) {
        if (info.active_shell->read(&data, 1) == 1) {
            if (data == SERIAL_DEBUG_UART_CONSOLE_EXIT_KEY) {
                break;
            } else {
                uart_write_bytes(SERIAL_DEBUG_UART_PORT, &data, 1);
                info.data_num_sent++;
                cpost(serial_debug_uart_update_info, .delay=50, .attrs.flag=CPOST_FLAG_CANCEL_CURRENT);
            }
        }
    }
    info.active_shell = NULL;
    info.uart_mode = SERIAL_DEBUG_UART_MODE_NORMAL;
    serial_debug_uart_update_info();
}

static ShellCommand uart_group[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, baudrate, serial_debug_uart_set_baudrate, 
        set uart baudrate\r\nuartd baudrate [baudrate]),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, databits, serial_debug_uart_set_data_bits,
        set uart data bits\r\nuartd databits [data bits]\r\n
        0 - 5 bits\r\n
        1 - 6 bits\r\n
        2 - 7 bits\r\n
        3 - 8 bits),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, parity, serial_debug_uart_set_parity,
        set uart parity\r\nuartd parity [parity]\r\n
        1 - Disable\r\n
        2 - Even\r\n
        3 - Odd),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, stopbits, serial_debug_uart_set_stop_bits,
        set uart stop bits\r\nuartd stopbits [stop bits]\r\n
        1 - 1 bits\r\n
        2 - 1.5 bits\r\n
        3 - 2 bits),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, flowctrl, serial_debug_uart_set_flow_ctrl,
        set uart flow control\r\nuartd flowctrl [flow control] [rx threshold]\r\n
        flow control:\r\n
        0 - Disable\r\n
        1 - Rts\r\n
        2 - Cts\r\n
        3 - Rts/Cts),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, mode, serial_debug_uart_set_mode,
        set uart mode\r\nuartd mode [mode]\r\n
        0 - Normal\r\n
        1 - Console),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, send, serial_debug_uart_send,
        send data to uart\r\nuartd send [data], .data.cmd.signature="[q"),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, sendstr, serial_debug_uart_send_string,
        send string to uart\r\nuartd send [str]),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, console, serial_debug_uart_console,
        start uart console),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
uartd, uart_group, uart debug tool);

