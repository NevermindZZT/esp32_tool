/**
 * @file setting_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting common
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "display/lv_display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "gui.h"
#include "misc/lv_area.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "rtam.h"
#include "setting.h"

static const char *tag = "setting";

int setting_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == setting_get_screen()) {
            rtamTerminate("setting");
        } else {
            gui_back();
        }
        return 0;
    }
    return -1;
}

lv_obj_t *setting_create_text(lv_obj_t *parent, const void *icon, const char *txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_image_create(obj);
        lv_image_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

lv_obj_t *setting_create_slider(lv_obj_t *parent, const void *icon, const char *txt,
                                int32_t min, int32_t max, int32_t val, lv_event_cb_t event_cb)
{
    lv_obj_t * obj = setting_create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t * slider = gui_create_slider(obj, LV_PALETTE_BLUE, 8);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_ON);
    // lv_obj_set_width(slider, 20/*lv_display_get_horizontal_resolution(NULL) - 108*/);
    // lv_obj_set_style_pad_right(parent, 16, LV_PART_MAIN);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);

    lv_obj_add_event_cb(slider, event_cb, LV_EVENT_VALUE_CHANGED, slider);
    lv_obj_add_event_cb(slider, event_cb, LV_EVENT_RELEASED, slider);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    return obj;
}

