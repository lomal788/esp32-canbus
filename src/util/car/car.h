#ifndef _Car_H_
#define _Car_H_

#include <nvs_flash.h>
#include <nvs.h>
#include "twai_base.h"

class Car: public BaseCan{
    public:
        explicit Car(const char* name, uint8_t tx_time_ms, uint32_t baud);
    protected:
        void tx_frames(uint8_t bus) override;
        void on_rx_frame(uint32_t id,  uint8_t dlc, uint64_t data, uint64_t timestamp, uint8_t bus) override;
        void on_rx_done(uint64_t now_ts) override;
    private:
        bool keyfob = false;
        bool remote_start = false;
        uint32_t remote_start_time = 0;
        uint8_t car_status = 0; // 0 off, 1 acc , 2 on
        bool ingition = false;
        uint16_t batt = 0;
        int outTemp = 0;
        int inTemp = 0;
        int setTemp = 0;
        uint8_t counter = 0;
};

#endif
