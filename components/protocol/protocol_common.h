/**
 * @file protocol_common.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief protocol common
 * @version 1.0.0
 * @date 2024-06-12
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __PROTOCOL_COMMON_H__
#define __PROTOCOL_COMMON_H__

#include "misc/lv_color.h"
#include <stdint.h>

struct protocol_pin_map {
    int pin;
    const char *default_name;
    const char *name;
    lv_color_t color;
};

int protocol_reset_pin_map(void);
int protocol_set_pin_map(int pin, const char *name, lv_color_t color);
lv_obj_t* protocol_create_pin_map(lv_obj_t *parent);

#endif /* __PROTOCOL_COMMON_H__ */
