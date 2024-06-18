#ifndef SIM_7600
#define SIM_7600

#include <nvs_flash.h>
#include <nvs.h>
#define UART UART_NUM_2


void init_sim();
void init_mqtt();
void connect_mqtt_server();
void sendSimATMsg(const char* cmd);
void sendSimATCmd(const char* cmd);

void send_topic_mqtt(const char* nm, const char* msg);
void subscribe_mqtt(const char* nm);

void call_sim_spam_task(void *arg);
void rx_task(void *arg);
void tx_task(void *arg);

#endif
