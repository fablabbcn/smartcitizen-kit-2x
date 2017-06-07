#pragma once

#include <EasyTransfer.h>
#include <Sensors.h>
#define ONE_DAY_IN_SECONDS 86400

enum EspCommand {
	ESP_NULL,
	//						------ Events
	ESP_FACTORY_RESET_EVENT,
	ESP_DEBUG_EVENT,
	// 						------ Configuration
	ESP_WIFI_CONNECT_COM,	//
	ESP_WIFI_DISCONNECT_COM,//
	ESP_GET_WIFI_COM,		// @params uint8_t (the index of the network to retrieve)
	ESP_GET_BEST_WIFI_COM,	// Get best configured network
	ESP_SET_WIFI_COM,		// @params String ssid, String password
	ESP_CLEAR_WIFI_COM,			// @params uint8_t index (the index of the network to be cleared)
	ESP_GET_IP_COM,
	ESP_SET_TOKEN_COM,		// @params String Token
	ESP_GET_TOKEN_COM,
	ESP_CLEAR_TOKEN_COM,
	ESP_SET_CONF_COM,		// @params struct Configuration
							// @return bool: true if configuration saved, false otherwise
	ESP_GET_CONF_COM,		// @return struct Configuration
	//						------ Control
	ESP_START_AP_COM,
	ESP_STOP_AP_COM,
	ESP_START_WEB_COM,
	ESP_STOP_WEB_COM,
	ESP_DEEP_SLEEP_COM,
	ESP_GET_VERSION_COM,
	//						------ Get data
	ESP_GET_APCOUNT_COM,
	ESP_GET_APLIST_COM,		// @return String apList (json formatted)
	ESP_GET_TIME_COM,		// @return String: epoch time
	ESP_SYNC_HTTP_TIME_COM,
	ESP_MQTT_CONNECT_COM,	// @params int persistent flag
	ESP_MQTT_HELLOW_COM,
	ESP_MQTT_PUBLISH_COM,	// @params String payload, int QoS
	ESP_MQTT_SYNC_COM,
	ESP_MQTT_CLEAR_STATUS,
	ESP_GET_FREE_HEAP_COM,

	//						------ State
	ESP_GET_STATUS_COM,		// @return struct espStatus:
							//		-- wifi: ESP_WIFI_CONNECTED_EVENT, ESP_WIFI_ERROR_EVENT, ESP_WIFI_ERROR_PASS_EVENT, ESP_WIFI_ERROR_AP_EVENT
							//		-- net:	ESP_PING_OK, ESP_PING_ERROR
							//		-- mqtt: ESP_MQTT_HELLO_OK, ESP_MQTT_PUBLISH_OK, ESP_MQTT_ERROR_EVENT
							//		-- time: ESP_TIME_FAIL, ESP_TIME_UPDATED
							//		-- ap: ESP_AP_ON, ESP_AP_OFF
							//		-- web: ESP_WEB_ON, ESP_WEB_OFF
							//		-- conf: ESP_CONF_NOT_CHANGED, ESP_CONF_CHANGED
	ESP_WEB_CONFIG_SUCCESS,	//	Something has changed succsesfully via web server
	ESP_GET_LAST_EVENT_COM,	// @return espEvents

	ESP_SERIAL_DEBUG_ON,	// Turn on serial debug output
	ESP_SERIAL_DEBUG_OFF,	// Turn off serial debug output
	ESP_LED_OFF,			// Turn off both leds
	ESP_LED_ON				// Turn on both leds
}; 


// @return struct espStatus:
//	-- wifi: ESP_WIFI_CONNECTED, ESP_WIFI_ERROR, ESP_WIFI_ERROR_PASS, ESP_WIFI_ERROR_AP
#define ESP_WIFI_CONNECTED_EVENT 	1
#define ESP_WIFI_ERROR_EVENT 		2
#define ESP_WIFI_ERROR_PASS_EVENT 	3
#define ESP_WIFI_ERROR_AP_EVENT 	4

// -- net:	ESP_PING_OK, ESP_PING_ERROR
#define ESP_PING_OK_EVENT			1
#define ESP_PING_ERROR_EVENT		2

// -- mqtt: ESP_MQTT_HELLO_OK, ESP_MQTT_PUBLISH_OK, ESP_MQTT_ERROR_EVENT
#define	ESP_MQTT_HELLO_OK_EVENT		1
#define	ESP_MQTT_PUBLISH_OK_EVENT	2
#define	ESP_MQTT_ERROR_EVENT		3

//	-- time: ESP_TIME_FAIL, ESP_TIME_UPDATED
#define ESP_TIME_FAIL_EVENT			1
#define ESP_TIME_UPDATED_EVENT		2

//	-- ap: ESP_AP_ON, ESP_AP_OFF
#define ESP_AP_ON_EVENT				1
#define ESP_AP_OFF_EVENT			2

//	-- web: ESP_WEB_ON, ESP_WEB_OFF
#define ESP_WEB_ON_EVENT			1
#define ESP_WEB_OFF_EVENT			2

//	-- conf: ESP_CONF_NOT_CHANGED, ESP_CONF_CHANGED
#define ESP_CONF_NOT_CHANGED_EVENT	1
#define ESP_CONF_CHANGED_EVENT 		2

struct BUS_Serial {
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
	uint8_t wifi = 0;
	uint8_t net  = 0;
	uint8_t mqtt = 0;
	uint8_t time = 0;
	uint8_t ap 	 = 0;
	uint8_t web  = 0;
	uint8_t conf = 0;
};

enum SCKmodes {
	MODE_SETUP,
	MODE_NET,
	MODE_SD,
	MODE_FLASH,
	MODE_BRIDGE,
	MODE_OFF,
	MODE_COUNT
};
const uint32_t minimal_publish_interval = 60;
const uint32_t default_publish_interval = 600;
const uint32_t max_publish_interval = 86400;		// One day 

struct SensorConfig {
	bool enabled;
	uint32_t interval;
};

struct Configuration {
	bool valid = false;
	SCKmodes mode;
	SCKmodes persistentMode;						// This mode only changes on user configuration, it can only be MODE_SD or MODE_NET
	uint32_t publishInterval = default_publish_interval;	// in seconds
	SensorConfig sensor[SENSOR_COUNT];
};
