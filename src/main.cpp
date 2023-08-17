#include "driver/twai.h"
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

/* --------------------------- Tasks and Functions -------------------------- */
static void rx_task_loop(void *arg){
  while (true) {
    twai_message_t message;
    twai_status_info_t can_status;
    // esp_err_t aa = 
    twai_get_status_info(&can_status);
    uint8_t f_count  = can_status.msgs_to_rx;
    // printf("%d \n",aa);

    if (f_count == 0) {
        vTaskDelay(4 / portTICK_PERIOD_MS); // Wait for buffer to have at least 1 frame
      }else{
        printf("%d \n",f_count);
        if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
          //printf("Message received\n");
          // Process received message
        /* if (message.extd){
            printf("Message is in Extended Format\n");
          }else{
            printf("Message is in Standard Format\n");
          }*/
          // printf("0x%03x :", message.identifier);
          if (!(message.rtr)){
            for (int i = 0; i < message.data_length_code; i++){
              printf("0x%02x ", message.data[i]);
            }
            printf("\n");
          }
        }
        vTaskDelay(2 / portTICK_PERIOD_MS); // Reset watchdog here
    }
  }
}

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

  xTaskCreate(&rx_task_loop, "hello_task", 8192, NULL, 5, NULL);
}
