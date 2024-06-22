/**
 * @file battery.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief battery service
 * @version 1.0.0
 * @date 2024-06-08
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "esp_system.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "shell.h"
#include "rtam.h"

#define BATTERY_ADC_UINT ADC_UNIT_1
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_7
#define BATTERY_CHARGE_GPIO GPIO_NUM_3
#define BATTERY_STANDBY_GPIO GPIO_NUM_46

static const char *TAG = "battery";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle; 

static const int battery_voltage_table[] = {
    3450,
    3680,
    3740,
    3770,
    3790,
    3820,
    3870,
    3920,
    3980,
    4060,
    4200,
};

int battery_get_voltage(void);

int battery_is_charging(void)
{
    return gpio_get_level(BATTERY_CHARGE_GPIO);
}

int battery_is_standby(void)
{
    return gpio_get_level(BATTERY_STANDBY_GPIO) == 0;
}

int battery_get_level(void)
{
    int voltage = battery_get_voltage();
    if (voltage <= battery_voltage_table[0])
    {
        return 0;
    }
    for (int i = 1; i < sizeof(battery_voltage_table) / sizeof(battery_voltage_table[0]); i++)
    {
        if (voltage < battery_voltage_table[i])
        {
            return (i - 1) * 10 + (voltage - battery_voltage_table[i - 1]) * 10
                    / (battery_voltage_table[i] - battery_voltage_table[i - 1]);
        }
    }
    return 100;
}

int battery_get_voltage(void)
{
    int result;
    int voltage;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &result));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, result, &voltage));
    return voltage << 1;
}

void battery_info(void)
{
    ESP_LOGI(TAG, "Battery voltage: %d mV", battery_get_voltage());
    ESP_LOGI(TAG, "Battery is %s", battery_is_charging() ? "charging" : "discharging");
    ESP_LOGI(TAG, "Battery is %s", battery_is_standby() ? "standby" : "active");
    ESP_LOGI(TAG, "Battery level: %d%%", battery_get_level());
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
battery, battery_info, get battery info);

RtAppErr battery_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = BATTERY_ADC_UINT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg));

    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UINT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

    gpio_config_t io_conf;

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << BATTERY_CHARGE_GPIO;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << BATTERY_STANDBY_GPIO;
    gpio_config(&io_conf);

    return RTAM_OK;
}

RtAppErr battery_deinit(void)
{
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(cali_handle));
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = battery_init,
    .stop = battery_deinit,
};

RTAPP_EXPORT(battery, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, NULL, NULL);
