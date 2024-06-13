/**
 * @file protocol_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief protocol common
 * @version 1.0.0
 * @date 2024-06-12
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "string.h"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "esp_system.h"
#include "esp_log.h"
#include "layouts/lv_layout.h"
#include "lvgl.h"
#include "gui.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "protocol_common.h"
#include "stdlib/lv_sprintf.h"

static const char *TAG = "protocol_common";

static struct protocol_pin_map pin_map_1[] = {
    {43, "TX0",  NULL, LV_COLOR_MAKE(0xF4, 0x43, 0x36)},
    {44, "RX0",  NULL, LV_COLOR_MAKE(0xF4, 0x43, 0x36)},
    {42, "IO42", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {41, "IO41", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {40, "IO40", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {39, "IO39", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {38, "IO38", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {37, "IO37", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {36, "IO36", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {-1, "VCC",  NULL, LV_COLOR_MAKE(0x42, 0x42, 0x42)},
};
static struct protocol_pin_map pin_map_2[] = {
    {47, "IO47", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {21, "IO21", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {12, "IO12", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {11, "IO11", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {10, "IO10", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {9,  "IO9",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {35, "IO35", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {45, "IO45", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {48, "IO48", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    {-1, "GND",  NULL, LV_COLOR_MAKE(0x42, 0x42, 0x42)},
};


int protocol_reset_pin_map(void)
{
    for (int i = 0; i < sizeof(pin_map_1) / sizeof(pin_map_1[0]); i++) {
        if (pin_map_1[i].pin != 43 && pin_map_1[i].pin != 44 && pin_map_1[i].pin != -1) {
            pin_map_1[i].name = NULL;
            pin_map_1[i].color.red = 0x9E;
            pin_map_1[i].color.green = 0x9E;
            pin_map_1[i].color.blue = 0x9E;
        }
    }
    for (int i = 0; i < sizeof(pin_map_2) / sizeof(pin_map_2[0]); i++) {
        if (pin_map_2[i].pin != 47 && pin_map_2[i].pin != 21 && pin_map_2[i].pin != -1) {
            pin_map_2[i].name = NULL;
            pin_map_2[i].color.red = 0x9E;
            pin_map_2[i].color.green = 0x9E;
            pin_map_2[i].color.blue = 0x9E;
        }
    }
    return 0;

}

int protocol_set_pin_map(int pin, const char *name, lv_color_t color)
{
    if (pin < 0 || pin == 43 || pin == 44 || strlen(name) > 4) {
        return -1;
    }
    for (int i = 0; i < sizeof(pin_map_1) / sizeof(pin_map_1[0]); i++) {
        if (pin_map_1[i].pin == pin) {
            pin_map_1[i].name = name;
            pin_map_1[i].color = color;
            return 0;
        }
    }
    for (int i = 0; i < sizeof(pin_map_2) / sizeof(pin_map_2[0]); i++) {
        if (pin_map_2[i].pin == pin) {
            pin_map_2[i].name = name;
            pin_map_2[i].color = color;
            return 0;
        }
    }
    return -1;
}

static lv_obj_t* sprotocol_add_pin(lv_obj_t *parent, lv_obj_t *front, const char *name, lv_color_t color)
{
    
    lv_obj_t *pin_label = lv_label_create(parent);
    lv_label_set_text(pin_label, name);
    lv_obj_set_width(pin_label, 23);
    lv_obj_set_style_bg_color(pin_label, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pin_label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pin_label, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_label, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(pin_label, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(pin_label, &lv_font_montserrat_16, LV_PART_MAIN);

    if (front != NULL) {
        lv_obj_align_to(pin_label, front, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
    }
    return pin_label;
}


lv_obj_t* protocol_create_pin_map(lv_obj_t *parent)
{
    lv_obj_t *pin_layout = lv_obj_create(parent);
    lv_obj_set_size(pin_layout, lv_display_get_horizontal_resolution(NULL), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(pin_layout, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(pin_layout, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(pin_layout, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pin_layout, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_layout, 0, LV_PART_MAIN);

    lv_obj_t *pin_group1 = lv_obj_create(pin_layout);
    lv_obj_set_style_pad_all(pin_group1, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(pin_group1, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(pin_group1, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pin_group1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_group1, 0, LV_PART_MAIN);

    lv_obj_t *front = NULL;
    for (int i = 0; i < sizeof(pin_map_1) / sizeof(pin_map_1[0]); i++) {
        char name_buf[8];
        const char *name = pin_map_1[i].name != NULL ? pin_map_1[i].name : pin_map_1[i].default_name;
        int len = strlen(name);
        int j = 0;
        for (int i = 0; i < 4; i++) {
            name_buf[i << 1] = 4 - len > i ? ' ' : name[j++];
            name_buf[(i << 1) + 1] = '\n';
        }
        name_buf[7] = 0;
        front = sprotocol_add_pin(pin_group1, front, name_buf, pin_map_1[i].color);
    }
    lv_obj_set_width(pin_group1, LV_SIZE_CONTENT);
    lv_obj_set_height(pin_group1, LV_SIZE_CONTENT);

    lv_obj_t *pin_group2 = lv_obj_create(pin_layout);
    lv_obj_set_style_pad_all(pin_group2, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(pin_group2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(pin_group2, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pin_group2, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pin_group2, 0, LV_PART_MAIN);

    front = NULL;
    for (int i = 0; i < sizeof(pin_map_2) / sizeof(pin_map_2[0]); i++) {
        char name_buf[8];
        const char *name = pin_map_2[i].name != NULL ? pin_map_2[i].name : pin_map_2[i].default_name;
        int len = strlen(name);
        for (int i = 0; i < 4; i++) {
            name_buf[i << 1] = i < len ? name[i] : ' ';
            name_buf[(i << 1) + 1] = '\n';
        }
        name_buf[7] = 0;
        front = sprotocol_add_pin(pin_group2, front, name_buf, pin_map_2[i].color);
    }
    lv_obj_set_width(pin_group2, LV_SIZE_CONTENT);
    lv_obj_set_height(pin_group2, LV_SIZE_CONTENT);

    lv_obj_align_to(pin_group2, pin_group1, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    return pin_layout;
}