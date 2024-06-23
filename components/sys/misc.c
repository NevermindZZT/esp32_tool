/**
 * @file misc.c
 * @author Letter
 * @brief 
 * @version 0.1
 * @date 2020-02-29
 * 
 * @copyright (c) 2020 Letter
 * 
 */

#include "esp_system.h"
#include "esp_idf_version.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "shell.h"

static const char *TAG = "misc";

#define STATS_TICKS         pdMS_TO_TICKS(1000)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

void miscShowSystemInfo(void)
{
    /* Print chip information */
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    uint32_t flashSize;
    ESP_LOGI(TAG, "Chip:                %d CPU cores, WiFi%s%s",
             chipInfo.cores,
             (chipInfo.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chipInfo.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGI(TAG, "Silicon revision:    %d", chipInfo.revision);
    if (esp_flash_get_size(NULL, &flashSize) != ESP_OK) {
        ESP_LOGE(TAG, "Flash:               error");
    } else {
        ESP_LOGI(TAG, "Flash:               %luMB %s", flashSize / (1024 * 1024),
                (chipInfo.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    }

    ESP_LOGI(TAG, "Idf version:         %d.%d.%d", 
            ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    ESP_LOGI(TAG, "Memory:              %lu free, %lu minimum",
            esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
sysInfo, miscShowSystemInfo, show system info);


/**
 * @brief   Function to print the CPU usage of tasks over a given duration.
 *
 * This function will measure and print the CPU usage of tasks over a specified
 * number of ticks (i.e. real time stats). This is implemented by simply calling
 * uxTaskGetSystemState() twice separated by a delay, then calculating the
 * differences of task run times before and after the delay.
 *
 * @note    If any tasks are added or removed during the delay, the stats of
 *          those tasks will not be printed.
 * @note    This function should be called from a high priority task to minimize
 *          inaccuracies with delays.
 * @note    When running in dual core mode, each core will correspond to 50% of
 *          the run time.
 *
 * @param   xTicksToWait    Period of stats measurement
 *
 * @return
 *  - ESP_OK                Success
 *  - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
 *  - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET
 *  - ESP_ERR_INVALID_STATE Delay duration too short
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret;
    Shell *shell = shellGetCurrent();
    if (shell == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xTicksToWait == 0) {
        xTicksToWait = STATS_TICKS;
    }

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    shellPrint(shell, "%-16s %-8s %-10s %s\n", "Task", "Run Time", "Percentage", "Core");
    shellPrint(shell, "%-16s %-8s %-10s %s\n", "----", "------", "----", "----");
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        int k = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                k = j;
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        //Check if matching task found
        if (k >= 0) {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            shellPrint(shell, "%-16s %-8"PRIu32" %-10"PRIu32"%% %"PRIu32"\n",
                       start_array[i].pcTaskName, task_elapsed_time, percentage_time, start_array[i].xCoreID);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            shellPrint(shell, "%s Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            shellPrint(shell, "%s Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1),
ps, print_real_time_stats, print task stats);

static void intr_dump(void)
{
    esp_intr_dump(NULL);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1),
intr, intr_dump, dump interrupt information);
