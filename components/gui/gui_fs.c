/**
 * @file gui_fs.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief gui fs support
 * @version 1.0.0
 * @date 2024-06-09
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "lvgl.h"
#include "misc/lv_fs.h"

static const char *TAG = "gui_fs";

static bool lv_ready_cb(lv_fs_drv_t * drv)
{
    return true;
}

static void *lv_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    FILE *f = fopen(path, LV_FS_MODE_WR == mode ? "wb" : "rb");
    return f;
}

static lv_fs_res_t close_cb(lv_fs_drv_t * drv, void * file_p)
{
    fclose(file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    *br = fread(buf, 1, btr, file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t write_cb(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    *bw = fwrite(buf, 1, btw, file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    return fseek(file_p, pos, whence);
}

static lv_fs_res_t tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    *pos_p = ftell(file_p);
    return LV_FS_RES_OK;
}

void gui_fs_init(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter = 'S';
    drv.cache_size = 32768;

    drv.ready_cb = lv_ready_cb;
    drv.open_cb = lv_open_cb;
    drv.close_cb = close_cb;
    drv.read_cb = read_cb;
    drv.write_cb = write_cb;
    drv.seek_cb = seek_cb;
    drv.tell_cb = tell_cb;

    lv_fs_drv_register(&drv);
}
