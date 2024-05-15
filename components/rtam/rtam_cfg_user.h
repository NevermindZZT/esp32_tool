/**
 * @file rtam_cfg_user.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief rtos application manager user config
 * @version 0.1
 * @date 2023-10-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __RTAM_CFG_USER_H__
#define __RTAM_CFG_USER_H__

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define RTAM_PRINT(...)                 ESP_LOGI("rtam", __VA_ARGS__)

#define RTAM_WITH_LETTER_SHELL          1

#define RTAM_DELAY(ms)                  vTaskDelay(pdMS_TO_TICKS(ms))

typedef struct rtam_info {
    void *icon;
} RtamInfo;

#endif
