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
#include "sdkconfig.h"

#ifndef CONFIG_KEY_POWER_PIN
    #define CONFIG_KEY_POWER_PIN  -1
#endif

#ifndef CONFIG_KEY_POWER_PIN_INVERT
    #define CONFIG_KEY_POWER_PIN_INVERT  0
#endif

#ifndef CONFIG_KEY_BOOT_PIN
    #define CONFIG_KEY_BOOT_PIN  -1
#endif

#ifndef CONFIG_KEY_BOOT_PIN_INVERT
    #define CONFIG_KEY_BOOT_PIN_INVERT  0
#endif

static const char* TAG = "key";

static QueueHandle_t gpio_evt_queue = NULL;

static struct key_def keys[] = {
    {KEY_CODE_POWER, "power", CONFIG_KEY_POWER_PIN, CONFIG_KEY_POWER_PIN_INVERT, NULL},
    {KEY_CODE_BOOT,   "boot",  CONFIG_KEY_BOOT_PIN,  CONFIG_KEY_BOOT_PIN_INVERT, NULL},
};

int key_add_callback(enum key_code code, int (*on_press)(int key, enum key_action action))
{
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keys[i].code == code) {
            struct key_callbacks *callbacks = keys[i].callbacks;
            struct key_callbacks *new_callbacks = malloc(sizeof(struct key_callbacks));
            if (!new_callbacks) {
                return -1;
            }
            new_callbacks->on_press = on_press;
            new_callbacks->next = callbacks;
            keys[i].callbacks = new_callbacks;
            return 0;
        }
    }
    return -1;
}

int key_remove_callback(enum key_code code, int (*on_press)(int key, enum key_action action))
{
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keys[i].code == code) {
            struct key_callbacks *callbacks = keys[i].callbacks;
            struct key_callbacks *prev_callbacks = NULL;
            while (callbacks) {
                if (callbacks->on_press == on_press) {
                    if (prev_callbacks) {
                        prev_callbacks->next = callbacks->next;
                    } else {
                        keys[i].callbacks = callbacks->next;
                    }
                    free(callbacks);
                    return 0;
                }
                prev_callbacks = callbacks;
                callbacks = callbacks->next;
            }
        }
    }
    return -1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    struct key_def *key = arg;
    key->last_press_time = esp_log_timestamp();
    gpio_isr_handler_remove(key->gpio);
    xQueueSendFromISR(gpio_evt_queue, &key, NULL);
}

static enum key_action get_key(struct key_def *key)
{
    while (gpio_get_level(key->gpio) == key->invert ? 1 : 0
            && esp_log_timestamp - key->last_press_time < 2000) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    uint32_t time = esp_log_timestamp() - key->last_press_time;
    if (time < 5) {
        return KEY_ACTION_NONE;
    } else {
        if (time <= 1000) {
            return KEY_ACTION_SHORT_PRESS;
        } else {
            return KEY_ACTION_LONG_PRESS;
        }
    }
    return KEY_ACTION_NONE;
}

static int wait_key_release(struct key_def *key)
{
    while (gpio_get_level(key->gpio) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0;
}

static void key_task(void* arg)
{
    struct key_def *key;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &key, portMAX_DELAY)) {
            int action = get_key(key);
            ESP_LOGI(TAG, "key %s pressed, action %d", key->name, action);
            for (struct key_callbacks *callbacks = key->callbacks; callbacks; callbacks = callbacks->next) {
                if (callbacks->on_press) {
                    if (callbacks->on_press(key->code, action) == 0) {
                        break;
                    }
                }
            }
            wait_key_release(key);
            gpio_isr_handler_add(key->gpio, gpio_isr_handler, key);
        }
    }
}

static RtAppErr key_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    gpio_install_isr_service(0);

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (keys[i].gpio > 0) {
            io_conf.pin_bit_mask = 1ULL << keys[i].gpio;
            if (keys[i].invert) {
                io_conf.intr_type = GPIO_INTR_POSEDGE;
                io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
                io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            } else {
                io_conf.intr_type = GPIO_INTR_NEGEDGE;
                io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
                io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            }
            gpio_config(&io_conf);

            gpio_isr_handler_add(keys[i].gpio, gpio_isr_handler, (void*) &keys[i]);
        }
    }

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(key_task, "key_task", 8192, NULL, 10, NULL);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = key_init,
};

RTAPP_EXPORT(key, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, NULL, NULL);
