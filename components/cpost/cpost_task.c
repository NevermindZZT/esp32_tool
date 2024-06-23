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
    size_t process = (size_t)param;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        cpostProcess(process);
    }
}

RtAppErr cpost_init(void)
{
    xTaskCreatePinnedToCore(cpost_task, "cpost0", 4096, (void *)0, 2, NULL, 0);
    xTaskCreatePinnedToCore(cpost_task, "cpost1", 4096, (void *)1, 2, NULL, 1);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = cpost_init,
};

RTAPP_EXPORT(cpost, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, NULL, NULL);