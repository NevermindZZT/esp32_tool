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

RtAppErr cpost_init(void)
{
    xTaskCreate(cpost_task, "cpost", 4096, NULL, 2, NULL);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = cpost_init,
};

RTAPP_EXPORT(cpost, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, NULL, NULL);