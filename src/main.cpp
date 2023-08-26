#include <stdio.h>
#include <string.h>

#include <driver/twai.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_system.h>
#include <esp_event_loop.h>
#include <esp_err.h>
#include <esp_timer.h>

#include "my_config.h"

#include "util/http_server.h"
#include "util/wifi_config.h"
#include "util/mqtt_manage.h"

uint8_t msg_counter = 0;

inline void to_bytes(uint64_t src, uint8_t* dst) {
    for(uint8_t i = 0; i < 8; i++) {
        dst[7-i] = src & 0xFF;
        src >>= 8;
    }
}

/* --------------------------- Tasks and Functions -------------------------- */
static void rx_task_loop(void *arg){
  twai_message_t message;
  // twai_message_t tx_can;
  twai_status_info_t can_status;
  uint64_t tmp;
  

  while (true) {
    // esp_err_t aa = 
    uint64_t now = esp_timer_get_time() / 1000;
    twai_get_status_info(&can_status);
    uint8_t f_count  = can_status.msgs_to_rx;

    if (f_count == 0) {
      msg_counter++;
      // if( now % 2000 == 0){
      //   printf("\n");
      //   printf("TWAI Status: %d \n", can_status.state);
      //   printf("TWAI Messages to Receive: %lu \n", can_status.msgs_to_rx);
      //   printf("TWAI Messages to Send: %lu \n", can_status.msgs_to_tx);
      //   printf("TWAI Messages Receive Errors: %lu \n", can_status.rx_error_counter);
      //   printf("TWAI Messages Receive Missed: %lu \n", can_status.rx_missed_count);
      //   printf("TWAI Messages Bus errors: %lu \n", can_status.bus_error_count);
      //   printf("TWAI Messages ARB Lost: %lu \n", can_status.arb_lost_count);

      //   printf("TWAI statuss: %d \n", twai_get_status_info(&can_status));
      //   printf("TWAI rx error: %d \n", twai_receive(&message, pdMS_TO_TICKS(0)));

      //   // memset(&tx_can, 0x00, sizeof(twai_message_t));

      //   // tx_can.data_length_code = 8;
      //   // tx_can.identifier = 0x07E9;
      //   // tx_can.extd = 0;
      //   // tx_can.rtr = 0;
      //   // tx_can.ss = 1; // Always single shot
      //   // tx_can.self = 0;
      //   // tx_can.dlc_non_comp = 0;

      //   // twai_transmit(&tx_can, 5);
      // }

      vTaskDelay(4 / portTICK_PERIOD_MS);
    }else{
        printf("%d \n",f_count);
        if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
          printf("Message received\n");

          tmp = 0;
          for (int i = 0; i < message.data_length_code; i++){
            tmp |= (uint64_t)message.data[i] << (8*(7-i));
          }

          printf("%lu %d %llu %llu ", message.identifier, message.data_length_code, tmp, now);
          printf("\n");
        }
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
  }
}

// mqtt_settings settings = {
// 	.host = MQTT_SERVER_IP,
// 	.port = 8883,
// 	.client_id = MQTT_CLIENT_ID,
// 	.clean_session = 0,
// 	.keepalive = 120,
// 	.connected_cb = mqtt_connected_callback
// };

extern "C" void app_main(void){
  esp_err_t can_init_status;
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  g_config.intr_flags = ESP_INTR_FLAG_IRAM; // Set TWAI interrupt to IRAM (Enabled in menuconfig)!
  g_config.rx_queue_len = 32;
  g_config.tx_queue_len = 32;


  // Install TWAI driver
  can_init_status = twai_driver_install(&g_config, &t_config, &f_config);
  if (can_init_status == ESP_OK) {
    printf("Driver installed\n");
  }else{
    printf("Failed to install driver\n");
    return;
  }

  can_init_status = twai_start();
  // Start TWAI driver
  if (can_init_status == ESP_OK){
    printf("Driver started\n");
  }else{
    printf("Failed to start driver\n");
    return;
  }

  wifi_connection();
	server_initiation();
  mqtt_start();

  xTaskCreate(&rx_task_loop, "hello_task", 8192, NULL, 5, NULL);
}
