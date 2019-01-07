#pragma once

#include <Arduino.h>

const uint32_t serialBaudrate = 115200;

enum ESPMessage {
	
	ESPMES_PLACEHOLDER,

	ESPMES_SET_CONFIG,		// SAM->ESP, Sends new config
	ESPMES_GET_NETINFO,		// SAM->ESP, ESP return network info
	ESPMES_GET_TIME,		// SAM->ESP, ESP returns epoch time
	ESPMES_MQTT_HELLO,		// SAM->ESP, ESP publish Hello and returns result
	ESPMES_MQTT_PUBLISH,		// SAM->ESP, ESP publish readings and returns results
	ESPMES_CONNECT, 		// SAM->ESP, ESP trys wifi conection
	ESPMES_START_AP, 		// SAM->ESP, ESP starts AP
	ESPMES_STOP_AP, 		// SAM->ESP, ESP stops AP
	ESPMES_LED_OFF, 		// SAM->ESP, ESP turns off led (esud before sleep)
	ESPMES_MQTT_INVENTORY, 		// SAM->ESP, ESP publish inventory and returns result
	ESPMES_MQTT_INFO, 		// SAM->ESP, ESP publish info and return result
	ESPMES_MQTT_CUSTOM, 		// SAM->ESP, ESP publish custom message on custom topic

	ESPMES_COUNT
};

enum SAMMessage {

	SAMMES_PLACEHOLDER,

	SAMMES_BOOTED, 			// ESP->SAM, On finished booting
	SAMMES_DEBUG,			// ESP->SAM, Send debug info
	SAMMES_NETINFO,			// ESP->SAM, Send network info
	SAMMES_WIFI_CONNECTED,		// ESP->SAM, On wifi succesfull conection
	SAMMES_SSID_ERROR,		// ESP->SAM, On ssid not found
	SAMMES_PASS_ERROR,		// ESP->SAM, On wrong password
	SAMMES_WIFI_UNKNOWN_ERROR,	// ESP->SAM, On wifi unknown error
	SAMMES_TIME,			// ESP->SAM, Epoch time
	SAMMES_MQTT_HELLO_OK,		// ESP->SAM, On MQTT hello OK
	SAMMES_MQTT_PUBLISH_OK,		// ESP->SAM, On MQTT publish ok
	SAMMES_MQTT_PUBLISH_ERROR, 	// ESP->SAM, On MQTT publish error
	SAMMES_MQTT_INFO_OK, 		// ESP->SAM, On MQTT info publish ok
	SAMMES_MQTT_INFO_ERROR, 	// ESP->SAM, On MQTT info publish error
	SAMMES_MQTT_CUSTOM_OK, 		// ESP->SAM, On MQQT custom publish OK
	SAMMES_MQTT_CUSTOM_ERROR,	// ESP->SAM, On MQQT custom publish error
	SAMMES_SET_CONFIG,		// ESP->SAM, Sends new config

	SAMMES_COUNT
};

#define NETPACK_TOTAL_SIZE 60
#define NETPACK_CONTENT_SIZE (NETPACK_TOTAL_SIZE - 1)	// 60 - 1 = 59 bytes

#define NETBUFF_SIZE (NETPACK_CONTENT_SIZE * 10)
#define JSON_BUFFER_SIZE (NETBUFF_SIZE - 1)				// 1 for the command

#define SAM_ADDRESS 1
#define ESP_ADDRESS 2

struct VersionInt { uint8_t mayor; uint8_t minor; uint8_t build; };

VersionInt parseVersionStr(String versionStr);

