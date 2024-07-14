/**
 * @file ina226.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief ina226
 * @version 1.0.0
 * @date 2024-07-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __INA226_H__
#define __INA226_H__

#include "esp_log.h"

#define INA226_ADDR     0x40
#define INA226_I2C_BUS  1

#define INA226_REG_CONFIG       0x00
#define INA226_REG_SHUNT_VOLT   0x01
#define INA226_REG_BUS_VOLT     0x02
#define INA226_REG_POWER        0x03
#define INA226_REG_CURRENT      0x04
#define INA226_REG_CALIBRATION  0x05
#define INA226_REG_MASK_ENABLE  0x06
#define INA226_REG_ALERT_LIMIT  0x07
#define INA226_REG_MFG_ID       0xFE
#define INA226_REG_DIE_ID       0xFF

int ina226_debug_read(uint8_t reg);
int ina226_debug_write(uint8_t reg, uint16_t data);
int ina226_reset(void);
unsigned int ina266_read_id(void);
int ina226_init(float current_lsb, float r_shunt);
int ina226_read_voltage(void);
int ina226_read_shunt_voltage(void);
int ina226_read_current(void);
int ina226_read_power(void);

#endif
