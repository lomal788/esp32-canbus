#ifndef SIM_7600
#define SIM_7600

#include <nvs_flash.h>
#include <nvs.h>

static void rx_task(void *event_handler_arg);
static void tx_task(void *event_handler_arg);
void init_sim();

#endif