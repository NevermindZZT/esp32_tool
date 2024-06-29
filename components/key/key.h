/**
 * @file key.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief key
 * @version 1.0.0
 * @date 2024-06-09
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __KEY_H__
#define __KEY_H__

#include "esp_system.h"
#include "key.h"

enum key_code {
    KEY_CODE_POWER = 0,
    KEY_CODE_BOOT = 1,
};

enum key_action {
    KEY_ACTION_NONE,
    KEY_ACTION_SHORT_PRESS,
    KEY_ACTION_LONG_PRESS
};

struct key_def {
    enum key_code code;
    const char *name;
    uint32_t gpio;
    bool invert;
    struct key_callbacks *callbacks;
    uint32_t last_press_time;
};

struct key_callbacks {
    int (*on_press)(enum key_code code, enum key_action action);
    struct key_callbacks *next;
};

int key_add_callback(enum key_code code, int (*on_press)(int key, enum key_action action));

int key_remove_callback(enum key_code code, int (*on_press)(int key, enum key_action action));

#endif // __KEY_H__
