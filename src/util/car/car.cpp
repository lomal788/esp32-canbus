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

#define TAG "main"

Car::Car(const char* name, uint8_t tx_time_ms, uint32_t baud) : BaseCan(name, tx_time_ms, baud) {

  tx.extd = 0;
  tx.rtr = 0;
  tx.ss = 0; // Always single shot
  tx.self = 0;
  tx.dlc_non_comp = 0;
}

void Car::on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t bus) {
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
    esp_err_t can_tx_result = twai_transmit(&tx, 5);
    this->remote_start = false;
    // printf("%d \n",can_tx_result);
    // if(can_tx_result != 0){
    //   printf("Fail to Send Can msg %d \n",can_tx_result);
    // }
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
  // ACC
  // GPIO_MODE_OUTPUT
  gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT_OD);
  // ON
  gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT_OD);

  if(this->keyfob == false || this->remote_start == false){
    return;
  }

  if(relayType == 1){
    printf("ACC on\n");
    gpio_set_level(GPIO_NUM_16, 0);
  }else if(relayType == 2){
    printf("ACC ON\n");
    gpio_set_level(GPIO_NUM_16, 0);
    printf("ON ON\n");
    gpio_set_level(GPIO_NUM_18, 0);
  }else if(relayType == 3){
    printf("ACC ON\n");
    gpio_set_level(GPIO_NUM_16, 0);
    printf("ACC ON\n");
    gpio_set_level(GPIO_NUM_18, 0);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_18, 1);
  }else if(relayType == 4){
    printf("Relay off\n");
    gpio_set_level(GPIO_NUM_18, 1);
  }
}

void Car::setKeyFobStatus(int status){
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  
  if(status == 1){
    printf("Smart Key on\n");
    gpio_set_level(GPIO_NUM_21, 1);
    this->keyfob = true;
  }else{
    printf("Smart Key off\n");
    gpio_set_level(GPIO_NUM_21, 0);
    this->keyfob = false;
  }
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
