/**
 * @file serial_debug_i2c.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief serial debug i2c
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SERIAL_DEBUG_I2C_H__
#define __SERIAL_DEBUG_I2C_H__

#include "lvgl.h"

void serial_debug_i2c_init_info(lv_obj_t *label);
void serial_debug_i2c_deinit_info(void);
void serial_debug_i2c_init(int sda_pin, int scl_pin);
void serial_debug_i2c_deinit(void);

#endif /* __SERIAL_DEBUG_I2C_H__ */
