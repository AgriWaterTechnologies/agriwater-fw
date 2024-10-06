#pragma once

#include "mqtt_client.h"

typedef struct topic {
    char company[10];
    char *subtopic;
    char *device_id;
    char *cmd;
}mqtt_topic;


void mqtt_init(void);
void mqtt_publish(char *topic, char *payload, uint16_t payload_len, uint8_t qos);
uint8_t mqtt_isConnected();
esp_err_t mqtt_disconnect();