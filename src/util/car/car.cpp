#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "my_config.h"
#include "driver/uart.h"
#include <driver/adc.h>
#include <driver/gpio.h>
#include <driver/twai.h>
#include "car.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>
#include "esp_timer.h"
#include "../sim_7600.h"

#define TAG "main"

Car* twai_can_hal = nullptr;

Car::Car(const char* name, uint8_t tx_time_ms, uint32_t baud) : BaseCan(name, tx_time_ms, baud) {
  tx.extd = 0;
  tx.rtr = 0;
  tx.ss = 0; // Always single shot
  tx.self = 0;
  tx.dlc_non_comp = 0;

  // Set KeyFob
  // gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);

  // ACC
  // GPIO_MODE_OUTPUT
  // 22 GPIO WORK
  // gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT_OUTPUT_OD);
  // ON
  // gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT_OD);
  // Start Button
  // gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT_OD);
  // KeyFob
  // gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT_OD);
  // gpio_set_level(GPIO_NUM_22, 1);
  // vTaskDelay(500 / portTICK_PERIOD_MS);

  this->LTE = new LTE_MODEM();

}

void Car::on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t rawData[8]) {
  uint64_t now = esp_timer_get_time() / 1000;

  // printf(id+" "+data+" "+ timestamp +"\n");
  // ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "CAN Rx : "+id + " " + timestamp);

  printf("%lx", id);
  printf(" val = %" PRIx64, data);
  printf(" time Stamp = 0x%" PRIx64 "\n", timestamp);

  if (this->ecu_jerry.import_frames(data, id, timestamp)) {
    SAS11_CAN sasData;
    EMS14_CAN ems14Data;
    CLU2_CAN clu2Data;
    EMS11_CAN ems11Data;

    this->ecu_jerry.GET_SAS11_DATA(now, 9999999 ,&sasData);
    this->ecu_jerry.GET_EMS14_DATA(now, 9999999 ,&ems14Data);
    this->ecu_jerry.GET_CLU2_DATA(now, 9999999 ,&clu2Data);
    this->ecu_jerry.GET_EMS11_DATA(now, 9999999 ,&ems11Data);

    printf("STEER ANGLE : %f \n", (int16_t) reverse_bytes(sasData.SAS_ANGLE) * 0.1);
    printf("VB : %f \n", ems14Data.VB * 0.1015625);
    printf("CF_Clu_IGNSw : %d \n", clu2Data.CF_Clu_IGNSw);
    printf("SWI_IGK : %d , F_N_ENG : %d , RPM : %f  \n", ems11Data.SWI_IGK, ems11Data.F_N_ENG, reverse_bytes(ems11Data.RPM) * 0.25);
    // "%" PRIu32 
  }

  // DATC12 AC
  if (id == 0x042) {
    // this->inTemp = (rawData[0] * 0.5) + 14;
    this->inTemp = rawData[0];
  } else if (id == 0x0350) {
    // this->outTemp = (rawData[4] * 0.5) - 40;
    this->outTemp = rawData[4];
  }

  // OBD PID
  // rawData[0] & 0xF0
  if (id == 0x07BB && rawData[0] == 0x21 ) {
    this->inTemp = (rawData[3] / 2) - 40;
    this->outTemp = (rawData[4] / 2) - 40;

    printf("inTemp : %d, OutTemp = %d \n", this->inTemp, this->outTemp);
  } else if (id == 0x7E7 && rawData[2] == 0x2F ) { // 0x7E7
    this->fuel_remain_rate = (rawData[3] * 100) / 255;

    printf("fuel Rate : %d \n",this->fuel_remain_rate);
  } else if (id == 0x7E7 && rawData[2] == 0x42 ) { // 0x7E7 , 42 or 24 pid
    this->batt = rawData[3] / 1000;

    printf("batt : %d \n",this->fuel_remain_rate);
  }

  // Rx Obd Pid
  // if(id == 0x326){
  //   if(data[0] == 0x11){
  //     if(data[1] == 0x13){
  //     }
  //   }
  // }

  // EMS11
  // if(id == 0x316){
  //   // data 1byte 45 == then Ignition
  //   // this->ignition = true;
  // }else if(id == 0x690){ // CLU2
  //   // 00 off, 02 ACC, 03 ON, 04 Cranking
  //   // this->car_status = ;
  // }else if(id == 0x545){ // EMS14
  //   // VB
  //   // this->batt = ;
  // }


}

void Car::on_begin_task_done() {
  printf("begin task done \n");
  
    // if(ShifterStyle::TRRS == VEHICLE_CONFIG.shifter_style) {
    //     (static_cast<ShifterTrrs*>(shifter))->update_shifter_position(now_ts);
    // }
    if (this->car_task == nullptr) {
        ESP_LOG_LEVEL(ESP_LOG_INFO, this->name, "Starting Car task");
        // xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
        if (xTaskCreate(this->start_car_task_loop, "CAR_TASK_LOOP", 8192, this, 6, &this->car_task) != pdPASS) {
            ESP_LOG_LEVEL(ESP_LOG_ERROR, this->name, "Car task creation failed!");
        }
    }

  // this->LTE = new LTE_MODEM(&twai_can_hal);
}

[[noreturn]]
void Car::car_task_loop() {
  uint64_t now = esp_timer_get_time() / 1000;

  while(1){
    now = esp_timer_get_time() / 1000;

    if(this->LTE->carControlState == CarControlState::REMOTE_START){
      this->LTE->carControlState = CarControlState::IDLE;
      this->car_status = CAR_SATUS_ENUM::R_START_BEGIN;
    }else if(this->LTE->carControlState == CarControlState::ENGIN_OFF){
      this->remote_expire_time = 0;
      this->car_info_send_last_time = 0;
      this->LTE->carControlState = CarControlState::IDLE;
      this->car_status = CAR_SATUS_ENUM::R_STARTED;
    }else if(this->LTE->carControlState == CarControlState::EXTEND_TIME){
      // NOT OVER 30 Mins
      if( (this->remote_expire_time + 1000 * 60) < 1000 * 60 * 31 ){
        this->remote_expire_time = this->remote_expire_time + 1000 * 60;
      }
      this->LTE->carControlState = CarControlState::IDLE;
    }else if(this->LTE->carControlState == CarControlState::GET_CAR_STATUS){
      this->LTE->carControlState = CarControlState::IDLE;
      this->car_info_send_last_time = 0;
    }else if(this->LTE->carControlState == CarControlState::KEY_ON){
      this->LTE->carControlState = CarControlState::IDLE;
      this->setKeyFobStatus(true);
    }else if(this->LTE->carControlState == CarControlState::KEY_OFF){
      this->LTE->carControlState = CarControlState::IDLE;
      this->setKeyFobStatus(false);
    }else if(this->LTE->carControlState == CarControlState::INIT_STATUS){
      this->LTE->carControlState = CarControlState::IDLE;

      this->car_info_send_last_time = 0;
      this->remote_expire_time = 0;
      this->remote_start_time = 0;
      this->remote_start = false;
      gpio_set_level(GPIO_NUM_22, 1);
      this->car_status = CAR_SATUS_ENUM::IDLE;
    }

    // this->setKeyFobStatus(true);
    
    if(this->car_status == CAR_SATUS_ENUM::IDLE){

      this->ecu_jerry.GET_CLU2_DATA(now, 20000, &this->clu2Data);
      // Engine OFF
      if(this->clu2Data.CF_Clu_IGNSw == 0){
        // send data Every 5 Min If Car IGN OFF
        if(now > this->car_info_send_last_time + (1000 * 60 * 10) || this->car_info_send_last_time == 0 ){
          vTaskDelay(100 / portTICK_PERIOD_MS);
          this->setCarInfo();
          this->sendCarInfo();
          this->car_info_send_last_time = now;
        }
      } else {
        // send data Every 2 Min
        if(now > this->car_info_send_last_time + (1000 * 60 * 2) || this->car_info_send_last_time == 0 ){
          vTaskDelay(100 / portTICK_PERIOD_MS);
          this->setCarInfo();
          this->sendCarInfo();
          this->car_info_send_last_time = now;
        }
      }
      // this->relay_handle(1);
      // vTaskDelay(1000 / portTICK_PERIOD_MS);
      // printf("CAR IS IN IDLE ");
      // this->LTE->mainState = MainState_t::TEST;

      // LTE->uodate_task_status(MainState_t::TEST);

      // Car State Topic
    }else if(this->car_status == CAR_SATUS_ENUM::R_START_BEGIN){
      this->ecu_jerry.GET_EMS11_DATA(now, 500, &this->ems11Data);
      this->LTE->send_topic_mqtt("/device/asd123/resp", "ENG/1/RUN");

      // If Car is not running
      if(reverse_bytes(this->ems11Data.RPM) * 0.25 > 300.0){
        this->car_status = CAR_SATUS_ENUM::IDLE;
      } else {
        // Data Recieved Car Start Process Begin
        this->setKeyFobStatus(true);
        this->keyfob = true;
        this->remote_start = true;
        // WAKE UP
        this->relay_handle(1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        // PLEASE PRESS BUTTON ONE MORE TIME
        this->relay_handle(1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        // MAKE ACC
        this->relay_handle(1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        // TRYING TO START CAR / ACC -> ON STATE
        gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(GPIO_NUM_22, 0);
        this->car_status = CAR_SATUS_ENUM::R_STARTING;
      }
    }else if(this->car_status == CAR_SATUS_ENUM::R_STARTING){
      this->ecu_jerry.GET_EMS11_DATA(now, 500, &this->ems11Data);
      this->ecu_jerry.GET_CLU2_DATA(now, 20000, &this->clu2Data);

      printf("\n");
      printf("RPM RAW : %llu\n", this->ems11Data.raw);
      // printf("RPM RAW : %d\n", this->ems11Data.RPM);
      printf("RPM : %f\n", reverse_bytes(this->ems11Data.RPM) * 0.25);
      printf("\n");

      if(reverse_bytes(this->ems11Data.RPM) * 0.25 > 300.0 &&
        this->clu2Data.CF_Clu_IGNSw != 0
        ){
        gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(GPIO_NUM_22, 1);
        // Default 15 Mins Car
        this->remote_expire_time = 1000 * 60 * 15 ;
        // this->remote_expire_time = 5000 ;
        this->remote_start_time = esp_timer_get_time() / 1000;
        this->LTE->send_topic_mqtt("/device/asd123/resp", "ENG/1");
        this->car_info_send_last_time = 0;
        this->car_status = CAR_SATUS_ENUM::R_STARTED;
      }
    }else if(this->car_status == CAR_SATUS_ENUM::R_STARTED){
      // printf("Engine IS RUNNING\n");

      if(now > this->car_info_send_last_time + (1000 * 60 * 2) || this->car_info_send_last_time == 0 ){
        this->setCarInfo();
        this->sendCarInfo();
        this->car_info_send_last_time = now;
      }

      if(now > this->remote_start_time + this->remote_expire_time ){
        printf("Engine Stopping Proccess\n");
        this->LTE->send_topic_mqtt("/device/asd123/resp", "ENG/0/RUN");
        // engine stop Status Start
        this->relay_handle(1);
        this->setKeyFobStatus(false);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        this->relay_handle(1);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        this->car_status = CAR_SATUS_ENUM::R_STOPING;
      }

      // Car State Topic
    }else if(this->car_status == CAR_SATUS_ENUM::R_STOPING){
      this->ecu_jerry.GET_CLU2_DATA(now, 500, &this->clu2Data);
      if(this->clu2Data.CF_Clu_IGNSw == 0){
        if(this->keyfob){
          this->setKeyFobStatus(false);
        }
        this->car_status = CAR_SATUS_ENUM::R_STOPED;
      } else {
        this->ecu_jerry.GET_EMS11_DATA(now, 500, &this->ems11Data);
        if(reverse_bytes(this->ems11Data.RPM) * 0.25 > 300){
          this->setKeyFobStatus(false);
          this->relay_handle(1);
        } else if(this->clu2Data.CF_Clu_IGNSw == 2){
          // ACC
          this->setKeyFobStatus(false);
          this->relay_handle(1);
          vTaskDelay(100 / portTICK_PERIOD_MS);
          this->relay_handle(1);
        }
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }else if(this->car_status == CAR_SATUS_ENUM::R_STOPED){
      this->LTE->send_topic_mqtt("/device/asd123/resp", "ENG/0");
      this->car_info_send_last_time = 0;
      this->remote_expire_time = 0;
      this->remote_start_time = 0;
      this->remote_start = false;
      gpio_set_level(GPIO_NUM_22, 1);
      this->car_status = CAR_SATUS_ENUM::IDLE;
      // Car Stop Data Send

    }else if(this->car_status == CAR_SATUS_ENUM::M5){
      printf("m5 State \n");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      this->remote_start = false;
      this->LTE->carControlState = CarControlState::IDLE;
      this->car_status = CAR_SATUS_ENUM::IDLE;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void Car::tx_frames(uint8_t bus) {

  // tx.identifier = 0xAAAA;
  // tx.extd = 1;
  // tx.data_length_code = 4;
  // for (int i = 0; i < 4; i++) {
  //     tx.data[i] = 0;
  // }
  char tx_dataaa[8] = {0x1,0x2,0x3,0x4,0xf,0xa,0xc,0x1d};
  CLU2_CAN clu2Data;
  this->ecu_jerry.GET_CLU2_DATA(esp_timer_get_time() / 1000, 9999999 ,&clu2Data);

  // tx.data = clu2Data.bytes;
  to_bytes(clu2Data.raw, tx.data);

  // for (int i=0; i < 8; i++) {
  //   // if(i == 1){
  //   //   // uint8_t srccc = dest.CRC_ENG_RQ2_TCM & 0xFF;
  //   //   // srccc >>= 8;
  //   //   // uint32_t val = (dest.CRC_ENG_RQ2_TCM[0] << 16) + (dest.CRC_ENG_RQ2_TCM[1] << 8) + dest.CRC_ENG_RQ2_TCM[2];
  //   //   // print()
  //   //   tx.data[i] = 0x00;
  //   // }else {
  //   // printf("%d", i);
  //   if(i == 3){
  //     tx.data[i] = this->alive_cnt;
  //   }else{
  //     tx.data[i] = 0x01;
  //   }
  // }

  //  tx.data[2] = 0x11;
  // printf("%d", sizeof(tx_dataaa));
  // printf("%d \n",alive_cnt);

  tx.identifier = 0x0218;
  // tx.data_length_code = sizeof(tx_dataaa);
  tx.data_length_code = 8;
  // to_bytes(dest.raw, tx.data);

  // to_bytes(eng_rq1_tcm_tx.raw, tx_can.data);
  // twai_transmit(&tx, 5);

  // Base 50 msg per Second

  if(counter == 50){
    counter = 0;
  }

  // Should be 20 Hz but I decided to send 17hz
  if ( counter % 3 == 0) {
  // if ( counter % 3 == 0 && this->remote_start) {
    // esp_err_t can_tx_result = twai_transmit(&tx, 5);
    // this->remote_start = false;
    // printf("%d \n",can_tx_result);
    // if(can_tx_result != 0){
    //   printf("Fail to Send Can msg %d \n",can_tx_result);
    // }
  }

  // 1hz
  if ( counter % 50 == 0) {
    // uint8_t twai_message_t::data[8];
    // tx.identifier = ECUID;
    // tx.data_length_code = 3;
    // tx.data[0] = 0x7F;
    // tx.data[1] = sid;
    // tx.data[2] = nrc;

    // esp_err_t can_tx_result = twai_transmit(&tx, 5);
    if(reverse_bytes(this->ems11Data.RPM) * 0.25 > 300.0
    ){
      // // IN, OUT TEMP
      // tx.identifier = 0x07B3;
      // tx.data_length_code = 8;
      // // tx.data = {0x22, 0x01, 0x22, 0x01, 0, 0, 0, 0};
      // tx.data[0] = 0x22;
      // tx.data[1] = 0x01;
      // tx.data[2] = 0x22;
      // tx.data[3] = 0x01;
      // tx.data[4] = 0x00;
      // tx.data[5] = 0x00;
      // tx.data[6] = 0x00;
      // tx.data[7] = 0x00;
      // twai_transmit(&tx, 5);

      // // IN, OUT TEMP
      // tx.identifier = 0x07DF;
      // tx.data_length_code = 8;
      // // tx.data = {0x02, 0x01, 0x2F, 0, 0, 0, 0, 0};
      // tx.data[0] = 0x02;
      // tx.data[1] = 0x01;
      // tx.data[2] = 0x2F;
      // tx.data[3] = 0x00;
      // tx.data[4] = 0x00;
      // tx.data[5] = 0x00;
      // tx.data[6] = 0x00;
      // tx.data[7] = 0x00;
      // twai_transmit(&tx, 5);

      // // BATT
      // tx.identifier = 0x07DF;
      // tx.data_length_code = 8;
      // // tx.data = {0x02, 0x01, 0x2F, 0, 0, 0, 0, 0};
      // tx.data[0] = 0x02;
      // tx.data[1] = 0x01;
      // tx.data[2] = 0x42;
      // // 42 or 24 pid
      // tx.data[3] = 0x00;
      // tx.data[4] = 0x00;
      // tx.data[5] = 0x00;
      // tx.data[6] = 0x00;
      // tx.data[7] = 0x00;
      // twai_transmit(&tx, 5);
      
    }
  }

  
  counter++;
}

void Car::on_rx_done(uint64_t now_ts) {
    // if(ShifterStyle::TRRS == VEHICLE_CONFIG.shifter_style) {
    //     (static_cast<ShifterTrrs*>(shifter))->update_shifter_position(now_ts);
    // }
}

// void make_car_ign(){
//   this->remote_expire_time = esp_timer_get_time() + (1000000 * 60 * 15 ); // 15Min Default
// }

// void add_car_ign(uint64_t ts){
//   this->remote_expire_time += (1000000 * ts )
// }

/*
0x010 DoorId
*/

static void test(void *arg){

  // 100, 501 B캔 잠금
  // gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  // gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);

  // digitalWrite(switchPin1, LOW); // 배터리 전원인가
  // delay(3000);
  // digitalWrite(switchPin2, LOW); // 락버튼 1회 누름
  // delay(1000);
  // digitalWrite(switchPin2, HIGH); // 락버튼 뗌
  // delay(1000);
  // digitalWrite(switchPin2, LOW); // 락버튼 1회 누름
  // delay(1000);
  // digitalWrite(switchPin2, HIGH); // 락버튼 뗌
  // delay(4000);
  // digitalWrite(switchPin1, HIGH); // 배터리 전원끊음
}

// 1 ACC 2 ACC ON, 3 ACC AND START, 4 ALL OFF
void Car::relay_handle(int relayType){
  // Start Button
  gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT_OD);
  // KeyFob
  // gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT_OD);

  if(this->keyfob == false || this->remote_start == false){
    // return;
  }

  if(relayType == 1){
    printf("ACC on\n");
    gpio_set_level(GPIO_NUM_22, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_22, 1);
  }else if(relayType == 2){
    printf("ACC ON\n");
    gpio_set_level(GPIO_NUM_22, 0);
    // printf("ON ON\n");
    // gpio_set_level(GPIO_NUM_18, 1);
  }else if(relayType == 3){
    printf("ACC ON\n");
    // gpio_set_level(GPIO_NUM_10, 1);
    gpio_set_level(GPIO_NUM_22, 0);
    printf("TRYING TO START CAR\n");
    // gpio_set_level(GPIO_NUM_18, 0);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_22, 1);
  }else if(relayType == 4){
    printf("Relay off\n");
    gpio_set_level(GPIO_NUM_22, 1);
  }
}

void Car::setKeyFobStatus(bool status){
  gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT_OD);
  
  if(status){
    printf("Smart Key on\n");
    gpio_set_level(GPIO_NUM_23, 1);
    this->keyfob = true;
  }else{
    printf("Smart Key off\n");
    gpio_set_level(GPIO_NUM_23, 0);
    this->keyfob = false;
  }
}

void Car::sendCarInfo(){
  char* msg = (char*)malloc(100);
  uint8_t rawData[7];
  to_bytes(this->carInfo.raw, rawData);
  sprintf(msg, "1_%02x%02x%02x%02x%02x%02x%02x%02x", rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5], rawData[6], rawData[7]);
  this->LTE->send_topic_mqtt("/device/asd123/status",(const char*) msg);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  to_bytes(this->carInfo2.raw, rawData);
  sprintf(msg, "2_%02x%02x%02x%02x%02x%02x%02x%02x", rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5], rawData[6], rawData[7]);
  this->LTE->send_topic_mqtt("/device/asd123/status",(const char*) msg);

  free(msg);
}

void Car::setCarInfo(){
  EMS14_CAN ems14Data;
  uint64_t now = esp_timer_get_time() / 1000;
  uint16_t remoteETime = 0;
  uint16_t remoteLTime = 0;

  if(this->remote_start){
    if(this->remote_expire_time > 999){
      remoteETime = this->remote_expire_time;
    }

    if(this->remote_start_time + this->remote_expire_time - now > 0){
      remoteLTime = this->remote_start_time + this->remote_expire_time - now;
    }
  }

  this->ecu_jerry.GET_EMS14_DATA(now, 60000 ,&ems14Data);
  this->ecu_jerry.GET_CLU2_DATA(now, 60000 ,&this->clu2Data);
  this->ecu_jerry.GET_EMS11_DATA(now, 60000 ,&this->ems11Data);
  this->ecu_jerry.GET_CLU1_DATA(now, 60000 ,&this->clu1Data);

  this->carInfo.CF_Clu_ParkBrakeSw = this->clu1Data.CF_Clu_ParkBrakeSw;
  this->carInfo.SWI_IGK = this->ems11Data.SWI_IGK;
  this->carInfo.F_N_ENG = this->ems11Data.F_N_ENG;
  this->carInfo.RPM = reverse_bytes(this->ems11Data.RPM);
  // this->carInfo.RPM = reverse_bytes(this->ems11Data.RPM) * 0.25;
  this->carInfo.CF_Clu_IGNSw = this->clu2Data.CF_Clu_IGNSw;
  // this->carInfo.VB = ems14Data.VB * 0.1015625;
  this->carInfo.VB = ems14Data.VB;
  this->carInfo.KEY_FOB = this->keyfob;
  this->carInfo.R_M_STAT = this->remote_start;
  this->carInfo.R_M_E_TIME = remoteETime;
  this->carInfo.R_M_L_TIME = remoteLTime;

  this->ecu_jerry.GET_DATC12_DATA(now, 60000 ,&this->datc12Data);
  this->ecu_jerry.GET_FATC_DATA(now, 60000 ,&this->fatcData);

  this->carInfo2.CR_Fatc_OutTemp = this->fatcData.CR_Fatc_OutTemp;
  this->carInfo2.CR_Datc_DrTempDispC = this->datc12Data.CR_Datc_DrTempDispC;
  this->carInfo2.CF_Clu_Odometer = this->clu1Data.CF_Clu_Odometer;

}

// adc_batt = adc_channel_t::ADC_CHANNEL_8
// adc_channel_t adc_batt;

// esp_err_t Sensors::read_vbatt(uint16_t *dest){
//     int v = 0;
//     int read = 0;
//     esp_err_t res = adc_oneshot_read(adc2_handle, pcb_gpio_matrix->sensor_data.adc_batt, &read);
//     res = adc_cali_raw_to_voltage(adc2_cal, read, &v);
//     if (res == ESP_OK) {
//         // Vin = Vout(R1+R2)/R2
//         *dest = v * 5.54; // 5.54 = (100+22)/22
//     }
//     return res;
// }

// CH 0 GPIO 36
// + 100k ohm to 36 pin
// between gnd, + 16k ohm
// max 24v , float analogRead(36) / 4096 * 24 * (15942 / 16000)
// analogRead(36) / 4093 * 30 * 1000
// https://ohmslawcalculator.com/voltage-divider-calculator
static void get_battery_voltage(){
  int average = 0;

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);

  while(1){
    for (int i = 0; i < 10; i++) average = average + adc1_get_raw(ADC1_CHANNEL_0);
    average = average / 10;

    // average = average / 4093 * 30 * 1000;

    // 5 Min
    vTaskDelay((1000 * 60 * 5) / portTICK_PERIOD_MS);
  }
}
