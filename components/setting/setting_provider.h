/**
 * @file setting_provicer.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting provider 
 * @version 1.0.0
 * @date 2024-07-13
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __SETTING_PROVIDER_H__
#define __SETTING_PROVIDER_H__

#include "stdbool.h"

#define SETTING_NVS_NAMESPACE "setting"

#define SETTING_KEY_WIFI_ENABLED "wifi_enabled"
#define SETTING_KEY_BT_ENABLED "bt_enabled"
#define SETTING_KEY_SCR_BRIGHT "scr_bright"


int setting_get(const char *key, int def);
int setting_set(const char *key, int value);
char *setting_get_str(const char *key, char *def);
int setting_set_str(const char *key, const char *value);
bool setting_get_bool(const char *key, bool def);
int setting_set_bool(const char *key, bool value);

#endif
