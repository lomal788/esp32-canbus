#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "driver/uart.h"
#include <driver/adc.h>
#include <driver/gpio.h>
#include <driver/twai.h>
#include "twai_base.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "main"

const unsigned int doorId = 16; // Door lock/unlock ID 0x010
const unsigned int doorCatchId = 274; // Door Catch Push ID 0x112
const unsigned int doorStateId = 360; // Door Open State ID 0x168
const unsigned int keyInId = 273; // KeyOn Acc Start State 0x111

BaseCan* twai_can_hal = nullptr;

BaseCan::BaseCan(const char* name, uint8_t tx_time_ms, uint32_t baud) {
  this->name = name;
  this->diag_rx_id = 0x07E1;
  this->diag_tx_id = 0x07E9;
  this->diag_rx_queue = nullptr;
  this->can_init_status = ESP_OK;
  this->tx_time_ms = tx_time_ms;

  // Firstly try to init CAN
  ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "Booting CAN Layer");
  esp_err_t can_init_status;

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(this->can_tx_pin[0], this->can_rx_pin[0], TWAI_MODE_NORMAL);
  // twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_23, GPIO_NUM_22, TWAI_MODE_NORMAL);
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  twai_timing_config_t timing_config{};

  g_config.intr_flags = ESP_INTR_FLAG_IRAM; // Set TWAI interrupt to IRAM (Enabled in menuconfig)!
  // g_config.clkout_io = TWAI_IO_UNUSED;
  // g_config.bus_off_io = TWAI_IO_UNUSED;
  g_config.controller_id = 0;
  g_config.rx_queue_len = 32;
  g_config.tx_queue_len = 32;
  
  // g_config.alerts_enabled = TWAI_ALERT_NONE;
  // g_config.clkout_divider = 0;
  // timing_config = TWAI_TIMING_CONFIG_1MBITS();
  timing_config = TWAI_TIMING_CONFIG_500KBITS();

  // gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  // gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);
  g_config.controller_id = 0;

  // Install TWAI driver
  can_init_status = twai_driver_install_v2(&g_config, &timing_config, &f_config, &this->twai_handler[0]);

  if (can_init_status == ESP_OK) {
    printf("Driver installed\n");
  }else{
    printf("Failed to install driver\n");
    return;
  }

  this->can_init_status = twai_start_v2(this->twai_handler[0]);

  if (this->can_init_status == ESP_OK) {
    printf("started\n");
  }else{
    printf("Failed start\n");
    return;
  }

  g_config.controller_id = 1;
  g_config.tx_io = this->can_tx_pin[1];
  g_config.rx_io = this->can_rx_pin[1];
  can_init_status = twai_driver_install_v2(&g_config, &timing_config, &f_config, &this->twai_handler[1]);
  if (can_init_status == ESP_OK) {
    printf("Driver installed2\n");
  }else{
    printf("Failed to install driver2\n");
    return;
  }

  this->can_init_status = twai_start_v2(this->twai_handler[1]);
  if (can_init_status == ESP_OK){
    printf("Driver started2\n");
  }else{
    printf("Failed to start driver2\n");
    return;
  }

    // Now set the constants for the Tx message.
    this->tx.extd = 0;
    this->tx.rtr = 0;
    this->tx.ss = 0; // Always single shot
    this->tx.self = 0;
    this->tx.dlc_non_comp = 0;
}

esp_err_t BaseCan::init_state() const {
    return this->can_init_status;
}

BaseCan::~BaseCan() {
    if (this->rx_task != nullptr) {
        vTaskDelete(this->rx_task);
    }
    if (this->tx_task != nullptr) {
        vTaskDelete(this->tx_task);
    }
    // Delete CAN
    if (this->can_init_status == ESP_OK) {
        twai_stop_v2(this->twai_handler[0]);
        twai_driver_uninstall_v2(this->twai_handler[0]);
    }
}

bool BaseCan::begin_tasks(){
    if (this->can_init_status != ESP_OK) {
        return false;
    }


    if (this->tx_task == nullptr) {
        ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "Starting CAN Tx task");
        if (xTaskCreate(this->start_tx_task_loop, "TWAI_CAN_TX", 4096, this, 5, &this->tx_task) != pdPASS) {
            ESP_LOG_LEVEL(ESP_LOG_ERROR, this->name, "CAN Tx task creation failed!");
            return false;
        }
    }else{
      ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "ss CAN Rx task");
      printf("tx_take arelady exist");
    }

    // Prevent starting again
    if (this->rx_task == nullptr) {
        ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "Starting CAN Rx task");
        if (xTaskCreate(this->start_rx_task_loop, "TWAI_CAN_RX", 8192, this, 5, &this->rx_task) != pdPASS) {
            ESP_LOG_LEVEL(ESP_LOG_ERROR, this->name, "CAN Rx task creation failed!");
            return false;
        }
    }


    return true; // Ready!
}

inline void to_bytes(uint64_t src, uint8_t* dst) {
  for(uint8_t i = 0; i < 8; i++) {
      dst[7-i] = src & 0xFF;
      src >>= 8;
  }
}

[[noreturn]]
void BaseCan::tx_task_loop() {
  while(true) {
    if (this->send_messages[0]) {
          this->tx_frames(0);
      }
    // for(uint8_t i = 0; i < 2; i++){
    //   if (this->send_messages[i]) {
    //       this->tx_frames(i);
    //   }
    // }
      vTaskDelay(this->tx_time_ms / portTICK_PERIOD_MS);
  }
}

[[noreturn]]
void BaseCan::rx_task_loop(){
  twai_message_t message;
  twai_message_t rx;
  // twai_message_t tx_can;
  twai_status_info_t can_status;
  uint8_t i;
  uint64_t tmp;
  
  while (true) {
    // esp_err_t aa = 
    uint64_t now = esp_timer_get_time() / 1000;
    // twai_get_status_info_v2(this->twai_handler[0], &can_status);
    twai_get_status_info(&can_status);
    uint8_t f_count  = can_status.msgs_to_rx;

    this->alive_cnt+=1;

    if (f_count == 0) {
      // printf("%d", f_count);
      if( now % 2000 == 0){
        // printf("\n");
        // printf("TWAI Status: %d \n", can_status.state);
        // printf("TWAI Messages to Receive: %lu \n", can_status.msgs_to_rx);
        // printf("TWAI Messages to Send: %lu \n", can_status.msgs_to_tx);
        // printf("TWAI Messages Receive Errors: %lu \n", can_status.rx_error_counter);
        // printf("TWAI Messages Receive Missed: %lu \n", can_status.rx_missed_count);
        // printf("TWAI Messages Bus errors: %lu \n", can_status.bus_error_count);
        // printf("TWAI Messages ARB Lost: %lu \n", can_status.arb_lost_count);

        // printf("TWAI statuss: %d \n", twai_get_status_info(&can_status));
        // printf("TWAI rx error: %d \n", twai_receive(&message, pdMS_TO_TICKS(0)));
        // printf("TWAI length Code : %d \n", message.data_length_code);
        // printf("TWAI flags : %lu \n", message.flags);

        // memset(&tx_can, 0x00, sizeof(twai_message_t));
        // unsigned char tx_dataaa[8] = {0x1,0x2,0x3,0x4,0xf,0xa,0xc,0xd};

        // for (int i=0; i < sizeof(tx_dataaa); i++) {
        //   tx_can.data[i] = tx_dataaa[i];
        // }

        // tx_can.data_length_code = sizeof(tx_dataaa);
        // tx_can.identifier = 0x07E8;
        // tx_can.extd = 0;
        // tx_can.rtr = 0;
        // tx_can.ss = 1; // Always single shot
        // tx_can.self = 0;
        // tx_can.dlc_non_comp = 0;
        // // to_bytes(tx_dataa, tx_can.data);

        // // to_bytes(eng_rq1_tcm_tx.raw, tx_can.data);
        // twai_transmit(&tx_can, 5);

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
      for(uint8_t x = 0; x < f_count; x++) { // Read all frames
        if (twai_receive_v2(this->twai_handler[0], &rx, pdMS_TO_TICKS(0)) == ESP_OK && rx.data_length_code != 0 && rx.flags == 0) {
          if (this->diag_rx_id != 0 && rx.identifier == this->diag_rx_id) {
            // ISO-TP Diag endpoint
            if (this->diag_rx_queue != nullptr && rx.data_length_code == 8) {
                // Send the frame
                if (xQueueSend(*this->diag_rx_queue, rx.data, 0) != pdTRUE) {
                    ESP_LOG_LEVEL(ESP_LOG_ERROR, "EGS_BASIC_CAN","Discarded ISO-TP endpoint frame. Queue send failed");
                }
            }
          } else { // Normal message
            tmp = 0;
            for(i = 0; i < rx.data_length_code; i++) {
                tmp |= (uint64_t)rx.data[i] << (8*(7-i));
            }
            this->on_rx_frame(rx.identifier, rx.data_length_code, tmp, now, 0);
          }
        }
      }
        // printf("%d \n",f_count);
        // if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        //   // printf("Message received\n");

        //   tmp = 0;
        //   for (int i = 0; i < message.data_length_code; i++){
        //     tmp |= (uint64_t)message.data[i] << (8*(7-i));
        //   }

        //   if(message.identifier == 790) {
        //     printf("%lu %d %llu %llu ", message.identifier, message.data_length_code, tmp, now);
        //     // printf("%d", message.data[0]);
        //     printf("\n");
        //   // 시동 확인
        //   }else if( message.identifier == keyInId) {
        //     if(message.data[0] == 64 || message.data[0] == 128 || message.data[0] == 130){
        //       // 시동 KeyIn & On
        //     }else{
        //       // KeyOut & Off
        //     }
        //   // 도어 상태 확인
        //   }else if( message.identifier == doorStateId){

        //   }

        // }
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
    this->on_rx_done(now);
  }
}
