#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "my_config.h"
#include "driver/uart.h"
#include <driver/adc.h>
#include <driver/gpio.h>
#include "Car.h"

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"

#define TAG "main"


Car::Car() {
}

Car::~Car() {
    // this->gearbox_ref->diag_regain_control(); // Re-enable engine starting
}

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

// Relay Control
static void relay_control_task(void *arg){
  gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
  int level = 0;

  while(1){
    if(level == 1){
      printf("relay on\n");
      gpio_set_level(GPIO_NUM_16, 0);
    }else{
      printf("relay off\n");
      gpio_set_level(GPIO_NUM_16, 1);
    }
    level = !level;

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}


static void key_fob_task(void *arg){
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  int level = 0;

  while(1){
    if(level == 1){
      printf("Smart Key on\n");
      gpio_set_level(GPIO_NUM_21, 0);
    }else{
      printf("Smart Key off\n");
      gpio_set_level(GPIO_NUM_21, 1);
    }
    level = !level;

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}

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
