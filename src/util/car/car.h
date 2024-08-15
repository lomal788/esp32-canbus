#ifndef _Car_H_
#define _Car_H_

#include <nvs_flash.h>
#include <nvs.h>
#include "twai_base.h"
#include "../sim_7600.h"
#include "../../ecu/src/ECU_CAR.h"

enum class CAR_SATUS_ENUM : uint16_t {
    IDLE = 0,
    R_START_BEGIN = 1,
    R_STARTING = 2,
    R_STARTED = 3,
    R_STOPING = 4,
    R_STOPED = 5,
    M5 = 6,
    M6 = 7,
    M7 = 8,
    M8 = 9,
    M9 = 10,
};

class Car: public BaseCan{
    public:
        explicit Car(const char* name, uint8_t tx_time_ms, uint32_t baud);
    protected:
        void on_begin_task_done() override;
        void tx_frames(uint8_t bus) override;
        void on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t bus) override;
        void on_rx_done(uint64_t now_ts) override;
        void relay_handle(int relayType);
        void setKeyFobStatus(bool status);
        
        [[noreturn]]
        void car_task_loop(void);

        static void start_car_task_loop(void *_this) {
            static_cast<Car*>(_this)->car_task_loop();
        }

    private:
        LTE_MODEM* LTE;
        ECU_JERRY ecu_jerry = ECU_JERRY();
        TaskHandle_t car_task = nullptr;
        CAR_SATUS_ENUM car_status = CAR_SATUS_ENUM::IDLE; // 0 off, 1 acc , 2 on
        bool keyfob = false;
        bool remote_start = false;
        uint64_t remote_expire_time = 0;
        bool ingition = false;
        uint16_t batt = 0;
        int outTemp = 0;
        int inTemp = 0;
        int setTemp = 0;
        uint8_t counter = 0;

        CLU2_CAN clu2Data;
        EMS11_CAN ems11Data;
};

#endif
