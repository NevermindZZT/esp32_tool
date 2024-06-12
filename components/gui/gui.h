/**
 * @file gui.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief gui
 * @version 1.0.0
 * @date 2024-05-10
 * @copyright (c) 2024 Letter All rights reserved.
 */
#ifndef __GUI_H__
#define __GUI_H__

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#define GUI_MAX_SCREEN_STACK 16

#define GUI_LOAD_RES_FROM_FS 0

#if GUI_LOAD_RES_FROM_FS == 1
#define GUI_APP_ICON(_app) "S:/spiflash/data/theme/default/app/" #_app "/icon.png"
#define GUI_APP_RES_PNG(_app, _res) "S:/spiflash/data/theme/default/app/" #_app "/" #_res ".png"
#else
#define GUI_APP_ICON(_app) &icon_app_##_app
#define GUI_APP_RES_PNG(_app, _res) &icon_app_##_app##_##_res
#endif

extern lv_font_t *source_han_sans_24;

#define gui_back() gui_pop_screen(LV_SCR_LOAD_ANIM_MOVE_RIGHT)
#define gui_home() gui_pop_to_frist(LV_SCR_LOAD_ANIM_FADE_OUT)

BaseType_t gui_lock(void);
void gui_unlock(void);
void gui_push_screen(lv_obj_t *screen, lv_screen_load_anim_t anim_type);
bool gui_pop_screen(lv_screen_load_anim_t anim_type);
bool gui_pop_to_frist(lv_screen_load_anim_t anim_type);
bool gui_is_han_font_loaded(void);

lv_obj_t *gui_create_menu_item(lv_obj_t*parent, lv_color_t bg_color, void *icon, const char *content);
lv_obj_t *gui_create_status_bar(lv_obj_t *parent, bool show_time, char *content);

void gui_fs_init(void);

#endif // __GUI_H__
