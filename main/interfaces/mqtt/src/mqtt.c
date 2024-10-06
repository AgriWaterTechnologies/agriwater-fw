#include <string.h>
#include <stdlib.h>
#include <inttypes.h>


#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"

#define CONFIG_BROKER_URI       "mqtt://broker.hivemq.com:1883" 
// #define CONFIG_BROKER_USERNAME  ""                           
// #define CONFIG_BROKER_PASSWORD  ""                          

#define MAX_TRIES               5
#define MQTT_DEBUG


static uint8_t mqttIsConnected  = 0;

esp_mqtt_client_handle_t client = {0};
static const char *TAG = "MQTT_CLIENT";
extern const uint8_t mqtt_eclipseprojects_io_pem_start[] asm("_binary_mqtt_eclipseprojects_io_pem_start");
extern const uint8_t mqtt_eclipseprojects_io_pem_end[] asm("_binary_mqtt_eclipseprojects_io_pem_end");

static void log_error_if_nonzero(const char *message, int error_code);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_subscribe(char *topic);
static void publish(char *topic, char *payload, uint16_t payload_len, uint8_t qos);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRId32, base, event_id);

    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        mqttIsConnected = 1;

        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        mqttIsConnected = 0;

        ESP_ERROR_CHECK(esp_mqtt_client_reconnect(client));

        // esp_restart();
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        }

        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);

            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }

        // esp_restart();
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);

        break;
    }
}

static void mqtt_subscribe(char *topic)
{
    int ret = esp_mqtt_client_subscribe(client, topic, 2);
    ESP_LOGI(TAG, "sent subscribe successful, ret=%d", ret);
}

static void publish(char *topic, char *payload, uint16_t payload_len, uint8_t qos)
{
    if (!payload || !payload_len)
    {
        ESP_LOGE(TAG, "NULL PAYLOAD or PAYLOAD_LEN < 0");
        return;
    }

    int ret = esp_mqtt_client_publish(client, topic, payload, payload_len, qos, 0);
    ESP_LOGI(TAG, "sent publish successful, ret=%d", ret);
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
        .address.uri = CONFIG_BROKER_URI,
        // .verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start
        },
        // .credentials.username = CONFIG_BROKER_USERNAME,
        // .credentials.authentication.password = CONFIG_BROKER_PASSWORD,
        .network.disable_auto_reconnect = false,
        .session.keepalive = 60,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_init()
{
    ESP_LOGI(TAG, "MQTT init");

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    mqtt_app_start();
}

void mqtt_publish(char *topic, char *payload, uint16_t payload_len, uint8_t qos)
{
    if(mqttIsConnected) publish(topic, payload, payload_len, qos);
    else ESP_LOGE(TAG, "CLIENT NOT CONNECTED");
}

esp_err_t mqtt_disconnect() 
{   
    return esp_mqtt_client_disconnect(client);
}

uint8_t mqtt_isConnected()
{
    return mqttIsConnected;
}