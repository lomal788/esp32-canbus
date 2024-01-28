#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <driver/gpio.h>
#include <driver/adc.h>
#include "driver/uart.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "util/sim_7600.h"
#include "util/http_server.h"
#include "util/mqtt_manage.h"
#include "util/wifi_config.h"
#include "util/car/twai.h"
#include "my_config.h"

extern "C" void app_main(void) {

  init_twai();

  xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 5, NULL);
}
