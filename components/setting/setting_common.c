/**
 * @file setting_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief setting common
 * @version 1.0.0
 * @date 2024-05-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_obj_tree.h"
#include "display/lv_display.h"
#include "draw/lv_draw_rect.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "gui.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "rtam.h"
#include "setting.h"
#include "setting_provider.h"
#include "widgets/checkbox/lv_checkbox.h"
#include "widgets/image/lv_image.h"

static const char *TAG = "setting";

static lv_obj_t *setting_create_radio_page(setting_item_config_t *item_configs, const char *title);

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

static void setting_item_event_cb(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    setting_item_config_t *item_config = (setting_item_config_t *) lv_event_get_user_data(event);

    if (gui_is_global_gesture_actived()) {
        return;
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        if (item_config->type == SETTING_ITEM_SWITCH) {
            // swtich item
            bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
            if (setting_set_bool(item_config->key, state) != 0) {
                ESP_LOGE(TAG, "set %s %s failed", item_config->name, state ? "on" : "off");
            }
            ESP_LOGI(TAG, "switch %s %s", item_config->name, state ? "on" : "off");
        }
    } else if (code == LV_EVENT_CLICKED) {
        if (item_config->type == SETTING_ITEM_SUB_PAGE) {
            // sub page item
            gui_push_screen(setting_create_page(NULL, item_config->data, item_config->name), LV_SCR_LOAD_ANIM_MOVE_LEFT);
        } else if (item_config->type == SETTING_ITEM_SWITCH) {
            // switch item
            lv_obj_t *switch_obj = item_config->widget;
            if (switch_obj != NULL) {
                bool state = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
                if (state) {
                    lv_obj_clear_state(switch_obj, LV_STATE_CHECKED);
                } else {
                    lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
                }
                if (setting_set_bool(item_config->key, state ? 0 : 1) != 0) {
                    ESP_LOGE(TAG, "set %s %s failed", item_config->name, state ? "on" : "off");
                }
            }
        } else if (item_config->type == SETTING_ITEM_RADIO) {
            // radio item
            gui_push_screen(setting_create_radio_page(item_config, item_config->name), LV_SCR_LOAD_ANIM_MOVE_LEFT);
        } else if (item_config->type == SETTING_ITEM_CUSTOM) {
            // custom item
            setting_item_custom_get_screen get_screen = (setting_item_custom_get_screen) item_config->data;
            gui_push_screen(get_screen(), LV_SCR_LOAD_ANIM_MOVE_LEFT);
        }
        ESP_LOGI(TAG, "click %s", item_config->name);
    }
}

lv_obj_t *setting_create_item(lv_obj_t *parent, setting_item_config_t *item_config)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_width(item, LV_PCT(100));
    lv_obj_set_height(item, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 8, LV_PART_MAIN);

    lv_obj_t *icon_img = lv_image_create(item);
    lv_image_set_src(icon_img, item_config->icon);
    lv_obj_set_width(icon_img, 48);
    lv_obj_set_height(icon_img, 48);
    lv_obj_align_to(icon_img, item, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *label = lv_label_create(item);
    lv_label_set_text(label, item_config->name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align_to(label, icon_img, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    // switch item
    if (item_config->type == SETTING_ITEM_SWITCH) {
        lv_obj_t *switch_obj = lv_switch_create(item);
        lv_obj_align(switch_obj, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(switch_obj, setting_item_event_cb, LV_EVENT_VALUE_CHANGED, item_config);
        if (setting_get_bool(item_config->key, false)) {
            lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
        }
        item_config->widget = switch_obj;
    }

    lv_obj_add_event_cb(item, setting_item_event_cb, LV_EVENT_CLICKED, item_config);

    return item;
}

lv_obj_t *setting_create_page(lv_obj_t *screen, setting_item_config_t *item_configs, const char *title)
{
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }

    lv_obj_t *menu = lv_menu_create(screen);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(menu);

    // create main page
    lv_obj_t *main_page = lv_menu_page_create(menu, title);

    while (item_configs->name) {
        setting_create_item(main_page, item_configs);
        item_configs++;
    }

    lv_menu_set_page(menu, main_page);

    return screen;
}


static void setting_radio_event_cb(lv_event_t *event)
{
    lv_obj_t *current_target = lv_event_get_current_target(event);
    lv_obj_t *obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    setting_item_config_t *item_config = (setting_item_config_t *) lv_event_get_user_data(event);

    if (gui_is_global_gesture_actived()) {
        return;
    }

    if (current_target == obj) {
        return;
    }

    const char **item_data = item_config->data;

    if (code == LV_EVENT_CLICKED) {
        for (int i =  0; i < lv_obj_get_child_count(item_config->widget); i++) {
            lv_obj_t *radio = lv_obj_get_child(item_config->widget, i);
            if (radio == obj) {
                setting_set_str(item_config->key, item_data[i]);
                lv_obj_add_state(radio, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(radio, LV_STATE_CHECKED);
            }
        }
    }

}

lv_obj_t *setting_create_radio_page(setting_item_config_t *item_config, const char *title)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *menu = lv_menu_create(screen);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(menu);

    // create main page
    lv_obj_t *main_page = lv_menu_page_create(menu, title);

    const char **item_data = item_config->data;

    while (*item_data) {
        lv_obj_t *radio = lv_checkbox_create(main_page);
        lv_obj_set_style_bg_color(radio, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(radio, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(radio, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(radio, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(radio, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(radio, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
        lv_obj_set_style_bg_image_src(radio, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_flag(radio, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_checkbox_set_text(radio, *item_data);
        if (strcmp(setting_get_str(item_config->key, ""), *item_data) == 0) {
            lv_obj_add_state(radio, LV_STATE_CHECKED);
        }

        item_data++;
    }
    item_config->widget = main_page;
    lv_obj_add_event_cb(main_page, setting_radio_event_cb, LV_EVENT_CLICKED, item_config);

    lv_menu_set_page(menu, main_page);

    return screen;
} 