/**
 * @file pwm.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief pwm
 * @version 1.0.0
 * @date 2024-06-23
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_rom_gpio.h"
#include "hal/ledc_types.h"
#include "lvgl.h"
#include "launcher.h"
#include "draw/lv_image_dsc.h"
#include "gui.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_event.h"
#include "misc/lv_text.h"
#include "protocol_common.h"
#include "rtam.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "soc/clk_tree_defs.h"
#include "soc/ledc_periph.h"
#include "stdlib/lv_sprintf.h"
#include "widgets/label/lv_label.h"
#include "widgets/slider/lv_slider.h"

#define PWM_GROUP_NUM           2
#define PWM_GROUP_CHANNEL_NUM   3
#define PWM_DUTY_RESOLUTION     LEDC_TIMER_10_BIT

#define PWM_DUTY_MAX            (1 << PWM_DUTY_RESOLUTION)


struct pwm_group {
    ledc_timer_t timer;
    lv_obj_t *freq_content;
    lv_obj_t *freq_slider;
    struct {
        ledc_channel_t channel;
        int pin;
        lv_obj_t *duty_content;
        lv_obj_t *duty_slider;
    } pwm_cfg[PWM_GROUP_CHANNEL_NUM];
};

struct pwm_group pwm_group[PWM_GROUP_NUM] = {
    {
        .timer = 1,
        .pwm_cfg = {
            {LEDC_CHANNEL_1, 16},
            {LEDC_CHANNEL_2, 14},
            {LEDC_CHANNEL_3, 12},
        }
    },
    {
        .timer = 2,
        .pwm_cfg = {
            {LEDC_CHANNEL_1, 15},
            {LEDC_CHANNEL_2, 13},
            {LEDC_CHANNEL_3, 11},
        }
    }
};

static const char *TAG = "pwm";

static lv_obj_t *screen = NULL;

static lv_obj_t* pwm_get_screen(void);
static int pwm_set_freq(int group, int freq);
static int pwm_set_duty(int group, int index, int duty);

void pwm_global_event_cb(lv_event_t *event)
{
    // static bool gesture_enabled = false;
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target(event);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT /*&& gesture_enabled*/) {
            // gesture_enabled = false;
            if (obj == pwm_get_screen()) {
                rtamTerminate("pwm");
            } else {
                gui_back();
            }
            lv_indev_wait_release(lv_indev_active());
        } else if (dir == LV_DIR_BOTTOM) {
            if (obj == pwm_get_screen()) {
                // gui_push_screen(serial_debug_create_pin_map_screen(), LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
            }
        } else if (dir == LV_DIR_TOP) {
            if (obj != pwm_get_screen()) {
                // gui_pop_screen(LV_SCR_LOAD_ANIM_MOVE_TOP);
            }
        }
    } else if (code == LV_EVENT_PRESSED) {
        // lv_point_t point;
        // lv_indev_get_point(lv_indev_active(), &point);
        // if (point.x < 16) {
        //     gesture_enabled = true;
        // }
    } else if (code == LV_EVENT_RELEASED) {
        // gesture_enabled = false;
    }
}

void pwm_slider_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target(event);

    if (code == LV_EVENT_VALUE_CHANGED) {
        for (int i = 0; i < PWM_GROUP_NUM; i++)
        {
            if (obj == pwm_group[i].freq_slider) {
                int freq = lv_slider_get_value(obj);
                pwm_set_freq(i, freq);
            } else {
                for (int j = 0; j < PWM_GROUP_CHANNEL_NUM; j++)
                {
                    if (obj == pwm_group[i].pwm_cfg[j].duty_slider) {
                        int duty = lv_slider_get_value(obj);
                        pwm_set_duty(i, j, duty);
                    }
                }
            }
        }   
    }
}

static lv_obj_t* pwm_get_screen(void)
{
    if (!screen)
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    return screen;
}

static esp_err_t pwm_init_channel(int group_id, int index)
{
    int io = protocol_get_io(pwm_group[group_id].pwm_cfg[index].pin);
    ESP_LOGI(TAG, "pwm%d: io: %d", index, io);
    ledc_channel_config_t ledc_conf = {
        .channel = pwm_group[group_id].pwm_cfg[index].channel,
        .duty = 0,
        .gpio_num = io,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = pwm_group[group_id].timer,
        .intr_type = LEDC_INTR_DISABLE,
        .hpoint = 0,
    };
    esp_err_t ret = ledc_channel_config(&ledc_conf);
    // esp_rom_gpio_connect_out_signal(io,
    //                                 ledc_periph_signal[LEDC_LOW_SPEED_MODE].sig_out0_idx + pwm_cfgs[index].channel,
    //                                 0,
    //                                 0);
    return ret;
}

static void pwm_deinit_channel(int group_id, int index)
{
    ledc_stop(LEDC_LOW_SPEED_MODE, pwm_group[group_id].pwm_cfg[index].channel, 0);
    gpio_reset_pin(protocol_get_io(pwm_group[group_id].pwm_cfg[index].pin));
}

static esp_err_t pwm_init_group(int group_id)
{
    ledc_timer_config_t timer_conf = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = pwm_group[group_id].timer,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    for (int i = 0; i < PWM_GROUP_CHANNEL_NUM; i++)
    {
        ESP_ERROR_CHECK(pwm_init_channel(group_id, i));
    }
    return ESP_OK;
}

static esp_err_t pwm_deinit_timer(int group_id)
{
    for (int i = 0; i < PWM_GROUP_CHANNEL_NUM; i++)
    {
        pwm_deinit_channel(group_id, i);
    }

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = pwm_group[group_id].timer,
        .deconfigure = true
    };
    ledc_timer_pause(LEDC_LOW_SPEED_MODE, pwm_group[group_id].timer);
    return ledc_timer_config(&timer_conf);
}

static int pwm_set_freq(int group, int freq)
{
    if (group < 0 || group > PWM_GROUP_NUM) {
        return -1;
    }
    if (freq <= 0) {
        return -1;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, pwm_group[group].timer, freq);
    if (pwm_group[group].freq_content) {
        char buf[16];
        lv_snprintf(buf, 15, "%d", freq);
        lv_label_set_text(pwm_group[group].freq_content, buf);
    }
    if (shellGetCurrent() != NULL && pwm_group[group].freq_slider) {
        lv_slider_set_value(pwm_group[group].freq_slider, freq, LV_ANIM_ON);
    }
    return 0;

}

static int pwm_set_duty(int group, int index, int duty)
{
    if (group < 0 || group > PWM_GROUP_NUM || index < 0 || index >= PWM_GROUP_CHANNEL_NUM) {
        return -1;
    }
    if (duty < 0) {
        duty = 0;
    }
    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }
    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, pwm_group[group].pwm_cfg[index].channel, duty) == ESP_OK
        && ledc_update_duty(LEDC_LOW_SPEED_MODE, pwm_group[group].pwm_cfg[index].channel) == ESP_OK) {
        if (pwm_group[group].pwm_cfg[index].duty_content) {
            char buf[16];
            lv_snprintf(buf, 15, "%d", duty);
            lv_label_set_text(pwm_group[group].pwm_cfg[index].duty_content, buf);
        }
        if (shellGetCurrent() != NULL && pwm_group[group].pwm_cfg[index].duty_slider) {
            lv_slider_set_value(pwm_group[group].pwm_cfg[index].duty_slider, duty, LV_ANIM_ON);
        }
        return ESP_OK;
    }
    return ESP_FAIL;
}

static ShellCommand pwm_cmd_group[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, freq, pwm_set_freq, 
        set pwm freq\r\npwm freq [group] [freq]\r\n
        group: 0),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, duty, pwm_set_duty,
        set pwm duty\r\npwm duty [group] [index] [duty]\r\n
        group: 0\r\n
        index: 0-2\r\n
        duty: 0-1024),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
pwm, pwm_cmd_group, pwm output tool);

static lv_obj_t* pwm_init_duty_view(int group, int index, lv_obj_t *parent)
{
    lv_obj_t *view = lv_obj_create(parent);
    lv_obj_set_width(view, LV_PCT(100));
    lv_obj_set_height(view, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(view, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(view, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(view, 12, LV_PART_MAIN);
    // lv_obj_set_style_pad_hor(view, 12, LV_PART_MAIN);
    // lv_obj_set_style_pad_ver(view, 12, LV_PART_MAIN);

    char buf[16];
    lv_obj_t *duty_title = lv_label_create(view);
    lv_snprintf(buf, 15, "Duty%d", index);
    lv_label_set_text(duty_title, buf);
    lv_obj_set_width(duty_title, LV_PCT(50));
    lv_obj_set_style_text_align(duty_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align_to(duty_title, view, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *duty_cont = lv_label_create(view);
    lv_label_set_text(duty_cont, "0");
    lv_obj_set_width(duty_cont, LV_PCT(50));
    lv_obj_set_style_text_align(duty_cont, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align_to(duty_cont, duty_title, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_obj_t *duty_slider = lv_slider_create(view);
    lv_obj_set_width(duty_slider, LV_PCT(100));
    lv_slider_set_range(duty_slider, 0, PWM_DUTY_MAX);
    lv_slider_set_value(duty_slider, 0, LV_ANIM_ON);
    lv_obj_add_event_cb(duty_slider, pwm_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_align_to(duty_slider, duty_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);

    pwm_group[group].pwm_cfg[index].duty_content = duty_cont;
    pwm_group[group].pwm_cfg[index].duty_slider = duty_slider;

    return view;
}

static void pwm_init_tab(lv_obj_t *tab, int group)
{
    lv_obj_t *view = lv_obj_create(tab);
    lv_obj_set_width(view, LV_PCT(100));
    lv_obj_set_height(view, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(view, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(view, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(view, 12, LV_PART_MAIN);
    // lv_obj_set_style_pad_ver(view, 12, LV_PART_MAIN);

    lv_obj_t *freq_title = lv_label_create(view);
    lv_label_set_text(freq_title, "Frequency");
    lv_obj_set_width(freq_title, LV_PCT(60));
    lv_obj_set_align(freq_title, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_text_align(freq_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    // freq
    lv_obj_t *freq_cont = lv_label_create(view);
    lv_label_set_text(freq_cont, "5000");
    lv_obj_set_width(freq_cont, LV_PCT(40));
    lv_obj_set_style_text_align(freq_cont, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align_to(freq_cont, freq_title, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_obj_t *freq_slider = lv_slider_create(view);
    lv_obj_set_width(freq_slider, LV_PCT(100));
    lv_slider_set_range(freq_slider, 100, 10000);
    lv_slider_set_value(freq_slider, 5000, LV_ANIM_ON);
    lv_obj_add_event_cb(freq_slider, pwm_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_align_to(freq_slider, freq_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);

    lv_obj_t *front = view;
    for (int i = 0; i < PWM_GROUP_CHANNEL_NUM; i++)
    {
        lv_obj_t *duty_view = pwm_init_duty_view(group, i, tab);
        lv_obj_align_to(duty_view, front, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        front = duty_view;
    }

    pwm_group[group].freq_content = freq_cont;
    pwm_group[group].freq_slider = freq_slider;
}

static void pwm_init_screen(void)
{
    lv_obj_t *scr = pwm_get_screen();

    lv_obj_add_event_cb(scr, pwm_global_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, pwm_global_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, pwm_global_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *tabview = lv_tabview_create(scr);
    lv_obj_set_size(tabview, 
                    lv_display_get_horizontal_resolution(NULL),
                    lv_display_get_vertical_resolution(NULL));
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_20, LV_PART_MAIN);

    char title[16];
    for (int i = 0; i < PWM_GROUP_NUM; i++)
    {
        lv_snprintf(title, 15, "PWM%d", i);
        lv_obj_t *tab = lv_tabview_add_tab(tabview, title);
        lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
        lv_obj_add_flag(tab, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_flag(tab, LV_OBJ_FLAG_EVENT_BUBBLE);
        pwm_init_tab(tab, i);
    }
}

static RtAppErr pwm_suspend(void)
{
    launcher_go_home(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true);
    screen = NULL;
    return RTAM_OK;
}

static RtAppErr pwm_resume(void)
{
    pwm_init_screen();
    gui_push_screen(pwm_get_screen(), LV_SCR_LOAD_ANIM_FADE_IN);
    return RTAM_OK;
}

static RtAppErr pwm_init(void)
{
    for (int i = 0; i < PWM_GROUP_NUM; i++)
    {
        pwm_init_group(i);
    }
    return RTAM_OK;
}

static RtAppErr pwm_deinit(void)
{
    for (int i = 0; i < PWM_GROUP_NUM; i++)
    {
        pwm_deinit_timer(i);
    }
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = pwm_init,
    .stop = pwm_deinit,
    .suspend = pwm_suspend,
    .resume = pwm_resume,
};

static const RtAppDependencies dependencies = {
    .required = (const char *[]){
        "gui",
        "launcher",
        NULL
    },
    .conflicted = (const char *[]){
        "serial_debug",
        NULL
    }
};

extern const lv_image_dsc_t icon_app_pwm;
static const RtamInfo pwm_info = {
    .label = "pwm",
    .icon = (void *) GUI_APP_ICON(pwm),
};

RTAPP_EXPORT(pwm, &interface, 0, &dependencies, &pwm_info);