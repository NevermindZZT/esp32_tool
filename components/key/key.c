/**
 * @file key.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief key
 * @version 1.0.0
 * @date 2024-06-09
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <string.h>
#include "esp_intr_alloc.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "rtam.h"
#include <stdint.h>
#include "key.h"

#define BOOT_KEY_PIN GPIO_NUM_0

static const char* TAG = "key";

static int rt_app_status = RTAPP_STATUS_STOPPED;
static QueueHandle_t gpio_evt_queue = NULL;

static struct key_def keys[] = {
    {"power", BOOT_KEY_PIN, NULL},
};

int key_register_callback(const char *name, void (*on_press)(int key, enum key_action action))
{
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (strcmp(keys[i].name, name) == 0) {
            keys[i].on_press = on_press;
            return 0;
        }
    }
    return -1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    struct key_def *key = arg;
    gpio_isr_handler_remove(BOOT_KEY_PIN);
    xQueueSendFromISR(gpio_evt_queue, &key, NULL);
}

static enum key_action get_key(uint32_t gpio)
{
    int i = 0;
    while (gpio_get_level(gpio) == 0 && i < 200) {
        vTaskDelay(pdMS_TO_TICKS(10));
        i++;
    }
    if (i < 2) {
        return KEY_ACTION_NONE;
    } else {
        if (i <= 100) {
            return KEY_ACTION_SHORT_PRESS;
        } else {
            return KEY_ACTION_LONG_PRESS;
        }
    }
    return KEY_ACTION_NONE;
}

static int wait_key_release(uint32_t gpio)
{
    while (gpio_get_level(gpio) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0;
}

static void key_task(void* arg)
{
    struct key_def *key;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &key, portMAX_DELAY)) {
            int action = get_key(key->gpio);
            if (key->on_press) {
                key->on_press(key->gpio, action);
            }
            wait_key_release(key->gpio);
            gpio_isr_handler_add(BOOT_KEY_PIN, gpio_isr_handler, key);
        }
    }
}

static int key_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << keys[i].gpio;
        gpio_config(&io_conf);
    }

    gpio_install_isr_service(0);

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        gpio_isr_handler_add(keys[i].gpio, gpio_isr_handler, (void*) &keys[i]);
    }

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(key_task, "key_task", 2048, NULL, 10, NULL);

    rt_app_status = RTAPP_STATUS_RUNNING;
    return 0;
}

static int storage_get_status(void)
{
    return rt_app_status;
}
RTAPP_EXPORT(key, key_init, NULL, storage_get_status, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, NULL, NULL);
