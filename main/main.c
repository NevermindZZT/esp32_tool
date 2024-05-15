/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "rtam.h"
#include "shell_port.h"
#include "esp_task_wdt.h"

extern void exportKeep(void);

static void start_task(void *param)
{
    rtamInit();
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());

    // userShellInit();
    exportKeep();
    xTaskCreate(start_task, "start", 4096, NULL, 1, NULL);
}
