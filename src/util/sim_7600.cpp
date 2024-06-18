#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <nvs_flash.h>
#include <nvs.h>
#include "my_config.h"
#include "driver/uart.h"
#include <driver/gpio.h>
#include "sim_7600.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "main"

static const int RX_BUF_SIZE = 1024;
#define MAX_STR_LEN 4000

#define TXD_PIN (GPIO_NUM_14)
#define RXD_PIN (GPIO_NUM_27)

// #define UART UART_NUM_2

char* rtrim(char* s) {
 
char t[MAX_STR_LEN];
 char *end;
strcpy(t, s);
 
end = t + strlen(t) - 1;
 while (end != t && isspace(*end))
end--;
 *(end + 1) = '\0';
 s = t;
 return s;
}

char* ltrim(char *s) {
 char* begin;
 begin = 
s;
 while (*begin != '\0') {
 if (isspace(*begin))
 
begin++;
 else {
 s = begin;
 break;
 }
 
}
 return s;
}

char* trim(char *s) {
 return rtrim(ltrim(s));
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**) malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


/* return an empty string if a >= b */
char *slice(const char *s, size_t a, size_t b) {
    size_t len = strlen(s);
    if (a > len)
        a = len;
    if (b < a)
        b = a;
    char *slice = (char*)malloc(b - a + 1);
    // char *foo = (char*)malloc(1);
    if (slice != NULL) {
        for (size_t i = a; i < b; i++)
             slice[i - a] = s[i];
        slice[b - a] = '\0';
    }
    return slice;
}


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
  
  // sendSimATCmd("AT+CRESET");
  // sendSimATCmd("AT+CPIN?");
  // sendSimATCmd("AT+COPS?");
  // sendSimATCmd("AT+CPSI?");
  

  // uart_intr_config_t uart_intr = {
  //     .intr_enable_mask = UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL,
  //     .rxfifo_full_thresh = 20,
  //     // .rx_timeout_thresh = 20,
  // };

  // uart_intr_config(UART, &uart_intr);
  // uart_enable_rx_intr(UART);
}

void http_req(){
  sendSimATCmd("AT+HTTPINIT");
  sendSimATCmd("AT+HTTPPARA=\"URL\", \"http://webhook.site/991d65bd-ce0d-4d4d-8cdc-16ec65795d73\"");
  sendSimATCmd("AT+HTTPACTION=0");
  sendSimATCmd("AT+HTTPREAD?");
  sendSimATCmd("AT+HTTPREAD=0,140");
}
// AT+CPIN?

void init_mqtt(){
  
  sendSimATCmd("AT+CPSI?");
  sendSimATCmd("AT+CGREG?");
  sendSimATCmd("ATE0");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CIPMODE=1");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CSOCKSETPN=1");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CIPMODE=0");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+NETOPEN");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CIPMODE=0");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CMQTTCONNECT?");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CMQTTSTART");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CMQTTACCQ=0,\"SIM7600_client\"");
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

void disconnect_mqtt(){
//   AT+CMQTTDISC=0,120 
// // release the client
// AT+CMQTTREL=0
// //stop MQTT Service
// AT+CMQTTSTOP
// AT+CMQTTDISC
}

void connect_mqtt_server(){
  sendSimATCmd("AT+CMQTTCONNECT=0,\"tcp://broker.mqtt.cool:1883\",60,1");
  vTaskDelay(500 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CMQTTSUB=0,9,1,1");
  vTaskDelay(500 / portTICK_PERIOD_MS);
  sendSimATCmd("test/1234");
}

void sendSimATMsg(const char* cmd){
  char* Txdata = (char*) malloc(100);

  sprintf(Txdata, "%s",cmd);
  const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
  if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
  free(Txdata);
}

void sendSimATCmd(const char* cmd){
  char* Txdata = (char*) malloc(100);

  sprintf(Txdata, "%s\r\n",cmd);
  const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
  // if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
  free(Txdata);
}

void subscribe_mqtt(const char* nm){
  char* sub_cmd = (char*) malloc(100);

  sprintf(sub_cmd, "AT+CMQTTSUB=0,%d,1,1", strlen(nm));
  sendSimATCmd((const char*) sub_cmd);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  sendSimATMsg(nm);
}

char* getMqttResponse(){
  uint8_t* rx_buffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
  const int rxBytes = uart_read_bytes(UART, rx_buffer, RX_BUF_SIZE, 500 / portTICK_PERIOD_MS);
  
  if (rxBytes > 0) {
    rx_buffer[rxBytes] = 0;
    // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'\n", rxBytes, rx_buffer);
  }
  return (char*)rx_buffer;
}

void send_topic_mqtt(const char* nm, const char* msg){

  char* topic_cmd = (char*) malloc(100);
  sprintf(topic_cmd, "AT+CMQTTTOPIC=0,%d", strlen(nm));
  sendSimATCmd((const char*)topic_cmd);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  sendSimATMsg(nm);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  char* payload_cmd = (char*) malloc(100);
  sprintf(payload_cmd, "AT+CMQTTPAYLOAD=0,%d", strlen(msg));

  sendSimATCmd((const char*)payload_cmd);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  sendSimATMsg(msg);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  sendSimATCmd("AT+CMQTTPUB=0,1,60");
  vTaskDelay(50 / portTICK_PERIOD_MS);
  // char* res = getMqttResponse();
  // // +CMQTTPUB: 0,0
  // ESP_LOGI(TAG, "Read : '%s' \n", res);
}

void call_sim_spam_task(void *arg){

// sendSimATCmd("AT+CMGF=1");
// sendSimATCmd("AT+CMSS=1");

  while(1){
    sendSimATCmd("ATD01099819709;");
    ESP_LOGI(TAG, "CALL\n");
    
    vTaskDelay(7500 / portTICK_PERIOD_MS);
    sendSimATCmd("AT+CHUP");
    sendSimATCmd("AT&F");
    ESP_LOGI(TAG, "HNAG UP\n");
  }
  vTaskDelete(NULL);
}

void tx_task(void *arg){

  while(1){
    sendSimATCmd("AT+GMR");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void rx_task(void *arg){
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

      // String aa = "aa";

      // TOPIC Receive
      if(strstr((const char*) rx_buffer, "+CMQTTRXPAYLOAD: 0") && strstr((const char*) rx_buffer, "+CMQTTRXEND: 0")){
        char* pch = NULL;

        pch = strtok((char *)rx_buffer, "\r");
        // printf("%s\n", pch);
        pch = strtok(NULL, "\r");
        // printf("%s\n", pch);
        pch = strtok(NULL, "\r");
        char* topicNm = replaceAll(pch, "\r", "");
        topicNm = trim(topicNm);
        // printf("%s\n", pch);
        pch = strtok(NULL, "\r");
        // printf("%s\n", pch);
        pch = strtok(NULL, "\r");
        char* payLoad = replaceAll(pch, "\r", "");
        payLoad = rtrim(ltrim(payLoad));

        printf("topic : %s , %d / payLoad : %s , %d \n", topicNm, strlen((const char*) topicNm), payLoad, strlen((const char*) payLoad));
        // payLoad = replaceAll(payLoad, "\n", "");

        // if(!strstr((const char*) topicNm, "OK") && strlen((const char *) payLoad) > 0){
        //   // send_topic_mqtt("test/1234", (const char*) topicNm);
        //   // vTaskDelay(100 / portTICK_PERIOD_MS);
        //   // send_topic_mqtt("test/1234", (const char*) trim(payLoad));
        // }

        if(strncmp((const char *) topicNm, "test/1234", strlen((const char *) topicNm)) == 0){
          send_topic_mqtt("test/1234", (const char*) topicNm);
          vTaskDelay(100 / portTICK_PERIOD_MS);
          send_topic_mqtt("test/1234", (const char*) payLoad);

          if(strncmp((const char *) payLoad, "hello aa", strlen((const char *) payLoad)) == 0){
            vTaskDelay(50 / portTICK_PERIOD_MS);
            send_topic_mqtt("test/1234", "hello aa i'm aab");
          }
        }
      }

      // if (strstr((const char*)rx_buffer, "OK")) {
      // }else if (strstr((const char*)rx_buffer, "ERROR")) {
      // }

      // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, rx_buffer, rxBytes, ESP_LOG_INFO);
    }
  }

  free(rx_buffer);
  vTaskDelete(NULL);
}

  // init_sim();
  // xTaskCreate(&rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
  // send_topic_mqtt("test/1234", "abcasd");
