/**
 * @file cpost_user_config.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief cpost user config
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __CPOST_USER_CONFIG_H__
#define __CPOST_USER_CONFIG_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief 最大handler数量
 */
#define     CPOST_MAX_HANDLER_SIZE      16

/**
 * @brief 获取系统tick函数
 */
#define     CPOST_GET_TICK()            xTaskGetTickCount()

/**
 * @brief tick最大值
 */
#define     CPOST_MAX_TICK              0xFFFFFFFF

#endif /* __CPOST_USER_CONFIG_H__ */
