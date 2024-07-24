/**
 * @file screensaver.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief screensaver
 * @version 1.0.0
 * @date 2024-07-23
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SCREENSAVER_H__
#define __SCREENSAVER_H__

#include "lvgl.h"

struct screensaver_config {
    const char *name;
    lv_obj_t *(*get_screen)(void);
    void (*task)(void*);
};

bool screensaver_running(void);

lv_obj_t *simple_time_get_screen(void);

#endif