#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "my_config.h"
#include "driver/uart.h"
#include <driver/gpio.h>

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "main"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_14)
#define RXD_PIN (GPIO_NUM_27)

#define UART UART_NUM_2


char *replaceAll(char *s, const char *olds, const char *news) {
  char *result, *sr;
  size_t i, count = 0;
  size_t oldlen = strlen(olds); if (oldlen < 1) return s;
  size_t newlen = strlen(news);


  if (newlen != oldlen) {
    for (i = 0; s[i] != '\0';) {
      if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
      else i++;
    }
  } else i = strlen(s);


  result = (char *) malloc(i + 1 + count * (newlen - oldlen));
  if (result == NULL) return NULL;


  sr = result;
  while (*s) {
    if (memcmp(s, olds, oldlen) == 0) {
      memcpy(sr, news, newlen);
      sr += newlen;
      s  += oldlen;
    } else *sr++ = *s++;
  }
  *sr = '\0';

  return result;
}


void init_sim(){
  // const uart_port_t uart_num = UART_NUM_2;
  const uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
      // .rx_flow_ctrl_thresh = 122,
  };

  // const int uart_buffer_size = (1024 * 2);
  // QueueHandle_t uart_queue;
  // ESP_ERROR_CHECK(uart_driver_install(UART, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
  // uart_driver_install(UART, RX_BUF_SIZE * 2, 0, 0, NULL, ESP_INTR_FLAG_IRAM);
  ESP_ERROR_CHECK(uart_driver_install(UART, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  // uart_intr_config_t uart_intr = {
  //     .intr_enable_mask = UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL,
  //     .rxfifo_full_thresh = 20,
  //     // .rx_timeout_thresh = 20,
  // };

  // uart_intr_config(UART, &uart_intr);
  // uart_enable_rx_intr(UART);
}

static void tx_task(void *arg){
  char* Txdata = (char*) malloc(100);

  while(1){
    sprintf(Txdata, "AT+GMR\r\n");
    const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
    if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  free(Txdata);
  vTaskDelete(NULL);
}

static void rx_task(void *arg){
  static const char *RX_TASK_TAG = "RX_TASK";
  esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
  uint8_t* rx_buffer = (uint8_t*) malloc(RX_BUF_SIZE+1);

  while(1){
    // const int64_t message_received_us = esp_timer_get_time();
    // int length = 0;
    // ESP_ERROR_CHECK(uart_get_buffered_data_len(UART, (size_t*)&length));
    // if (length) {
    //   ESP_LOGI(TAG, "received %d bytes\n", length);
    // }

    const int rxBytes = uart_read_bytes(UART, rx_buffer, RX_BUF_SIZE, 500 / portTICK_PERIOD_MS);

    if (rxBytes > 0) {

      rx_buffer[rxBytes] = 0;
      // String aa = "";

      // rx_buffer = replaceAll(rx_buffer,"\r\n", "OK");

      ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'\n", rxBytes, rx_buffer);

      

      // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, rx_buffer, rxBytes, ESP_LOG_INFO);
    }
  }

  free(rx_buffer);
  vTaskDelete(NULL);
}
