/**
 * @file serial_debug_spi.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug spi
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SERIAL_DEBUG_SPI_H__
#define __SERIAL_DEBUG_SPI_H__

#include "lvgl.h"

void serial_debug_spi_init_info(lv_obj_t *label);
void serial_debug_spi_deinit_info(void);
void serial_debug_spi_init(int cs_pin, int sclk_pin, int mosi_pin, int miso_pin, int mio2_pin, int mio3_pin);
void serial_debug_spi_deinit(void);

#endif /* __SERIAL_DEBUG_SPI_H__ */ 