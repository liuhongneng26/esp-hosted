#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

#include "network.h"

#include "iperf_cmd.h"

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        printf("OPEN\t");
        break;
    case WIFI_AUTH_OWE:
        printf("OWE\t");
        break;
    case WIFI_AUTH_WEP:
        printf("WEP\t");
        break;
    case WIFI_AUTH_WPA_PSK:
        printf("WPA_PSK\t");
        break;
    case WIFI_AUTH_WPA2_PSK:
        printf("WPA2_PSK\t");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        printf("WPA_WPA2_PSK\t");
        break;
    case WIFI_AUTH_ENTERPRISE:
        printf("ENTERPRISE\t");
        break;
    case WIFI_AUTH_WPA3_PSK:
        printf("WPA3_PSK\t");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        printf("WPA2_WPA3_PSK\t");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        printf("WPA3_ENT_192\t");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK:
        printf("WPA3_EXT_PSK\t");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
        printf("WPA3_EXT_PSK_MIXED_MODE\t");
        break;
    default:
        printf("UNKNOWN_AUTH\t");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher)
    {
    case WIFI_CIPHER_TYPE_NONE:
        printf("CIPHER_NONE\t");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        printf("WEP40\t");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        printf("WEP104\t");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        printf("TKIP\t");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        printf("CCMP\t");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        printf("TKIP_CCMP\t");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        printf("AES_CMAC128\t");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        printf("SMS4\t");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        printf("GCMP\t");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        printf("GCMP256\t");
        break;
    default:
        printf("CIPHER_UNKNOWN\t");
        break;
    }

    switch (group_cipher)
    {
    case WIFI_CIPHER_TYPE_NONE:
        printf("CIPHER_NONE\t");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        printf("WEP40\t");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        printf("WEP104\t");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        printf("TKIP\t");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        printf("CCMP\t");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        printf("TKIP_CCMP\t");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        printf("SMS4\t");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        printf("GCMP\t");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        printf("GCMP256\t");
        break;
    default:
        printf("CIPHER_UNKNOWN\t");
        break;
    }
}

int wifi_scan_down()
{
    int i;
    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

    for (i = 0; i < number; i++)
    {
        printf("%s\n\t%d\t", ap_info[i].ssid, ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        printf("%d\t", ap_info[i].primary);
        if (ap_info[i].authmode != WIFI_AUTH_WEP)
        {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        printf("\n");
    }

    return 0;
}

int wifi_scan()
{
    esp_wifi_scan_start(NULL, true);
    wifi_scan_down();
    return 0;
}


void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    printf("event:%s-%ld\n", event_base, event_id);

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:

                break;
            case WIFI_EVENT_SCAN_DONE:
                
                break;
            default:
                break;
        }

    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                printf("got ip:" IPSTR"\n", IP2STR(&event->ip_info.ip));

                break;
            case IP_EVENT_STA_LOST_IP:

                break;
            default:
                break;
        }
    }

    return;
}

int wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

	esp_netif_set_hostname(sta_netif, "esp32_lhn");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_config_t));
#if 0
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
#endif

    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
    printf("wifi info! \n%s-%s\n",  wifi_cfg.sta.ssid, wifi_cfg.sta.password);

    if (strlen((const char *)wifi_cfg.sta.ssid))
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    else
    {
        network_prov();
    }

    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);


    esp_err_t ret = iperf_cmd_register_iperf();
    if (ret != ESP_OK)
    {
        printf("register iper error: %d\n", ret);
    }
    mqtt_init();

    return 0;
}