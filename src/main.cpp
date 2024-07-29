#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "driver/uart.h"
#include <driver/gpio.h>
#include <driver/adc.h>

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "util/sim_7600.h"
#include "util/http_server.h"
#include "util/mqtt_manage.h"
#include "util/wifi_config.h"
#include "util/car/twai_base.h"
#include "util/car/car.h"
#include "util/car/spi_can_base.h"

#include "my_config.h"

// #include <stdlib.h>
// #include "esp_attr.h"
// #include "soc/timer_group_struct.h"
// #include "soc/timer_group_reg.h"
// #include "driver/spi_master.h"


// #include "esp_netif.h"
// #include "esp_netif_ppp.h"
// #include "mqtt_client.h"
// #include "esp_modem.h"
// #include "esp_modem_netif.h"
// #include "esp_log.h"
// #include "sim800.h"
// #include "bg96.h"
// #include "sim7600.h"

extern "C" void app_main(void) {
	printf("%s on core %d", __func__, xPortGetCoreID());

  // wifi_connection();
  // server_initiation();
	spi_can_init();

  twai_can_hal = new Car("CAN_BASE", 20, 500000);

  if (!twai_can_hal->begin_tasks()) {
    printf("can task err");

  }

  // init_twai();

  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 5, NULL);

  // mqtt_start();

  // int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);

  // if (data_read > 0) {
  // }

//   init_sim();
  // init_mqtt();
  // connect_mqtt_server();
  // subscribe_mqtt("test/1234");

  // xTaskCreate(&rx_task, "mqtt_sim_rx_task", 1024 * 8, NULL, 6, NULL);
  // xTaskCreate(&call_sim_spam_task, "call_spam_task", 1024*4, NULL, 5, NULL);

  // init_twai();
  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 6, NULL);
}
