/**
 * @file storage.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief storage
 * @version 1.0.0
 * @date 2024-06-09
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "rtam.h"
#include "shell.h"
#include "wear_levelling.h"

#define CONFIG_PATH_SPIFLASH "/spiflash"

static wl_handle_t internal_storage_handle = WL_INVALID_HANDLE;

static int rt_app_status = RTAPP_STATUS_STOPPED;

wl_handle_t storage_get_internal_handle(void)
{
    return internal_storage_handle;
}

static void storage_test_file_read(char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE("storage", "Failed to open file for reading, file: %s", path);
        return;
    }
    ESP_LOGI("storage", "File opened for reading, file: %s", path);
    fclose(f);

}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
test_file_read, storage_test_file_read, test file read);

static int storage_internal_init(void)
{
    rt_app_status = RTAPP_STATUS_STARING;
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 256,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
    };
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl(CONFIG_PATH_SPIFLASH, "storage", &mount_config, &internal_storage_handle));
    rt_app_status = RTAPP_STATUS_RUNNING;
    return 0;
}

static int storage_exit(void)
{
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount_rw_wl(CONFIG_PATH_SPIFLASH, internal_storage_handle));
    return 0;
}

static int storage_get_status(void)
{
    return rt_app_status;
}

RTAPP_EXPORT(storage, storage_internal_init, storage_exit, storage_get_status, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, NULL, NULL);
