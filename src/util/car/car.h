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
        uint16_t battery = 0;

};


#endif