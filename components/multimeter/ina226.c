/**
 * @file ina226.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief ina226
 * @version 1.0.0
 * @date 2024-07-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "ina226.h"
#include "driver/i2c.h"
#include "freertos/projdefs.h"

static const char *TAG = "ina226";

static float ina226_current_lsb = 0.0;
static float ina226_r_shunt = 0.0;

static int ina226_write(uint8_t reg, uint16_t data)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = data >> 8;
    buf[2] = data & 0xff;
    return i2c_master_write_to_device(INA226_I2C_BUS, INA226_ADDR, buf, 3, pdMS_TO_TICKS(100));
}

static int ina226_read(uint8_t reg, uint16_t *data)
{
    uint8_t buf[2] = {0};
    int res = i2c_master_write_read_device(INA226_I2C_BUS, INA226_ADDR, &reg, 1, buf, 2, pdMS_TO_TICKS(100));
    if (res == 0)
    {
        *data = (buf[0] << 8) | buf[1];
    }
    return res;
}

int ina226_debug_read(uint8_t reg)
{
    uint16_t data;
    ina226_read(reg, &data);
    ESP_LOGI(TAG, "reg: 0x%02x, data: 0x%04x", reg, data);
    return 0;

}

int ina226_debug_write(uint8_t reg, uint16_t data)
{
    ESP_LOGI(TAG, "reg: 0x%02x, data: 0x%04x", reg, data);
    return ina226_write(reg, data);
}

int ina226_reset(void)
{
    return ina226_write(INA226_REG_CONFIG, 0x8000);
}

unsigned int ina266_read_id(void)
{
    uint16_t mfg_id, die_id;
    ina226_read(INA226_REG_MFG_ID, &mfg_id);
    ina226_read(INA226_REG_DIE_ID, &die_id);
    return (mfg_id << 16) | die_id;
}

int ina226_init(float current_lsb, float r_shunt)
{
    ina226_current_lsb = current_lsb;
    ina226_r_shunt = r_shunt;
    ina226_write(INA226_REG_CONFIG, 0x4527);
    ina226_write(INA226_REG_CALIBRATION, (uint16_t)(0.00512 / (current_lsb * r_shunt)));
    return 0;
}

int ina226_read_voltage(void)
{
    uint16_t data;
    ina226_read(INA226_REG_BUS_VOLT, &data);
    return (int)((float)data * 1.25);
}

int ina226_read_shunt_voltage(void)
{
    uint16_t data;
    ina226_read(INA226_REG_SHUNT_VOLT, &data);
    return (int)(data * 2.5);
}

int ina226_read_current(void)
{
    uint16_t data;
    ina226_read(INA226_REG_CURRENT, &data);
    return (int)(data * ina226_current_lsb);
}

int ina226_read_power(void)
{
    uint16_t data;
    ina226_read(INA226_REG_POWER, &data);
    return (int)(data * ina226_current_lsb * 25);
}
