/**
 * @file gui_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief gui common
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
 
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style_gen.h"
#include "font/lv_symbol_def.h"
#include "freertos/FreeRTOS.h"
#include "gui.h"
#include "lvgl.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_types.h"
#include "battery.h"


lv_obj_t *gui_create_menu_item(lv_obj_t*parent, lv_color_t bg_color, void *icon, const char *content)
{
    lv_obj_t *button = lv_button_create(parent);

    lv_obj_set_style_bg_color(button, bg_color, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(button, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(button, 8, LV_PART_MAIN);
    lv_obj_set_style_width(button, LV_PCT(100), LV_PART_MAIN);
    lv_obj_set_style_height(button, LV_SIZE_CONTENT, LV_PART_MAIN);

    lv_obj_add_flag(button, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *img = lv_img_create(button);
    lv_img_set_src(img, icon);
    lv_obj_set_align(button, LV_ALIGN_LEFT_MID);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, content);
    if (gui_is_han_font_loaded()) {
        lv_obj_set_style_text_font(label, source_han_sans_24, LV_PART_MAIN);
    }
    lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    return button;
}

lv_obj_t *gui_create_status_bar(lv_obj_t *parent, bool show_time, char *content)
{
    lv_obj_t *status_bar = lv_obj_create(parent);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_pad_hor(status_bar, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(status_bar, 0, LV_PART_MAIN);

    if (content != NULL) {
        lv_obj_t *content_label = lv_label_create(status_bar);
        lv_label_set_text(content_label, content);
        lv_obj_align(content_label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_width(content_label, 160, LV_PART_MAIN);
        lv_obj_set_align(content_label, LV_ALIGN_LEFT_MID);
    }

    lv_obj_t *battery_label = lv_label_create(status_bar);
    int battery_level = battery_get_level();
    char *battery_icon = battery_level > 80 ? LV_SYMBOL_BATTERY_FULL :
                         battery_level > 60 ? LV_SYMBOL_BATTERY_3 :
                         battery_level > 40 ? LV_SYMBOL_BATTERY_2 :
                         battery_level > 20 ? LV_SYMBOL_BATTERY_1 :
                         LV_SYMBOL_BATTERY_EMPTY;
    lv_label_set_text(battery_label, battery_icon);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_align(battery_label, LV_ALIGN_RIGHT_MID);

    if (parent != NULL) {
        lv_obj_align(parent, LV_ALIGN_TOP_MID, 0, 0);
    }

    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_width(status_bar, LV_PCT(100), LV_PART_MAIN);
    lv_obj_set_style_height(status_bar, LV_SIZE_CONTENT, LV_PART_MAIN);
    // lv_obj_set_size(status_bar, 
    //                 lv_display_get_horizontal_resolution(NULL),
    //                 24);

    lv_obj_update_layout(status_bar);

    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    return status_bar;
}
