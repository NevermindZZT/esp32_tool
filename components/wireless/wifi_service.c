/**
 * @file wifi_service.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief wifi service
 * @version 1.0.0
 * @date 2024-06-11
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "rtam.h"
 
static const char *TAG = "wifi_service";

static EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // TODO
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        esp_netif_sntp_start();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

#ifdef CONFIG_SET_MAC_ADDRESS_OF_TARGET_AP
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            ESP_LOGI(TAG, "Set MAC address of target AP: "MACSTR" ", MAC2STR(evt->bssid));
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }
#endif

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static ShellCommand wifi_cmd_group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, connect, esp_wifi_connect, connect to ap),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, disconnect, esp_wifi_disconnect, disconnect to ap),
    // {.attr.value = 0},
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
wifi, wifi_cmd_group, wifi service command group);

static void wifi_ntp_init(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("ntp.aliyun.com");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
}

static RtAppErr wifi_service_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    esp_wifi_connect();

    wifi_ntp_init();

    return RTAM_OK;
}

static RtAppErr wifi_service_stop(void)
{
    ESP_ERROR_CHECK( esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler) );
    ESP_ERROR_CHECK( esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler) );
    ESP_ERROR_CHECK( esp_event_handler_unregister(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler) );
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_netif_deinit() );
    ESP_ERROR_CHECK( esp_event_loop_delete_default() );
    vEventGroupDelete(s_wifi_event_group);
    return RTAM_OK;
}

static const RtAppInterface interface = {
    .start = wifi_service_init,
    .stop = wifi_service_stop,
};

RTAPP_EXPORT(wifi_service, &interface, RTAPP_FLAG_AUTO_START|RTAPP_FLAG_SERVICE, NULL, NULL);