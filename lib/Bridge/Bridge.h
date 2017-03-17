#pragma once

#include <EasyTransfer.h>
#define ONE_DAY_IN_SECONDS 86400

enum EspCommand {
	ESP_NULL,
	//						------ Events
	ESP_BOOTED_AND_READY,
	ESP_WIFI_CONNECTED_EVENT,
	ESP_WIFI_ERROR_EVENT,
	ESP_WIFI_ERROR_PASS_EVENT,
	ESP_WIFI_ERROR_AP_EVENT,
	ESP_PING_OK_EVENT,
	ESP_PING_ERROR_EVENT,
	ESP_MQTT_HELLO_OK_EVENT,
	ESP_MQTT_PUBLISH_OK_EVENT,
	ESP_MQTT_ERROR_EVENT,
	ESP_TIME_FAIL_EVENT,
	ESP_TIME_UPDATED_EVENT,
	ESP_AP_ON_EVENT,
	ESP_AP_OFF_EVENT,
	ESP_WEB_ON_EVENT,
	ESP_WEB_OFF_EVENT,
	ESP_FACTORY_RESET_EVENT,
	ESP_CONF_NOT_CHANGED_EVENT,
	ESP_CONF_CHANGED_EVENT,
	// 						------ Configuration
	ESP_WIFI_CONNECT_COM,	//
	ESP_WIFI_DISCONNECT_COM,//
	ESP_GET_WIFI_COM,		// @params uint_8t (the index of the network to retrieve)
	ESP_GET_BEST_WIFI_COM,	// Get best configured network
	ESP_SET_WIFI_COM,		// @params String ssid, String password
	ESP_CLEAR_WIFI_COM,			// @params uint8_t index (the index of the network to be cleared)
	ESP_GET_IP_COM,
	ESP_SET_TOKEN_COM,		// @params String Token
	ESP_GET_TOKEN_COM,
	ESP_SET_CONF_COM,		// @params struct Configuration
							// @return bool: true if configuration saved, false otherwise
	ESP_GET_CONF_COM,		// @return struct Configuration
	//						------ Control
	ESP_START_AP_COM,
	ESP_STOP_AP_COM,
	ESP_START_WEB_COM,
	ESP_STOP_WEB_COM,
	ESP_DEEP_SLEEP_COM,
	//						------ Get data
	ESP_GET_APCOUNT_COM,
	ESP_GET_APLIST_COM,		// @return String apList (json formatted)
	ESP_GET_TIME_COM,		// @return String: epoch time
	ESP_MQTT_CONNECT_COM,	// @params int persistent flag
	ESP_MQTT_HELLOW_COM,
	ESP_MQTT_PUBLISH_COM,	// @params String payload, int QoS
	ESP_MQTT_SYNC_COM,
	//						------ State
	ESP_GET_STATUS_COM,		// @return struct espStatus:
							//		-- wifi: ESP_WIFI_CONNECTED, ESP_WIFI_ERROR, ESP_WIFI_ERROR_PASS, ESP_WIFI_ERROR_AP
							//		-- net:	ESP_PING_OK, ESP_PING_ERROR
							//		-- mqtt: ESP_MQTT_HELLO_OK, ESP_MQTT_PUBLISH_OK, ESP_MQTT_ERROR
							//		-- time: ESP_TIME_FAIL, ESP_TIME_UPDATED
							//		-- ap: ESP_AP_ON, ESP_AP_OFF
							//		-- web: ESP_WEB_ON, ESP_WEB_OFF
							//		-- conf: ESP_CONF_NOT_CHANGED, ESP_CONF_CHANGED
	ESP_GET_LAST_EVENT_COM,	// @return espEvents

	ESP_SERIAL_DEBUG_ON,	// Turn on serial debug output
	ESP_SERIAL_DEBUG_OFF,	// Turn off serial debug output
	ESP_LED_OFF,			// Turn off both leds
	ESP_LED_ON				// Turn on both leds
};

// puede ser que el error tenga que ver con mezclar tipos dentro del struct
struct BUS_Serial {
	uint32_t time = 0;					// epoch time
	uint8_t com;
	char param[240];
	bool waitAnswer = false;
};

struct Credentials {
	uint32_t time = 0;			// epoch time
	char ssid[64] = "ssid";
	char password[64] = "password";
};

struct ESPstatus {
	uint8_t wifi = ESP_NULL;
	uint8_t net = ESP_NULL;
	uint8_t mqtt = ESP_NULL;
	uint8_t time = ESP_NULL;
	uint8_t ap = ESP_NULL;
	uint8_t web = ESP_NULL;
	uint8_t conf = ESP_NULL;
};

struct Configuration {
	uint32_t readInterval = 60;
};