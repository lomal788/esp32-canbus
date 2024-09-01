#ifndef WIFI_CONFIG
#define WIFI_CONFIG

#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_connection();

#endif