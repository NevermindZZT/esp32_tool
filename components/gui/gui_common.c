/**
 * @file gui_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief gui common
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
 
#include "core/lv_obj.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "misc/lv_color.h"
#include "misc/lv_types.h"


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
    lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    return button;
}
