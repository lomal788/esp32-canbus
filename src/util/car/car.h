#ifndef _Car_H_
#define _Car_H_

#include <nvs_flash.h>
#include <nvs.h>

class Car {
    public:
        Car();
        ~Car();
    private:
        bool ingition = false;
        bool keyfob = false;
        uint16_t batt = 0;
        int outTemp = 0;
        int inTemp = 0;
        int setTemp = 0;

};


#endif