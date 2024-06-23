/**
 * @file protocol_common.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief protocol common
 * @version 1.0.0
 * @date 2024-06-12
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "sdkconfig.h"
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

static char pin_inited = 0;

static struct protocol_pin pin_list[] = {
    [1]  = {CONFIG_PROTOCOL_PIN1_IO,  "P1",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [2]  = {CONFIG_PROTOCOL_PIN2_IO,  "P2",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [3]  = {CONFIG_PROTOCOL_PIN3_IO,  "P3",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [4]  = {CONFIG_PROTOCOL_PIN4_IO,  "P4",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [5]  = {CONFIG_PROTOCOL_PIN5_IO,  "P5",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [6]  = {CONFIG_PROTOCOL_PIN6_IO,  "P6",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [7]  = {CONFIG_PROTOCOL_PIN7_IO,  "P7",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [8]  = {CONFIG_PROTOCOL_PIN8_IO,  "P8",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [9]  = {CONFIG_PROTOCOL_PIN9_IO,  "P9",  NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [10] = {CONFIG_PROTOCOL_PIN10_IO, "P10", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [11] = {CONFIG_PROTOCOL_PIN11_IO, "P11", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [12] = {CONFIG_PROTOCOL_PIN12_IO, "P12", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [13] = {CONFIG_PROTOCOL_PIN13_IO, "P13", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [14] = {CONFIG_PROTOCOL_PIN14_IO, "P14", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [15] = {CONFIG_PROTOCOL_PIN15_IO, "P15", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [16] = {CONFIG_PROTOCOL_PIN16_IO, "P16", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [17] = {CONFIG_PROTOCOL_PIN17_IO, "P17", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [18] = {CONFIG_PROTOCOL_PIN18_IO, "P18", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [19] = {CONFIG_PROTOCOL_PIN19_IO, "P19", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)},
    [20] = {CONFIG_PROTOCOL_PIN20_IO, "P20", NULL, LV_COLOR_MAKE(0x9E, 0x9E, 0x9E)}
};

static const int pin_map1[] = {
    20, 18, 16, 14, 12, 10, 8, 6, 4, 2,
};

static const int pin_map2[] = {
    19, 17, 15, 13, 11, 9, 7, 5, 3, 1,
};

int protocol_get_io(int pin)
{
    if (pin < 1 || pin > 20) {
        return -1;
    }
    return pin_list[pin].io;
}

void protocol_init_pin(void)
{
    if (pin_inited) {
        return;
    }
    for (int i = 1; i < sizeof(pin_list) / sizeof(pin_list[0]); i++) {
        if (pin_list[i].io < 0) {
            pin_list[i].color.red = 0x42;
            pin_list[i].color.green = 0x42;
            pin_list[i].color.blue = 0x42;
        } else if (pin_list[i].io == 43 || pin_list[i].io == 44) {
            pin_list[i].color.red = 0xF4;
            pin_list[i].color.green = 0x43;
            pin_list[i].color.blue = 0x36;
        }
        if (pin_list[i].io == -1) {
            pin_list[i].name = "NC";
        } else if (pin_list[i].io == -2) {
            pin_list[i].name = "VCC";
        } else if (pin_list[i].io == -3) {
            pin_list[i].name = "GND";
        } else if (pin_list[i].io == -4) {
            pin_list[i].name = "I+/V";
        } else if (pin_list[i].io == -5) {
            pin_list[i].name = "I-/R";
        } else if (pin_list[i].io == 43) {
            pin_list[i].name = "TX0";
        } else if (pin_list[i].io == 44) {
            pin_list[i].name = "RX0";
        }
    }
    pin_inited = 1;
}

int protocol_reset_pin(void)
{
    for (int i = 1; i < sizeof(pin_list) / sizeof(pin_list[0]); i++) {
        if (pin_list[i].io < 0 || pin_list[i].io == 43 || pin_list[i].io == 44) {
            continue;
        }
        pin_list[i].name = NULL;
        pin_list[i].color.red = 0x9E;
        pin_list[i].color.green = 0x9E;
        pin_list[i].color.blue = 0x9E;
    }
    return 0;

}

int protocol_set_pin(int pin, const char *name, lv_color_t color)
{
    if (pin < 1 || pin > 20) {
        return -1;
    }
    pin_list[pin].name = name;
    pin_list[pin].color.red = color.red;
    pin_list[pin].color.green = color.green;
    pin_list[pin].color.blue = color.blue;
    return 0;
}

int protocol_set_pin_by_io(int io, const char *name, lv_color_t color)
{
    if (io < 0 || io == 43 || io == 44 || strlen(name) > 4) {
        return -1;
    }
    for (int i = 1; i < sizeof(pin_list) / sizeof(pin_list[0]); i++) {
        return protocol_set_pin(i, name, color);
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
    protocol_init_pin();

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
    for (int i = 0; i < sizeof(pin_map1) / sizeof(pin_map1[0]); i++) {
        char name_buf[8];
        const char *name = pin_list[pin_map1[i]].name != NULL ? pin_list[pin_map1[i]].name : pin_list[pin_map1[i]].default_name;
        int len = strlen(name);
        int j = 0;
        for (int i = 0; i < 4; i++) {
            name_buf[i << 1] = 4 - len > i ? ' ' : name[j++];
            name_buf[(i << 1) + 1] = '\n';
        }
        name_buf[7] = 0;
        front = sprotocol_add_pin(pin_group1, front, name_buf, pin_list[pin_map1[i]].color);
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
    for (int i = 0; i < sizeof(pin_map2) / sizeof(pin_map2[0]); i++) {
        char name_buf[8];
        const char *name = pin_list[pin_map2[i]].name != NULL ? pin_list[pin_map2[i]].name : pin_list[pin_map2[i]].default_name;
        int len = strlen(name);
        for (int i = 0; i < 4; i++) {
            name_buf[i << 1] = i < len ? name[i] : ' ';
            name_buf[(i << 1) + 1] = '\n';
        }
        name_buf[7] = 0;
        front = sprotocol_add_pin(pin_group2, front, name_buf, pin_list[pin_map2[i]].color);
    }
    lv_obj_set_width(pin_group2, LV_SIZE_CONTENT);
    lv_obj_set_height(pin_group2, LV_SIZE_CONTENT);

    lv_obj_align_to(pin_group2, pin_group1, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    return pin_layout;
}