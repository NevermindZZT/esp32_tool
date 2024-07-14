/**
 * @file setting_provider.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting provider
 * @version 1.0.0
 * @date 2024-07-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "cpost.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "setting_provider.h"

static const char *tag = "setting_provicer";

static nvs_handle_t setting_nvs_handle = 0;

static uint32_t setting_get_nvs_handle(void)
{
    if (setting_nvs_handle == 0) {
        nvs_flash_init();
        nvs_open(SETTING_NVS_NAMESPACE, NVS_READWRITE, &setting_nvs_handle);
    }
    return setting_nvs_handle;
}

int setting_get(const char *key, int def)
{
    int32_t value = def;
    if (nvs_get_i32(setting_get_nvs_handle(), key, &value) == ESP_OK) {
        return value;
    }
    return def;
}

int setting_set(const char *key, int value)
{
    if (nvs_set_i32(setting_get_nvs_handle(), key, value) == ESP_OK) {
        cpost(0, nvs_commit, (void *) setting_get_nvs_handle(), .delay=50, .attrs.flag=CPOST_FLAG_CANCEL_CURRENT);
        return 0;
    }
    return -1;
}

char *setting_get_str(const char *key, char *def)
{
    size_t required_size = 0;
    if (nvs_get_str(setting_get_nvs_handle(), key, NULL, &required_size) == ESP_OK) {
        char *value = heap_caps_malloc(required_size, MALLOC_CAP_DEFAULT);
        if (nvs_get_str(setting_get_nvs_handle(), key, value, &required_size) == ESP_OK) {
            return value;
        }
        heap_caps_free(value);
    }
    return def;
}

int setting_set_str(const char *key, const char *value)
{
    if (nvs_set_str(setting_get_nvs_handle(), key, value) == ESP_OK) {
        cpost(0, nvs_commit, (void *) setting_get_nvs_handle(), .delay=50, .attrs.flag=CPOST_FLAG_CANCEL_CURRENT);
        return 0;
    }
    return -1;
}

bool setting_get_bool(const char *key, bool def)
{
    int8_t value = def;
    if (nvs_get_i8(setting_get_nvs_handle(), key, &value) == ESP_OK) {
        return value;
    }
    return def;
}

int setting_set_bool(const char *key, bool value)
{
    if (nvs_set_i8(setting_get_nvs_handle(), key, value) == ESP_OK) {
        cpost(0, nvs_commit, (void *) setting_get_nvs_handle(), .delay=50, .attrs.flag=CPOST_FLAG_CANCEL_CURRENT);
        return 0;
    }
    return -1;
}
