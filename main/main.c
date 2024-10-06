#include <stdio.h>
#include <inttypes.h>
#include "string.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "wifi.h"
#include "mqtt.h"
#include <ultrasonic.h>


#define TAG             "APP_MAIN"
#define MQTT_INIT_TOPIC	"AgriWater/INIT"
#define MQTT_DATA_TOPIC	"AgriWater/data/level"

#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO    5
#define ECHO_GPIO       18

uint8_t is_wifi_connected 	= 0;

void ultrasonic(void *pvParameters)
{
    char dataStr[32] = {0};

    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);

    while (true)
    {
        float distance = 0.0;
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error %d: ", res);
            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%s\n", esp_err_to_name(res));
            }
        }
        else
        {
            printf("Distance: %0.04f cm ||| 7.1 - distance: %0.04f\n", distance*100, ((7.1 - (distance*100)) < 0 ) ? 0 : (7.1 - (distance*100)));
            sprintf(dataStr, "%.04f", ((7.1 - (distance*100)) < 0 ) ? 0 : (7.1 - (distance*100)));

            mqtt_publish(MQTT_DATA_TOPIC, dataStr, strlen(dataStr), 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    printf("Hello world!\n");
    // initialize NVS
	esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

    ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "NVS INIT");

    is_wifi_connected = wifi_init();

    if(is_wifi_connected)
    {
        ESP_LOGI(TAG, "WIFI INIT OK");
        mqtt_init();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else ESP_LOGW(TAG, "WIFI INIT FAIL");

    while(!is_wifi_connected && !mqtt_isConnected()) vTaskDelay(50 / portTICK_PERIOD_MS);
    
    char *init_message = (char *) malloc(35);
	
    if (!init_message) ESP_LOGW(TAG, "Failed to allocate memory to init_message");
	else
	{
		sprintf(init_message, "ESP INIT OK");
		mqtt_publish(MQTT_INIT_TOPIC, init_message, strlen(init_message), 0);
	}
    
    xTaskCreate(ultrasonic, "ultrasonic", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
