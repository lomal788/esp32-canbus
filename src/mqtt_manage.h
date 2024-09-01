#ifndef MQTT_MANAGE
#define MQTT_MANAGE

#include <mqtt_client.h>


void mqtt_start();
void mqtt_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void mqtt_task_loop(void *arg);

#endif