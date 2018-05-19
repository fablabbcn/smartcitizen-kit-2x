#pragma once

const uint32_t serialBaudrate = 230400;

enum ESPMessage {

	ESPMES_SET_CONFIG,		// SAM->ESP, Sends new config
	ESPMES_GET_NETINFO,		// SAM->ESP, ESP return network info
	ESPMES_GET_TIME,		// SAM->ESP, ESP returns epoch time
	ESPMES_MQTT_HELLO,		// SAM->ESP, ESP publish Hello and returns result
	ESPMES_MQTT_PUBLISH,		// SAM->ESP, ESP publish readings and returns results
	ESPMES_START_AP, 		// SAM->ESP, ESP starts AP
	ESPMES_STOP_AP, 		// SAM->ESP, ESP stops AP

	ESPMES_COUNT
};

enum SAMMessage {

	SAMMES_SET_CONFIG,		// ESP->SAM, Sends new config
	SAMMES_DEBUG,			// ESP->SAM, Send debug info
	SAMMES_NETINFO,			// ESP->SAM, Send network info
	SAMMES_WIFI_CONNECTED,		// ESP->SAM, On wifi succesfull conection
	SAMMES_SSID_ERROR,		// ESP->SAM, On ssid not found
	SAMMES_PASS_ERROR,		// ESP->SAM, On wrong password
	SAMMES_WIFI_UNKNOWN_ERROR,	// ESP->SAM, On wifi unknown error
	SAMMES_TIME,			// ESP->SAM, Epoch time
	SAMMES_MQTT_HELLO_OK,		// ESP->SAM, On MQTT hello OK
	SAMMES_MQTT_PUBLISH_OK,		// ESP->SAM, On MQTT publish ok

	SAMMES_COUNT
};

enum NetPacks { TOTAL_PARTS = 0 };
#define NETPACK_TOTAL_SIZE 60
#define NETPACK_CONTENT_SIZE (NETPACK_TOTAL_SIZE - 1)	// 60 - 1 = 59 bytes

#define NETBUFF_SIZE (NETPACK_CONTENT_SIZE * 8)
#define JSON_BUFFER_SIZE (NETBUFF_SIZE - 1)				// 1 for the command

#define SAM_ADDRESS 1
#define ESP_ADDRESS 2
