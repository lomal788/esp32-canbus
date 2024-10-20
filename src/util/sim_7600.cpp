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
#include "car/car.h"

#include "esp_timer.h"
#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "car/car.h"
#include "car/twai_base.h"

#define TAG "main"

static const int RX_BUF_SIZE = 1024 * 2;
#define MAX_STR_LEN 4000

#define TXD_PIN (GPIO_NUM_25)
#define RXD_PIN (GPIO_NUM_14)

// #define UART UART_NUM_2

// LTE_MODEM::LTE_MODEM(Car** car_can_hal) {
LTE_MODEM::LTE_MODEM() {
  // this->car_can_hal = car_can_hal;

  this->init_sim();
}

LTE_MODEM::~LTE_MODEM() {
}

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
 begin = s;

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

char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
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

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token) {
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
    if (a > len) a = len;
    if (b < a) b = a;

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

void LTE_MODEM::init_sim(){
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
  // ESP_ERROR_CHECK(uart_set_pin(UART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_set_pin(UART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));


  // Modem Reset
  // void Network::modemReset() { // Modem will go for Hard Reset. The same function can be used to Switch ON Modem after a Shutdown or as warm reboot.
	// int resetPin = 23;
	// pinMode(resetPin, OUTPUT);
	// digitalWrite(resetPin, HIGH);
  // 	delay(1000);
  // 	digitalWrite(resetPin, LOW);
	// Serial.println("Modem Reset Complete.");
  // modem Reset


  // AT+COPS=0 // SET NetworkAuto RES OK
  // AT+CREG? // is network Attached RES +CREG 1,1 / 1,5 also work else not attached

  // AT+CGSN // get IMEI RES OK with  1~16 IMEI num
  // AT+CGPADDR // GET IP 
  // AT+QSPN // GET CURRENT SERVICE PROVIDER

  // AT+CSQ // CHECK LTE SIGNAL

  // AT+MQTTCONN= // CONNECT MQTT RES OK TIMEOUT 10000
  // AT+MQTTPUB= // PUBLISH MSG RES OK TIMEOUT 10000
  // AT+MQTTSUBUNSUB= // SUBSCRIBE RES OK TIMEOUT 10000
  // AT+MQTTDISCONN // DISCONNECT MQTT


  // AT+CGPS=1, 1 / AT+CGPS=1 / AT+CGPSCOLD / AT+CGPSHOT // START GPS  RES OK
  // AT+CGNSSINFO // GET GPS DATA RES +CGNSSINFO: 2, 06, 03, 00, 3426.693019, S, 15051.184731, E, 170521, 034216.0, 46.5, 0.0, 0.0, 1.2, 0.9, 0.9
  // AT+CGREG? // GET NETWORK STATUS
  // AT+CGREG=1 // REGISTER TO NETWORK
  // AT+CFUN=6 // RESET DEVICE
  // AT+CRESET // RESET DEVICE


  //HTTP REQUEST
  // AT+HTTPINIT // Init HTTP service
  // AT+HTTPPARA="CID", 1 // Set parameters for HTTP session
  // AT+HTTPPARA="URL", "www.sim.com" // Set parameters for HTTP session
  // HTTPACTION : 0 GET, 1 POST
  // AT+HTTPACTION=0 // GET session start / GET successfully RES OK / +HTTPACTION: 0, 200, 1000	
  // AT+HTTPREAD // Read the data of HTTP server RES +HTTPREAD: 1000 / OK	
  // AT+HTTPTERM // Terminate HTTP service


  // AT COMMAND REFRENCE
  // https://github.com/cavli-wireless/P32C1RM-Arduino/blob/master/lib/C1RMCore.cpp
  // https://github.com/Jasons531/4G_SIM7600/blob/master/Src/sim7600.c
  // https://github.com/pomarrc/Lte_SIM7600/blob/main/src/Lte_SIM7600.cpp
  // https://github.com/Nafih-SA/ESP32_SIM7600-GSM-MQTT-User-Experience/blob/master/src/main.cpp

  // MQTT 
  // https://github.com/IndustrialArduino/SIM7600_MQTT/blob/main/SIM7600_MQTT_using_AT_commands/README.md
  // https://github.com/DenilsonGomes/mqtt_SIM7600/blob/main/mqtt_SIM7600.cpp
  //https://github.com/shiftops/FreematicsSim7600MQTT/blob/master/lib/MQTTClientSIM7600/MQTTClientSIM7600.cpp


  // EXAMPLE
  // AT+IPR=9600 // BandRate 9600
  // ATE0
  // AT
  // ATE0
  // INIT
  // AT+CNMP=?
  // AT+CNMP=38
  // AT+COPS=4,2,"45005"  //SKT


  // Network.SerialInit();
  // pinMode(ledPin, OUTPUT);
  // while (!Network.isModemAvailable()) {
  //   if (!modemWait) {
  //     Serial.print("Waiting for modem.");
  //     modemWait = true;
  //   } else {
  //     Serial.print(".");
  //   }
  // }
  // Serial.println("Modem Ready");
  // delay(10000);
  // Serial.println(Network.getModemInfo());
  // Serial.println(Network.getIMEI());
  // Serial.println(Network.getICCID());

  // while (!Network.isNetworkAttached()) {
  //   if (!networkWait) {
  //     Serial.print("Waiting for Network.");
  //     networkWait = true;
  //   } else {
  //     delay(500);
  //     Serial.print(".");
  //   }
  // } Serial.println("Registered to Network");
  // if (Network.setPDN(3, "hubblethings.io")) {
  //   Serial.println(Network.getDefaultPDN());
  // }

  // while (!Network.enablePacketData(true)) {
  //   if (!dataWait) {
  //     Serial.print("Waiting for GPRS.");
  //     dataWait = true;
  //   } else {
  //     delay(500);
  //     Serial.print(".");
  //   }
  // }
  // if (Network.getPacketDataStatus()) {
  //   Serial.println("Packet Data Attached");
  //   Serial.println(Network.getIPAddr());
  //   Serial.println(Network.getDNSAddr());
  //   //    Serial.println("Ping IP:"+Network.getPingStatus("google.com").addr+Network.getPingStatus("google.com").stats);
  // }
  // Serial.println("RF Strength=" + Network.getRadioQuality().csq + ", RSSI=" + Network.getRadioQuality().rssi + ", BER=" + Network.getRadioQuality().ber);


//  if((Network.createMQTT("broker.mqttdashboard.com", "1883", "1111", "60", "0", "test_123", "123__4"))==1) 
//  Serial.println("MQTT Connection Successful!!");
//  else
//  Serial.println("MQTT Connection Unsuccessful!!");
//  while(!(Network.publishMQTT("Hubble", "Cavli-R&D-KOCHI", "0", "0", "0"))) {
//  } Serial.println("MQTT Message Published Successfully");
//  while (!(Network.subscribeMQTT("Hubble123", "0", false))) {
//  } Serial.println("MQTT Unsubscription Successfull");
//  while (!Network.subscribeMQTT("Hubble123", "0", true)) {
//  } Serial.println("MQTT Subscription Successfull");
//  while (!(Network.disconnectMQTT())) {
//  } Serial.println("MQTT Connection Disconnected Successfully");

  // vTaskDelay(5000 / portTICK_PERIOD_MS);

  // EXAMPLE
  // uint8_t* rx_buffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
  // const int rxBytes = uart_read_bytes(UART, rx_buffer, RX_BUF_SIZE, 500 / portTICK_PERIOD_MS);
  
  // if (rxBytes > 0) {
  //   rx_buffer[rxBytes] = 0;
  //   ESP_LOGI("RX_TASK_TAGAA", "Read %d bytes: '%s'\n", rxBytes, rx_buffer);
  // }

  // if ( (strncmp((const char*) rx_buffer, "AT", 2)) == 0 && strncmp((const char*) rx_buffer, "OK", 2) == 0 ) {
  //   printf("AT OKAY!!");
  //   // commandid_reply = REPLY_OK;
  //   // Usart1SendData_DMA("at\r\n", strlen("at\r\n"));
  // }


  // this->sendSimATCmd("AT+CPIN?");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+COPS?");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CPSI?");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPS=0");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // // this->sendSimATCmd("AT+CGNSSMODE=15,1");
  // // vTaskDelay(100 / portTICK_PERIOD_MS);
  // // this->sendSimATCmd("AT+CGPSNMEA=200191");
  // // vTaskDelay(100 / portTICK_PERIOD_MS);
  // // this->sendSimATCmd("AT+CGPSNMEARATE=1");
  // // vTaskDelay(100 / portTICK_PERIOD_MS);
  
  // this->sendSimATCmd("AT+CVAUXV=3050");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CVAUXS=1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CVAUXS=0");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPS=1,1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // // this->sendSimATCmd("AT+CGPSINFOCFG=1,31");
  // // vTaskDelay(100 / portTICK_PERIOD_MS);

  // this->sendSimATCmd("AT+CGPS=?");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPSINFO");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  
  
  

  // uart_intr_config_t uart_intr = {
  //     .intr_enable_mask = UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL,
  //     .rxfifo_full_thresh = 20,
  //     // .rx_timeout_thresh = 20,
  // };

  // uart_intr_config(UART, &uart_intr);
  // uart_enable_rx_intr(UART);
  
  this->begin_tasks();
}

void LTE_MODEM::begin_tasks(){
  printf("START SIM INIT \n");
  // this->mainState = MODE_START;
  
  xTaskCreate(&this->start_rx_task, "uart_rx_task", 1024*8, this, 5, &this->lte_rx_task);
  xTaskCreate(&this->start_status_task, "uart_status_task", 1024*4, this, 5, &this->lte_status_task);
  // vTaskDelay(100 / portTICK_PERIOD_MS);

  // this->sendSimATCmd("AT+CGNSSMODE=15,1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPSNMEA=200191");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPSNMEARATE=1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  
  // this->sendSimATCmd("AT+CVAUXV=3050");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CVAUXS=1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CVAUXS=0");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CGPS=1,1");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
}

// void http_req(){
//   this->sendSimATCmd("AT+HTTPINIT");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   this->sendSimATCmd("AT+HTTPPARA=\"URL\", \"https://webhook.site/7f2c7a86-54ed-43c6-8ac3-eec8dad29d76\"");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   this->sendSimATCmd("AT+HTTPACTION=0");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   this->sendSimATCmd("AT+HTTPREAD?");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   this->sendSimATCmd("AT+HTTPREAD=0,140");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   this->sendSimATCmd("AT+HTTPTERM");
//   vTaskDelay(100 / portTICK_PERIOD_MS);
// }
// AT+CPIN?

void LTE_MODEM::init_mqtt(){
  
  // this->sendSimATCmd("AT+CPSI?");
  this->sendSimATCmd("AT+CGREG?");
  this->sendSimATCmd("ATE0");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CIPMODE=1");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CSOCKSETPN=1");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("AT+CIPMODE=0");
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+NETOPEN");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+NETOPEN?");
  // this->sendSimATCmd("AT+CIPMODE=0");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CMQTTCONNECT?");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CMQTTSTART");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CMQTTACCQ=0,\"asd123\"");
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

void LTE_MODEM::connect_mqtt_server(){
  this->sendSimATCmd("AT+CMQTTCONNECT=0,\"tcp://mt.lomal788.us:1883\",60,1,\"asd123\",\"asd\"");
  // this->sendSimATCmd("AT+CMQTTCONNECT=0,\"tcp://broker.mqtt.cool:1883\",60,1,\"asd123\",\"asd\"");
  // this->sendSimATCmd("AT+CMQTTCONNECT=0,\"tcp://mqtt-dashboard.com:8884\",60,1");
  
  vTaskDelay(500 / portTICK_PERIOD_MS);
  
  this->sendSimATCmd("AT+CMQTTCONNECT?");
  // this->sendSimATCmd("AT+CMQTTSUB=0,9,1,1");
  vTaskDelay(100 / portTICK_PERIOD_MS);
  // this->sendSimATCmd("test/1234"); // \x1A -> close
}

void LTE_MODEM::sendSimATMsg(const char* cmd){
  char* Txdata = (char*) malloc(100);

  sprintf(Txdata, "%s",cmd);
  const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
  if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
  free(Txdata);
}

void LTE_MODEM::sendSimATCmd(const char* cmd){
  char* Txdata = (char*) malloc(100);

  sprintf(Txdata, "%s\r\n",cmd);
  const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
  // if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
  free(Txdata);
}

// void this->sendSimATCmd(const char* cmd, uint32_t timeout_ms){
//   char* Txdata = (char*) malloc(100);
//   uint64_t startMillis = esp_timer_get_time();

//   sprintf(Txdata, "%s\r\n",cmd);
//   const int lengthaa = uart_write_bytes(UART, Txdata, strlen(Txdata));
//   // if(lengthaa > 0) ESP_LOGI(TAG, "sent %d\n", lengthaa);
//   free(Txdata);

//   while (esp_timer_get_time() - startMillis < timeout_ms){
//     uint8_t* rx_buffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
//     const int rxBytes = uart_read_bytes(UART, rx_buffer, RX_BUF_SIZE, 500 / portTICK_PERIOD_MS);

//     if (rxBytes > 0) {
//       rx_buffer[rxBytes] = 0;
//       return 
//     }

//   }
// }

void LTE_MODEM::rx_mqtt_msg(const char* topicNm, const char* payLoad){
  printf("topic : %s , %d / payLoad : %s , %d \n", topicNm, strlen(topicNm), payLoad, strlen(payLoad));
  // payLoad = replaceAll(payLoad, "\n", "");

  // if(!strstr((const char*) topicNm, "OK") && strlen((const char *) payLoad) > 0){
  //   // send_topic_mqtt("test/1234", (const char*) topicNm);
  //   // vTaskDelay(100 / portTICK_PERIOD_MS);
  //   // send_topic_mqtt("test/1234", (const char*) trim(payLoad));
  // }

  if(strncmp(topicNm, "/device/asd123", strlen(topicNm)) == 0){
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // send_topic_mqtt("test/1234", (const char*) payLoad);
    
    if(strncmp(payLoad, "hello aa", strlen(payLoad)) == 0){
      // vTaskDelay(50 / portTICK_PERIOD_MS);
      send_topic_mqtt("/device/asd123/resp", "hello aa i'm aab");
    }else if(strncmp(payLoad, "HTTP REQUEST", strlen(payLoad)) == 0){
      // vTaskDelay(50 / portTICK_PERIOD_MS);
      // http_req();
    }else if(strncmp(payLoad, "GET GPS", strlen(payLoad)) == 0){
      this->sendSimATCmd("AT+CGPSINFO?");
      // vTaskDelay(50 / portTICK_PERIOD_MS);
      this->sendSimATCmd("AT+CGPSINFO");
      // vTaskDelay(100 / portTICK_PERIOD_MS);
      
      // http_req();
    }else if(strncmp(payLoad, "RESET DEVICE", strlen(payLoad)) == 0){
      this->mainState = MainState_t::MODE_RECONNECT_INIT;
    }else if(strncmp(payLoad, "HTTP_REQ", strlen(payLoad)) == 0){
      // http_req();
    }else if(strncmp(payLoad, "CAN SEND", strlen(payLoad)) == 0){
      
      // Car->remote_start = true;
      // this->mainState = MODE_RECONNECT_INIT;
      // this->sendSimATCmd("AT+CGPSINFO?");
      // vTaskDelay(50 / portTICK_PERIOD_MS);
      // this->sendSimATCmd("AT+CGPSINFO");
      // vTaskDelay(100 / portTICK_PERIOD_MS);
      
      // http_req();
    }else if(strncmp(payLoad, "ENG_ON", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::REMOTE_START;
    }else if(strncmp(payLoad, "ENG_OFF", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::ENGIN_OFF;
    }else if(strncmp(payLoad, "EXT_TIME", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::EXTEND_TIME;
    }else if(strncmp(payLoad, "CAR_STAT", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::GET_CAR_STATUS;
    }else if(strncmp(payLoad, "KEY_ON", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::KEY_ON;
    }else if(strncmp(payLoad, "KEY_OFF", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::KEY_OFF;
    }else if(strncmp(payLoad, "RESET", strlen(payLoad)) == 0){
      this->carControlState = CarControlState::INIT_STATUS;
    }
    else{
      send_topic_mqtt("/device/asd123/resp", (const char*) topicNm);
    }
    
  }
}

void LTE_MODEM::subscribe_mqtt(const char* nm){
  char* sub_cmd = (char*) malloc(100);

  sprintf(sub_cmd, "AT+CMQTTSUB=0,%d,1", strlen(nm));
  this->sendSimATCmd((const char*) sub_cmd);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  this->sendSimATMsg(nm);
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

void LTE_MODEM::send_topic_mqtt(const char* nm, const char* msg){

  if(this->mainState != MainState_t::MODE_CONNECTED){
    return;
  }
  
  printf("data I receive : %s \n", msg);

  char* topic_cmd = (char*) malloc(100);
  sprintf(topic_cmd, "AT+CMQTTTOPIC=0,%d", strlen(nm));
  this->sendSimATCmd((const char*)topic_cmd);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  this->sendSimATMsg(nm);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  char* payload_cmd = (char*) malloc(100);
  sprintf(payload_cmd, "AT+CMQTTPAYLOAD=0,%d", strlen(msg));

  this->sendSimATCmd((const char*)payload_cmd);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  this->sendSimATMsg(msg);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  this->sendSimATCmd("AT+CMQTTPUB=0,1,60");
  vTaskDelay(50 / portTICK_PERIOD_MS);
  // char* res = getMqttResponse();
  // // +CMQTTPUB: 0,0
  // ESP_LOGI(TAG, "Read : '%s' \n", res);
}

void LTE_MODEM::call_sim_spam_task(void *arg){

// this->sendSimATCmd("AT+CMGF=1");
// this->sendSimATCmd("AT+CMSS=1");

  while(1){
    this->sendSimATCmd("ATD01099819709;");
    ESP_LOGI(TAG, "CALL\n");
    
    vTaskDelay(7500 / portTICK_PERIOD_MS);
    this->sendSimATCmd("AT+CHUP");
    this->sendSimATCmd("AT&F");
    ESP_LOGI(TAG, "HNAG UP\n");
  }
  vTaskDelete(NULL);
}

[[noreturn]]
void LTE_MODEM::tx_task_loop(){

  while(1){
    this->sendSimATCmd("AT+GMR");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void LTE_MODEM::modem_reset(){
  // this->mainState = MODEM_COMMAND_MODE;
  this->sendSimATCmd("AT+CRESET");
}

void LTE_MODEM::uodate_task_status(MainState_t status){
  this->mainState = status;
}

[[noreturn]]
void LTE_MODEM::status_task_loop(){
  uint64_t startMillis = esp_timer_get_time() / 1000;
  this->mainState = MainState_t::MODE_RECONNECT_INIT;

  while(1){
    // if((esp_timer_get_time() - startMillis) / 1000 > 10 * 1000 ){
    //   startMillis = esp_timer_get_time();
    //   printf("SEND AT COMMAND\n");
    //   this->sendSimATCmd("AT");
    // }

    switch (this->mainState) {
      case MainState_t::TEST:
        printf("TEST MODE!!!");
        this->mainState = MainState_t::MODE_START;
        // this->car_can_hal.car_status = CAR_SATUS_ENUM::M5;

        break;
      case MainState_t::IDLE:
        break;
      case MainState_t::MODE_START:
      // printf("hello");
        if((esp_timer_get_time() / 1000 - startMillis) > 60 * 1000){
          this->mainState = MainState_t::MODE_RECONNECT_INIT;
        }

        // else if((esp_timer_get_time() - startMillis) / 1000 > 5 * 1000){
        //   this->sendSimATCmd("AT+CPSI?");
        // }

        break;
      case MainState_t::MODE_CONNECTING:
          if((esp_timer_get_time() / 1000 - startMillis) > 60 * 1000){
            this->sendSimATCmd("AT+CMQTTCONNECT?");
            startMillis = esp_timer_get_time() / 1000;
          }

        break;
      case MainState_t::MODE_CONNECTED:
          if((esp_timer_get_time() / 1000 - startMillis) > 60 * 1000){
            this->sendSimATCmd("AT+CMQTTCONNECT?");
            startMillis = esp_timer_get_time() / 1000;
          }

        break;
      case MainState_t::MODE_INIT:
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // this->sendSimATCmd("AT+COPS?");
        // vTaskDelay(500 / portTICK_PERIOD_MS);
        this->sendSimATCmd("AT+CPSI?");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        this->sendSimATCmd("AT+CGPS=0");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        this->sendSimATCmd("AT+CGPS=?");
        vTaskDelay(500 / portTICK_PERIOD_MS);

        // startMillis = esp_timer_get_time();
        // if((esp_timer_get_time() - startMillis) / 1000 > 30 * 1000){
        //   this->mainState = MODE_RECONNECT_INIT;
        // }

        // this->mainState = MODE_START;
        break;
      case MainState_t::MODE_RECONNECT_INIT:
        printf("RESET DEVICE\n");
        this->modem_reset();
        startMillis = esp_timer_get_time() / 1000;
        this->mainState = MainState_t::MODE_START;
        break;
      default:
        break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

[[noreturn]]
void LTE_MODEM::rx_task_loop(){
  static const char *RX_TASK_TAG = "RX_TASK";
  esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
  int rxBytes;  
  uint8_t* rx_buffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
  printf("RX_TAKE START \n");

  while(1){
    // const int64_t message_received_us = esp_timer_get_time();
    // int length = 0;
    // ESP_ERROR_CHECK(uart_get_buffered_data_len(UART, (size_t*)&length));
    // if (length) {
    //   ESP_LOGI(TAG, "received %d bytes\n", length);
    // }

    rxBytes = uart_read_bytes(UART, rx_buffer, RX_BUF_SIZE, 0);

    if (rxBytes > 0) {
      rx_buffer[rxBytes] = 0;
      // String aa = "";
      // rx_buffer = replaceAll(rx_buffer,"\r\n", "OK");
      ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'\n", rxBytes, rx_buffer);
      // ESP_LOGI(RX_TASK_TAG, "Readaa %d", rx_buffer[0]);

      // if(rxBytes > 1){
      //   this->sendSimATCmd("AT");
      // }
      if(this->mainState == MainState_t::IDLE){
        printf("Change Car State to M5");
        // if(this->car_can_hal->car_status == CAR_SATUS_ENUM::IDLE){
        //   this->car_can_hal->car_status = CAR_SATUS_ENUM::M5;
        // }

      }else if(this->mainState == MainState_t::MODE_START){
        if(strstr((const char*) rx_buffer, "AT") &&
        strstr((const char*) rx_buffer, "OK") &&
        rxBytes == 9
        ){
          this->sendSimATCmd("AT+CPSI?");
        }else if(rxBytes < 9){
          this->sendSimATCmd("AT");
        }
      }else if(
        strstr((const char*) rx_buffer, "+CME ERROR: SIM busy")
        || strstr((const char*) rx_buffer, "+CME ERROR: SIM not inserted")
        ) {
        // AT+CRESET OK change to MODE START
        // this->sendSimATCmd("AT");
        this->mainState = MainState_t::MODE_RECONNECT_INIT;
      }else if(strstr((const char*) rx_buffer, "RDY")){
        this->mainState = MainState_t::MODE_INIT;
      }else if(strstr((const char*) rx_buffer, "+CPSI: NO SERVICE")) {
        // +CPSI: NO SERVICE,Online
        // +CPSI: NO SERVICE,Unknown
        vTaskDelay(500 / portTICK_PERIOD_MS);
        this->sendSimATCmd("AT+CGPS=0");
        // this->sendSimATCmd("AT");
        // this->mainState = MODE_RECONNECT_INIT;
      }
      if(strstr((const char*) rx_buffer, "PB DONE") ||
      strstr((const char*) rx_buffer, "+CPSI: LTE,Online") ||
      (
        strstr((const char*) rx_buffer, "+COPS: 0") &&
        strstr((const char*) rx_buffer, "OK")
      )
      // strstr((const char*) rx_buffer, "RDY")
      ){
        vTaskDelay(5000 / portTICK_PERIOD_MS);
          this->sendSimATCmd("AT+CFUN=1");
          vTaskDelay(100 / portTICK_PERIOD_MS);
          this->sendSimATCmd("AT+CGACT=1,1");
          vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      if(strstr((const char*) rx_buffer, "AT+CGACT=1,1")){
        init_mqtt();
        this->mainState = MainState_t::MODE_CONNECTING;
      }else if(strstr((const char*) rx_buffer, "+CMQTTCONNLOST")){
        this->connect_mqtt_server();
        this->mainState = MainState_t::MODE_CONNECTING;
      }else if(strstr((const char*) rx_buffer, "+CMQTTCONNECT: 0,13")){
        this->modem_reset();
        this->mainState = MainState_t::MODE_START;
      }else if(strstr((const char*) rx_buffer, "+CMQTTSTART: 23") ||
      strstr((const char*) rx_buffer, "+CMQTTSTART: 0")
      ){
        this->connect_mqtt_server();
      }else if(
          strstr((const char*) rx_buffer, "+CMQTTCONNECT: 0")
        && strstr((const char*) rx_buffer, "+CMQTTCONNECT: 1")
        && !strstr((const char*) rx_buffer, "+CMQTTCONNECT: 0,")
      ){
        this->connect_mqtt_server();
        this->mainState = MainState_t::MODE_CONNECTING;
      }else if(
        (
          strstr((const char*) rx_buffer, "+CMQTTCONNECT: 0,0") ||
          strstr((const char*) rx_buffer, "+CMQTTCONNECT: 0,")
        ) 
        && strstr((const char*) rx_buffer, "+CMQTTCONNECT: 1")
        && this->mainState != MainState_t::MODE_CONNECTED
      ){
        vTaskDelay(100 / portTICK_PERIOD_MS);
        this->subscribe_mqtt("/device/asd123");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        this->mainState = MainState_t::MODE_CONNECTED;
      }else if(strstr((const char*) rx_buffer, "+CGPSINFO: 0")){
        this->sendSimATCmd("AT+CGPS=1,1");
        vTaskDelay(50 / portTICK_PERIOD_MS);
        this->sendSimATCmd("AT+CGPS?");
        vTaskDelay(50 / portTICK_PERIOD_MS);
      }else if(strstr((const char*) rx_buffer, "+CIPEVENT: NETWORK CLOSED") ||
        // strstr((const char*) rx_buffer, "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY") ||
        (
        strstr((const char*) rx_buffer, "+CMQTTCONNECT") &&
        strstr((const char*) rx_buffer, "ERROR")
        )
       ){
        this->mainState = MainState_t::MODE_RECONNECT_INIT;
      }else if(strstr((const char*) rx_buffer, "+CMQTTRXPAYLOAD: 0") && strstr((const char*) rx_buffer, "+CMQTTRXEND: 0")){
      // TOPIC Receive

        char* pch = NULL;

        pch = strtok((char *)rx_buffer, "\r");
        pch = strtok(NULL, "\r");
        pch = strtok(NULL, "\r");
        char* topicNm = replaceAll(pch, "\r", "");
        topicNm = trim(topicNm);
        pch = strtok(NULL, "\r");
        pch = strtok(NULL, "\r");
        char* payLoad = replaceAll(pch, "\r", "");
        payLoad = rtrim(ltrim(payLoad));

        this->rx_mqtt_msg((const char*) topicNm, (const char*) payLoad);
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  free(rx_buffer);
  // vTaskDelete(NULL);
}

  // init_sim();
  // xTaskCreate(&rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
  // send_topic_mqtt("test/1234", "abcasd");

