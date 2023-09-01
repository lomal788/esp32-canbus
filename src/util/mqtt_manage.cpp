#include <mqtt_client.h>
#include "mqtt_manage.h"
#include "my_config.h"

// esp_mqtt_client_handle_t mqtt_client;
static bool mqtt_connected = false;
esp_mqtt_client_handle_t mqtt_client;

void mqtt_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  int msg_id;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t client = event->client;

  switch (event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
      printf("Mqtt Before connecting ... \n");
      break;
    case MQTT_EVENT_CONNECTED:
      printf("Mqtt connected ... \n");
      mqtt_connected = true;

      msg_id = esp_mqtt_client_subscribe(client, "testtopic/centralno", 0);
      printf("sent subscribe successful, msg_id=%d", msg_id);

      break;
    case MQTT_EVENT_DISCONNECTED:
      printf("Mqtt lost connection ... \n");
      mqtt_connected = false;
      break;
    case MQTT_EVENT_DATA:
      printf("Mqtt got Data ... \n\n");
      printf("TOPIC=%d , %.*s\r\n", event->topic_len, event->topic_len, event->topic);
      printf("DATA=%d , %.*s\r\n", event->data_len, event->data_len, event->data);
      // char * rx_topic = event->topic;
      // char * rx_data = event->data;
      // if(rx_topic == 'testtopic/centralno') {
      // }
      break;
    default:
      break;
    }

}

static void mqtt_task_loop(void *arg){
    while (1) {
      if(mqtt_connected == true){
        char * mqtt_topic = "testtopic/test";
        char * mqtt_msg = "test";
        esp_mqtt_client_publish(mqtt_client, mqtt_topic, mqtt_msg, (int)strlen(mqtt_msg), 0, 0);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
      }else{
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }
}

void mqtt_start(){
  const esp_mqtt_client_config_t mqtt_cfg = {
      .broker = {
        .address = {
          .uri = MQTT_SERVER_IP,
          .port = MQTT_SERVER_PORT,
        }
        // .verification.certificate = (const char *)mqtt_eclipse_org_pem_start,
      },
      .credentials = {
        // .username = "",
        // .client_id = MQTT_CLIENT_ID,
      },
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, mqtt_client);
  esp_mqtt_client_start(mqtt_client);

  xTaskCreate(&mqtt_task_loop, "mqtt_task", 8192, NULL, 5, NULL);

}
