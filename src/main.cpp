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
  // gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT_OD);
  // gpio_set_level(GPIO_NUM_22, 1);

	printf("%s on core %d", __func__, xPortGetCoreID());

  // wifi_connection();
  // server_initiation();

  twai_can_hal = new Car("CAR", 20, 500000);
  // printf("HELLO2");
	spi_can_init();
  // vTaskDelay(100);

  if (!twai_can_hal->begin_tasks()) {
    printf("can task err");
  }

  // this->LTE = new LTE_MODEM(&twai_can_hal);

  // xTaskCreate(&rx_task, "uart_rx_task", 1024*2, NULL, 5, NULL);

  // init_twai();0

  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 5, NULL);

  // mqtt_start();

  // int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);

  // if (data_read > 0) {
  // }

  // init_mqtt();
  // connect_mqtt_server();
  // subscribe_mqtt("test/1234");

  // xTaskCreate(&rx_task, "mqtt_sim_rx_task", 1024 * 8, NULL, 6, NULL);
  // xTaskCreate(&call_sim_spam_task, "call_spam_task", 1024*4, NULL, 5, NULL);

  // init_twai();
  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 6, NULL);
}