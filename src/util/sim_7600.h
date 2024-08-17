#ifndef _SIM_7600_H_
#define _SIM_7600_H_

#include <nvs_flash.h>
#include <nvs.h>
// #include "util/car/twai_base.h"
// #include "util/car/car.h"

#define UART UART_NUM_2


enum class MainState_t : uint16_t {
    IDLE = 0,
    MODE_START,
    MODE_INIT,
    MODE_CONNECTING,
    MODE_CONNECTED,
    MODE_RECONNECT_INIT,
    TEST,
};

enum class CarControlState : uint16_t {
    IDLE = 0,
    TEST,
    REMOTE_START,
    ENGIN_OFF,
};

class LTE_MODEM {
    public:
        // LTE_MODEM(Car** car_can_hal);
        LTE_MODEM();
        ~LTE_MODEM();
        void uodate_task_status(const MainState_t status);
        MainState_t mainState = MainState_t::IDLE;
        CarControlState carControlState = CarControlState::IDLE;
        void send_topic_mqtt(const char* nm, const char* msg);
    protected:
        
        void subscribe_mqtt(const char* nm);
        void call_sim_spam_task(void *arg);
        void tx_task(void *arg);
        void rx_mqtt_msg(const char* topicNm, const char* payLoad);

        [[noreturn]]
        void rx_task_loop(void);
        [[noreturn]]
        void status_task_loop(void);

        static void start_rx_task(void *_this) {
            static_cast<LTE_MODEM*>(_this)->rx_task_loop();
        }
        static void start_status_task(void *_this) {
            static_cast<LTE_MODEM*>(_this)->status_task_loop();
        }

        void sendSimATMsg(const char* cmd);
        void sendSimATCmd(const char* cmd);

    private:
        TaskHandle_t lte_rx_task = nullptr;
        TaskHandle_t lte_status_task = nullptr;
        // Car** car_can_hal;

        // CAR_SATUS_ENUM tests = CAR_SATUS_ENUM::IDLE;

        void modem_reset();
        void init_sim();
        void begin_tasks();
        void init_mqtt();
        void connect_mqtt_server();
};

// extern LTE_MODEM* lte_modem;


#endif
