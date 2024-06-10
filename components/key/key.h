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

enum key_action {
    KEY_ACTION_NONE,
    KEY_ACTION_SHORT_PRESS,
    KEY_ACTION_LONG_PRESS
};

struct key_def {
    const char *name;
    uint32_t gpio;
    void (*on_press)(int key, enum key_action action);
};

int key_register_callback(const char *name, void (*on_press)(int key, enum key_action action));

#endif // __KEY_H__
