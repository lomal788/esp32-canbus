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

class BaseCan {
    public:
        BaseCan(const char* name, uint8_t tx_time_ms, uint32_t baud);
        ~BaseCan();
        bool begin_tasks();
        esp_err_t init_state() const;

    protected:
        const char* name;
        TaskHandle_t tx_task = nullptr;
        TaskHandle_t rx_task = nullptr;
        uint8_t tx_time_ms = 0;
        gpio_num_t can_tx_pin = GPIO_NUM_5;
        gpio_num_t can_rx_pin = GPIO_NUM_4;
        

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

        virtual void tx_frames(){};
        virtual void on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp) {};
        virtual void on_rx_done(uint64_t now_ts){};
        void twai_rx_task(void *arg);

        bool send_messages = true;
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
        }
};

extern BaseCan* twai_can_hal;

#endif
