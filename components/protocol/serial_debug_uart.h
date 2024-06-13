/**
 * @file serial_debug_uart.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug uart
 * @version 1.0.0
 * @date 2024-06-12
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SERIAL_DEBUG_UART_H__
#define __SERIAL_DEBUG_UART_H__

#include "lvgl.h"

void serial_debug_uart_init_info(lv_obj_t *label);
void serial_debug_uart_deinit_info(void);
void serial_debug_uart_init(int tx_pin, int rx_pin, int rts_pin, int cts_pin);
void serial_debug_uart_deinit(void);

#endif /* __SERIAL_DEBUG_UART_H__ */