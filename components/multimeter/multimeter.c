/**
 * @file multimeter.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief multimeter
 * @version 1.0.0
 * @date 2024-07-14
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style_gen.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "draw/lv_image_dsc.h"
#include "gui.h"
#include "hal/gpio_types.h"
#include "ina226.h"
#include "launcher.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "rtam.h"
#include "rtam_cfg_user.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "widgets/arc/lv_arc.h"
#include "widgets/bar/lv_bar.h"
#include "widgets/label/lv_label.h"

#define MULTIMETER_POWER_CONNECT_IO 11

#define MULTIMETER_VOLTAGE_MAX      40000
#define MULTIMETER_CURRENT_LSB      0.001
#define MULTIMETER_SHUNT_RESISTOR   0.005
#define MUTLIMETER_CURRENT_MAX      (MULTIMETER_CURRENT_LSB * 32768)
#define MUTLIMETER_POWER_MAX        (MULTIMETER_VOLTAGE_MAX * MUTLIMETER_CURRENT_MAX)
#define MULTIMETER_RESISTOR_MAX     (3.3 / MULTIMETER_CURRENT_LSB)

#define MUTLIMETER_TYPE_VOLTAGE     0
#define MUTLIMETER_TYPE_CURRENT     1
#define MUTLIMETER_TYPE_POWER       2
#define MUTLIMETER_TYPE_RESISTOR    3

static const char *TAG = "multimeter";

static bool run = false;
static lv_obj_t *screen = NULL;
static lv_obj_t *value_arc = NULL;
static lv_obj_t *value_label = NULL;
static lv_obj_t *type_label = NULL;

static uint8_t type = MUTLIMETER_TYPE_VOLTAGE;

static lv_obj_t* multimeter_get_screen(void);
static void multimeter_set_type(uint8_t t);

static int multimeter_gesture_callback(lv_dir_t dir)
{
    if (dir == LV_DIR_RIGHT) {
        if (lv_screen_active() == multimeter_get_screen()) {
            rtamTerminate("multimeter");
        } else {
            gui_back();
        }
        return 0;
    } else if (dir == LV_DIR_BOTTOM || dir == LV_DIR_TOP) {
        type += dir == LV_DIR_BOTTOM ? 1 : -1;
        if (type > MUTLIMETER_TYPE_RESISTOR) {
            type = MUTLIMETER_TYPE_VOLTAGE;
        } else if (type < MUTLIMETER_TYPE_VOLTAGE) {
            type = MUTLIMETER_TYPE_RESISTOR;
        }
        multimeter_set_type(type);
    }
    return -1;
}

static void multimeter_set_type(uint8_t t)
{
    type = t;
    if (type_label != NULL) {
        gui_lock();
        gpio_set_level(MULTIMETER_POWER_CONNECT_IO, 0);
        if (type == MUTLIMETER_TYPE_VOLTAGE) {
            lv_label_set_text(type_label, "Voltage");
        } else if (type == MUTLIMETER_TYPE_CURRENT) {
            lv_label_set_text(type_label, "Current");
        } else if (type == MUTLIMETER_TYPE_POWER) {
            lv_label_set_text(type_label, "Power");
        } else if (type == MUTLIMETER_TYPE_RESISTOR) {
            lv_label_set_text(type_label, "Resistor");
            gpio_set_level(MULTIMETER_POWER_CONNECT_IO, 1);
        }
        lv_obj_align_to(type_label, value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 70);
        gui_unlock();
    }
}

static lv_obj_t* multimeter_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static void multimeter_init_screen(void)
{
    lv_obj_t *scr = multimeter_get_screen();

    value_arc = lv_arc_create(scr);
    lv_obj_set_size(value_arc, 220, 220);
    lv_arc_set_rotation(value_arc, 135);
    lv_arc_set_bg_angles(value_arc, 0, 270);
    lv_arc_set_value(value_arc, 100);
    lv_obj_remove_style(value_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(value_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(value_arc);

    value_label = lv_label_create(scr);
    lv_obj_center(value_label);
    lv_obj_set_style_text_font( value_label, &lv_font_montserrat_24, LV_PART_MAIN);

    type_label = lv_label_create(scr);
}

static void multimeter_task(void *arg)
{
    multimeter_set_type(MUTLIMETER_TYPE_VOLTAGE);
    while (run)
    {
        if (value_label)
        {
            int voltage = ina226_read_voltage();
            int current = ina226_read_current();
            int power = ina226_read_power();
            int shunt = ina226_read_shunt_voltage();
            // int current = (int)((float)shunt / MULTIMETER_SHUNT_RESISTOR / 1000);
            // int power = voltage * current;
            if (type == MUTLIMETER_TYPE_VOLTAGE) {
                gui_lock();
                lv_label_set_text_fmt(value_label, "%d mV", voltage);
                lv_arc_set_value(value_arc, (voltage * 100) / MULTIMETER_VOLTAGE_MAX);
                gui_unlock();
            } else if (type == MUTLIMETER_TYPE_CURRENT) {
                gui_lock();
                lv_label_set_text_fmt(value_label, "%d mA", current);
                lv_arc_set_value(value_arc, (current * 100) / MUTLIMETER_CURRENT_MAX);
                gui_unlock();
            } else if (type == MUTLIMETER_TYPE_POWER) {
                gui_lock();
                lv_label_set_text_fmt(value_label, "%d mW", power);
                lv_arc_set_value(value_arc, (power * 100) / MUTLIMETER_POWER_MAX);
                gui_unlock();
            } else if (type == MUTLIMETER_TYPE_RESISTOR) {
                gui_lock();
                int resistor = 0;
                if (current != 0) {
                    resistor = voltage / current;
                    lv_label_set_text_fmt(value_label, "%d Ohm", resistor);
                    lv_arc_set_value(value_arc, (resistor * 100) / MULTIMETER_RESISTOR_MAX);
                } else {
                    lv_label_set_text_fmt(value_label, "- Ohm");
                    lv_arc_set_value(value_arc, 100);
                }
                gui_unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelete(NULL);
}

static ShellCommand multimeter_group[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, dr, ina226_debug_read,
        reset\r\nmultimeter reset),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, dw, ina226_debug_write,
        reset\r\nmultimeter reset),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, reset, ina226_reset,
        reset\r\nmultimeter reset),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, id, ina266_read_id,
        read voltage\r\nmultimeter voltage),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, shunt, ina226_read_shunt_voltage, 
        read voltage\r\nmultimeter shunt voltage),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, voltage, ina226_read_voltage, 
        read voltage\r\nmultimeter voltage),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, current, ina226_read_current, 
        read voltage\r\nmultimeter current),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, power, ina226_read_power, 
        read voltage\r\nmultimeter power),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
multimeter, multimeter_group, multimeter);

static RtAppErr multimeter_suspend(void)
{
    run = false;
    gui_set_global_gesture_callback(NULL);
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static RtAppErr multimeter_resume(void)
{
    multimeter_init_screen();
    gui_push_screen(multimeter_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    gui_set_global_gesture_callback(multimeter_gesture_callback);
    run = true;
    xTaskCreate(multimeter_task, "multimeterTask", 2048, NULL, 1, NULL);
    return RTAM_OK;
}

static RtAppErr multimeter_init(void)
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << MULTIMETER_POWER_CONNECT_IO;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    ina226_reset();
    ina226_init(MULTIMETER_CURRENT_LSB, MULTIMETER_SHUNT_RESISTOR);
    return RTAM_OK;
}

static RtAppErr multimeter_stop(void)
{
    multimeter_set_type(MUTLIMETER_TYPE_VOLTAGE);
    gpio_reset_pin(MULTIMETER_POWER_CONNECT_IO);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .suspend = multimeter_suspend,
    .resume = multimeter_resume,
    .start = multimeter_init,
    .stop = multimeter_stop,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]) {
        "gui",
        "launcher",
        NULL
    }
};

extern const lv_image_dsc_t icon_app_multimeter;
static const RtamInfo multimeter_info = {
    .label = "multimeter",
    .icon = (void *) GUI_APP_ICON(multimeter),
};

RTAPP_EXPORT(multimeter, &interface, RTAPP_FLAG_BACKGROUND, &dependencies, &multimeter_info);
