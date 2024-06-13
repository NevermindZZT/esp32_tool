/**
 * @file cpost_task.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief cpost task
 * @version 1.0.0
 * @date 2024-06-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "freertos/FreeRTOS.h"
#include "cpost.h"
#include "freertos/projdefs.h"
#include "rtam.h"

void cpost_task(void *param)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        cpostProcess();
    }
}

int cpost_init(void)
{
    xTaskCreate(cpost_task, "cpost", 4096, NULL, 2, NULL);
    return 0;
}
RTAPP_EXPORT(cpost, cpost_init, NULL, NULL, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, NULL, NULL);