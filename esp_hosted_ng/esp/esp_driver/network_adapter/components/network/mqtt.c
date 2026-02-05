#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "pthread.h"
#include "esp_mac.h"
#include "esp_chip_info.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "network.h"

char mqtt_client_id[32];

static const char *TAG = "MQTT";
pthread_mutex_t g_mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;
esp_mqtt_client_handle_t g_mqtt_client = NULL;
int mqtt_status = 0;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        pthread_mutex_lock(&g_mqtt_mutex);
        mqtt_status = 1;
        pthread_mutex_unlock(&g_mqtt_mutex);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        pthread_mutex_lock(&g_mqtt_mutex);
        mqtt_status = 0;
        pthread_mutex_unlock(&g_mqtt_mutex);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

int mqtt_init_client_id()
{
    uint8_t mac[6];
    esp_chip_info_t chip_i;
    esp_chip_info(&chip_i);
    esp_read_mac(mac, ESP_MAC_BASE);

    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "ESP32_%d_%02x%02X%02X", chip_i.model, mac[3], mac[4], mac[5]);
    printf("MQTT client id %s\n", mqtt_client_id);
    return 0;
}

int mqtt_init()
{
    mqtt_init_client_id();

    const esp_mqtt_client_config_t mqtt_cfg =
    {
        // .broker.address.uri = "mqtt://192.168.12.173:1883",
        .broker.address.hostname = "192.168.12.173",
        .broker.address.port = 1883,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .broker.address.path = NULL,

        .credentials.client_id = mqtt_client_id,
        .credentials.set_null_client_id = false,
    };


    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (g_mqtt_client == NULL)
    {
        printf("esp_mqtt_client_init error!\n");
        return -1;
    }
    esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, g_mqtt_client);
    esp_mqtt_client_start(g_mqtt_client);

    return 0;
}

int mqtt_publish(const char *topic, const char *data)
{

    char _topic[64];

    snprintf(_topic, 64, "%s/%s", mqtt_client_id, topic);

    pthread_mutex_lock(&g_mqtt_mutex);
    
    do
    {
        if (mqtt_status == 0 || g_mqtt_client == NULL)
            break;
    
        esp_mqtt_client_publish(g_mqtt_client, _topic, data, strlen(data), 1, 1);

    }
    while (0);
    
    pthread_mutex_unlock(&g_mqtt_mutex);

    return 0;
}