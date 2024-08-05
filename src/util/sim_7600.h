#ifndef SIM_7600
#define SIM_7600

#include <nvs_flash.h>
#include <nvs.h>
#define UART UART_NUM_2


typedef enum
{
    MODE_START = 0,
    MODE_INIT,
    MODE_CONNECTING,
    MODE_CONNECTED,
    MODE_RECONNECT_INIT,
}MainState_t;

static MainState_t mainState;

void modem_reset();
void init_sim();
void begin_tasks22();
void init_mqtt();
void connect_mqtt_server();
void sendSimATMsg(const char* cmd);
void sendSimATCmd(const char* cmd);

void send_topic_mqtt(const char* nm, const char* msg);
void subscribe_mqtt(const char* nm);

void call_sim_spam_task(void *arg);
void rx_task(void *arg);
void tx_task(void *arg);
void status_task(void *arg);
void rx_mqtt_msg(const char* topicNm, const char* payLoad);


#endif
