#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "driver/uart.h"
#include <driver/adc.h>
#include <driver/gpio.h>
#include <driver/twai.h>
#include "twai.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "main"



void init_twai(){
  esp_err_t can_init_status;

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  twai_timing_config_t timing_config{};

  g_config.intr_flags = ESP_INTR_FLAG_IRAM; // Set TWAI interrupt to IRAM (Enabled in menuconfig)!
  // g_config.clkout_io = TWAI_IO_UNUSED;
  // g_config.bus_off_io = TWAI_IO_UNUSED;
  g_config.rx_queue_len = 32;
  g_config.tx_queue_len = 32;
  // g_config.alerts_enabled = TWAI_ALERT_NONE;
  // g_config.clkout_divider = 0;
  // timing_config = TWAI_TIMING_CONFIG_1MBITS();
  timing_config = TWAI_TIMING_CONFIG_500KBITS();

  // gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  // gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);

  
  // Install TWAI driver
  can_init_status = twai_driver_install(&g_config, &timing_config, &f_config);
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


  // uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
  // if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
  //   printf("CAN Alerts reconfigured");
  // } else {
  //   printf("Failed to reconfigure alerts");
  // }
}


uint8_t msg_counter = 0;

inline void to_bytes(uint64_t src, uint8_t* dst) {
    for(uint8_t i = 0; i < 8; i++) {
        dst[7-i] = src & 0xFF;
        src >>= 8;
    }
}

/* --------------------------- Tasks and Functions -------------------------- */
void twai_rx_task(void *arg){
  twai_message_t message;
  twai_message_t tx_can;
  twai_status_info_t can_status;
  uint64_t tmp;
  

  while (true) {
    // esp_err_t aa = 
    uint64_t now = esp_timer_get_time() / 1000;
    twai_get_status_info(&can_status);
    uint8_t f_count  = can_status.msgs_to_rx;

    if (f_count == 0) {
      msg_counter++;
      if( now % 2000 == 0){
        printf("\n");
        printf("TWAI Status: %d \n", can_status.state);
        printf("TWAI Messages to Receive: %lu \n", can_status.msgs_to_rx);
        printf("TWAI Messages to Send: %lu \n", can_status.msgs_to_tx);
        printf("TWAI Messages Receive Errors: %lu \n", can_status.rx_error_counter);
        printf("TWAI Messages Receive Missed: %lu \n", can_status.rx_missed_count);
        printf("TWAI Messages Bus errors: %lu \n", can_status.bus_error_count);
        printf("TWAI Messages ARB Lost: %lu \n", can_status.arb_lost_count);

        printf("TWAI statuss: %d \n", twai_get_status_info(&can_status));
        printf("TWAI rx error: %d \n", twai_receive(&message, pdMS_TO_TICKS(0)));
        printf("TWAI length Code : %d \n", message.data_length_code);
        printf("TWAI flags : %lu \n", message.flags);

        memset(&tx_can, 0x00, sizeof(twai_message_t));

        tx_can.data_length_code = 8;
        tx_can.identifier = 0x07E9;
        tx_can.extd = 0;
        tx_can.rtr = 0;
        tx_can.ss = 1; // Always single shot
        tx_can.self = 0;
        tx_can.dlc_non_comp = 0;

        twai_transmit(&tx_can, 5);

        // tx_can.identifier = 0xAAAA;
        // tx_can.extd = 1;
        // tx_can.data_length_code = 4;
        // for (int i = 0; i < 4; i++) {
        //     tx_can.data[i] = 0;
        // }
        
        // esp_err_t can_tx_result = twai_transmit(&tx_can, pdMS_TO_TICKS(1000));
        // printf("%d",can_tx_result);

        // //Queue message for transmission
        // if (can_tx_result == ESP_OK) {
        //     printf("Message queued for transmission\n");
        // } else {
        //     printf("Failed to queue message for transmission\n");
        // }

      }

      vTaskDelay(4 / portTICK_PERIOD_MS);
    }else{
        // printf("%d \n",f_count);
        if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
          // printf("Message received\n");

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