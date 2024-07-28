#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "driver/uart.h"
#include <driver/gpio.h>
#include <driver/adc.h>

#include <esp_system.h>
#include <esp_event.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "util/sim_7600.h"
#include "util/http_server.h"
#include "util/mqtt_manage.h"
#include "util/wifi_config.h"
#include "util/car/twai_base.h"
#include "util/car/car.h"
#include "my_config.h"

// #include <stdlib.h>
// #include "esp_attr.h"
// #include "soc/timer_group_struct.h"
// #include "soc/timer_group_reg.h"
// #include "driver/spi_master.h"

#include <can.h>
#include <mcp2515.h>
#include <driver/spi_master.h>


#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_INTERRUPT 21

bool SPI_Init(void) {
	printf("Hello from SPI_Init!\n\r");
	esp_err_t ret;
	//Configuration for the SPI bus
	// spi_bus_config_t bus_cfg2;

	// // memset(&bus_cfg2, 0, sizeof(bus_cfg2));

	// bus_cfg2.mosi_io_num=PIN_NUM_MOSI;
	// bus_cfg2.miso_io_num=PIN_NUM_MISO;
	// bus_cfg2.sclk_io_num=PIN_NUM_CLK;
	// bus_cfg2.quadwp_io_num=-1;
	// bus_cfg2.quadhd_io_num=-1;
	// bus_cfg2.max_transfer_sz = 0; // no limit
	// // bus_cfg2.flags = SPICOMMON_BUSFLAG_NATIVE_PINS;
	// bus_cfg2.intr_flags = ESP_INTR_FLAG_IRAM;
	// };

	spi_bus_config_t buscfg = {
            .mosi_io_num=PIN_NUM_MOSI,
            .miso_io_num=PIN_NUM_MISO,
            .sclk_io_num=PIN_NUM_CLK,
            .quadwp_io_num=-1,
            .quadhd_io_num=-1,
			.max_transfer_sz = 0,
            // .flags = SPICOMMON_BUSFLAG_NATIVE_PINS
			// .intr_flags = ESP_INTR_FLAG_IRAM,
    };

	// Define MCP2515 SPI device configuration
	// spi_device_interface_config_t dev_cfg;
	// dev_cfg.clock_speed_hz = 40000000; // 10mhz
	// dev_cfg.mode = 0; // (0,0)
	// dev_cfg.spics_io_num = PIN_NUM_CS;
	// dev_cfg.queue_size = 1024;

	spi_device_interface_config_t dev_cfg = {
		.mode = 0, // (0,0)
		.clock_speed_hz = 4*1000*1000, // 4 mhz
		.spics_io_num = PIN_NUM_CS,
		.queue_size = 1024
	};
	
	// dev_cfg.flags = 0;

	// printf("%s on core %d", __func__, xPortGetCoreID());
	// gpio_set_pull_mode(PIN_NUM_MISO, GPIO_PULLUP_ONLY);
    // gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
    // gpio_set_pull_mode(PIN_NUM_CLK, GPIO_PULLUP_ONLY);
    // gpio_set_pull_mode(PIN_NUM_CS, GPIO_PULLUP_ONLY);

	// };

	// Initialize SPI bus
	printf("%s on core %d", __func__, xPortGetCoreID());

	printf("Start Setup\n");
	ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
	if (ret == ESP_OK) {
		printf("Calling Setup\n");

		// Add MCP2515 SPI device to the bus
		ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &MCP2515_Object->spi);
		if (ret == ESP_OK) {
			printf("Calling Setup\n");
		} else {
			printf("Failed to start driver : %d \n", ret);
		}
	}else {
		printf("Failed to initialize SPI : %d \n", ret);
	}
	// ESP_ERROR_CHECK(ret);


	return true;
}

extern "C" void app_main(void) {
	printf("%s on core %d", __func__, xPortGetCoreID());

  // wifi_connection();
  // server_initiation();

  // twai_can_hal = new Car("CAN_BASE", 20, 500000);

  // if (!twai_can_hal->begin_tasks()) {
  //   printf("can task err");

  // }

  MCP2515_init();
  SPI_Init();
  MCP2515_reset();
  MCP2515_setBitrate(CAN_500KBPS, MCP_8MHZ);
  MCP2515_setNormalMode();

  while(1){
	struct can_frame frame;
	frame.can_id = 0x329;
	frame.can_dlc = 4;
	frame.data[0] = 0xFF;
	frame.data[1] = 0xFF;
	frame.data[2] = 0xFF;
	frame.data[3] = 0xFF;

	MCP2515_sendMessage(TXBn_t::TXB0, &frame);
	// if(MCP2515_sendMessageAfterCtrlCheck(can_frame_rx[0]) != ERROR_OK){
	// 	ESP_LOGE(TAG, "Couldn't send message.");
	// }
	vTaskDelay(10); // check freertos tickrate for make this delay 1 second
	// CAN_FRAME frame;

	// uint8_t irq = MCP2515_getInterrupts();

	// if (irq & CANINTF_RX0IF) {
	// 	if (MCP2515_readMessage(RXB0, &frame) == ERROR_OK) {
	// 		printf("get data %lu - %d \n" , frame.can_id,(unsigned char) frame.can_dlc);
	// 		// frame contains received message
	// 	}
	// }

	// if (irq & CANINTF_RX1IF) {
	// 	if (MCP2515_readMessage(RXB1, &frame) == ERROR_OK) {
	// 		printf("get data2 %lu - %d \n" , frame.can_id,(unsigned char) frame.can_dlc);
	// 		// frame contains received message	
	// 	}
	// }


// 	// MCP2515_readMessage
  }



  // init_twai();

  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 5, NULL);

  // mqtt_start();

  // int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);

  // if (data_read > 0) {
  // }

//   init_sim();
  // init_mqtt();
  // connect_mqtt_server();
  // subscribe_mqtt("test/1234");

  // xTaskCreate(&rx_task, "mqtt_sim_rx_task", 1024 * 8, NULL, 6, NULL);
  // xTaskCreate(&call_sim_spam_task, "call_spam_task", 1024*4, NULL, 5, NULL);

  // init_twai();
  // xTaskCreate(&twai_rx_task, "can_rx_task", 8192, NULL, 6, NULL);
}
