#ifndef _CAR_H_
#define _CAR_H_

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

typedef union {
    uint64_t raw;
    uint8_t bytes[8];
    struct {
        bool CF_Clu_ParkBrakeSw: 1;
        bool KEY_FOB: 1;
        bool R_M_STAT: 1;
        bool SWI_IGK: 1;
        bool F_N_ENG: 1;
        uint8_t CF_Clu_IGNSw: 3;
        uint8_t VB: 8;
        uint16_t RPM: 16;
        uint16_t R_M_E_TIME: 16;
        uint16_t R_M_L_TIME: 16;
    } __attribute__((packed));
} CAR_INFO;

typedef union {
    uint64_t raw;
    uint8_t bytes[8];
    struct {
        uint32_t CF_Clu_Odometer: 24;
		uint8_t CR_Fatc_OutTemp: 8;
		uint8_t CR_Datc_DrTempDispC: 8;
        uint32_t CUR_GR: 4;
    } __attribute__((packed));
} CAR_INFO2;

class LTE_MODEM;

class Car: public BaseCan{
    public:
        explicit Car(const char* name, uint8_t tx_time_ms, uint32_t baud);
        CAR_SATUS_ENUM car_status = CAR_SATUS_ENUM::IDLE; // 0 off, 1 acc , 2 on

    protected:
        void on_begin_task_done() override;
        void tx_frames(uint8_t bus) override;
        void on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t rawData[8]) override;
        void on_rx_done(uint64_t now_ts) override;
        void relay_handle(int relayType);
        void setKeyFobStatus(bool status);
        void setCarInfo();
        void sendCarInfo();
        
        [[noreturn]]
        void car_task_loop(void);

        static void start_car_task_loop(void *_this) {
            static_cast<Car*>(_this)->car_task_loop();
        }

    private:
        LTE_MODEM* LTE;
        ECU_JERRY ecu_jerry = ECU_JERRY();
        TaskHandle_t car_task = nullptr;
        bool keyfob = false;
        bool remote_start = false;
        uint64_t remote_start_time = 0;
        uint64_t remote_expire_time = 0;
        uint64_t car_info_send_last_time = 0;
        bool ingition = false;
        uint16_t batt = 0;
        int outTemp = 0;
        int inTemp = 0;
        int setTemp = 0;
        uint8_t fuel_remain_rate = 0;
        uint8_t counter = 0;

        CLU2_CAN clu2Data = {0};
        CLU1_CAN clu1Data = {0};
        EMS11_CAN ems11Data = {0};
        FATC_CAN fatcData = {0};
        DATC12_CAN datc12Data = {0};
        TCU2_CAN tcu2Data = {0};
        
        CAR_INFO carInfo = {0};
        CAR_INFO2 carInfo2 = {0};
        
};

extern Car* twai_can_hal;

#endif
