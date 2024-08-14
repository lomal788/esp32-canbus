#ifndef _Twai_H_
#define _Twai_H_

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <driver/twai.h>
#include <driver/gpio.h>

#define TWAI_BUS_SIZE 2

class BaseCan {
    public:
        BaseCan(const char* name, uint8_t tx_time_ms, uint32_t baud);
        ~BaseCan();
        bool begin_tasks();
        esp_err_t init_state() const;
        uint8_t aabbcc = 0;
        twai_handle_t twai_handler[2];
        twai_handle_t twai_handler2;
        // uint8_t TWAI_BUS_SIZE = 2;

    protected:
        const char* name;
        

        TaskHandle_t tx_task = nullptr;
        TaskHandle_t rx_task = nullptr;
        // TaskHandle_t tx_task = nullptr;
        // TaskHandle_t rx_task = nullptr;
        uint8_t tx_time_ms = 0;

        // gpio_num_t can_tx_pin[TWAI_BUS_SIZE] = {GPIO_NUM_5, GPIO_NUM_23};
        // gpio_num_t can_rx_pin[TWAI_BUS_SIZE] = {GPIO_NUM_4, GPIO_NUM_22};
        gpio_num_t can_tx_pin[TWAI_BUS_SIZE] = {GPIO_NUM_26, GPIO_NUM_23};
        gpio_num_t can_rx_pin[TWAI_BUS_SIZE] = {GPIO_NUM_27, GPIO_NUM_22};


        uint16_t diag_tx_id = 0;
        uint16_t diag_rx_id = 0;

        [[noreturn]]
        void tx_task_loop(void);
        [[noreturn]]
        void rx_task_loop(void);

        static void start_rx_task_loop(void *_this) {
            static_cast<BaseCan*>(_this)->rx_task_loop();
        }
        static void start_tx_task_loop(void *_this) {
            static_cast<BaseCan*>(_this)->tx_task_loop();
        }

        static void start_rx_task_loop_bus0(void *_this) {
            static_cast<BaseCan*>(_this)->rx_task_loop();
        }
        static void start_tx_task_loop_bus0(void *_this) {
            static_cast<BaseCan*>(_this)->tx_task_loop();
        }

        virtual void on_begin_task_done(){};
        virtual void tx_frames(uint8_t bus){};
        virtual void on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t bus) {};
        virtual void on_rx_done(uint64_t now_ts){};
        
        bool send_messages[TWAI_BUS_SIZE] = {true, true};
        QueueHandle_t* diag_rx_queue;
        twai_status_info_t can_status;
        esp_err_t can_init_status;
        twai_message_t tx;
        uint8_t alive_cnt = 0;

        inline void to_bytes(uint64_t src, uint8_t* dst) {
            for(uint8_t i = 0; i < 8; i++) {
                dst[7-i] = src & 0xFF;
                src >>= 8;
            }
        };

        inline uint16_t reverse_bytes(uint16_t value) {
            return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
        };
};

extern BaseCan* twai_can_hal;

#endif
