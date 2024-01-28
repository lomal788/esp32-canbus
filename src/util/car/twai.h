#ifndef _Twai_H_
#define _Twai_H_

#include <nvs_flash.h>
#include <nvs.h>

// class Twai {
//     public:
//         // Car();
//         // ~Car();
//     private:
// };

void init_twai();
void twai_rx_task(void *arg);

#endif