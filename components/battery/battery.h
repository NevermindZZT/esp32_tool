/**
 * @file battery.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief battery service
 * @version 1.0.0
 * @date 2024-06-08
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __BATTERY_H__
#define __BATTERY_H__

int battery_is_charging(void);
int battery_is_standby(void);
int battery_get_level(void);
int battery_get_voltage(void);

#endif /* __BATTERY_H__ */
