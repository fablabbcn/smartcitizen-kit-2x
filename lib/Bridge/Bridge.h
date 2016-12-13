#pragma once

enum EspEvent {
	ESP_NULL,
	ESP_WIFI_CONNECTED_EVENT,
	ESP_WIFI_ERROR_EVENT,
	ESP_WIFI_ERROR_PASS_EVENT,
	ESP_WIFI_ERROR_AP_EVENT,
	ESP_MQTT_HELLO_OK_EVENT,
	ESP_MQTT_PUBLISH_OK_EVENT,
	ESP_MQTT_ERROR_EVENT,
	ESP_TIME_FAIL_EVENT,
	ESP_TIME_NEW_EVENT,
	ESP_MODE_AP_EVENT,
	ESP_MODE_STA_EVENT,
	ESP_MODE_APSTATION_EVENT,
	ESP_FACTORY_RESET_EVENT,
	ESP_CONF_CHANGED_EVENT
};

enum EspCommand {
	// 						------ Configuration
	ESP_SET_WIFI_COM,		// @params String ssid, String password
							// @return bool: true if settings saved, false otherwise
	ESP_SET_TOKEN_COM,		// @params String Token
							// @return bool: true if token saved, false otherwise
	ESP_SET_CONF_COM,		// @params struct Configuration
							// @return bool: true if configuration saved, false otherwise
	ESP_GET_CONF_COM,		// @return struct Configuration
	ESP_FACTORY_RESET_COM,
	//						------ Control
	ESP_START_AP_COM,
	ESP_STOP_AP_COM,
	//						------ Get data
	ESP_GET_APLIST_COM,		// @return String apList (json formatted)
	ESP_GET_TIME_COM,		// @return String: epoch time
	ESP_MQTT_CONNECT_COM,	// @params int persistent flag
	ESP_MQTT_HELLOW_COM,
	ESP_MQTT_PUBLISH_COM,	// @params String payload, int QoS
	ESP_MQTT_SYNC_COM,
	//						------ State
	ESP_GET_STATUS_COM,		// @return struct espStatus:
							//		-- wifi: ESP_WIFI_CONNECTED, ESP_WIFI_ERROR, ESP_WIFI_ERROR_PASS, ESP_WIFI_ERROR_AP
							//		-- mqtt: ESP_MQTT_HELLO_OK, ESP_MQTT_PUBLISH_OK, ESP_MQTT_ERROR
							//		-- time: ESP_TIME_FAIL, ESP_TIME_NEW
							//		-- mode: ESP_MODE_AP, ESP_MODE_STA, ESP_MODE_APSTATION
							//		-- conf: ESP_CONF_CHANGED
	ESP_GET_LAST_EVENT_COM	// @return espEvents
};

struct EspStatus {
	EspEvent wifi = ESP_NULL;
	EspEvent mqtt = ESP_NULL;
	EspEvent time = ESP_NULL;
	EspEvent mode = ESP_NULL;
	EspEvent conf = ESP_NULL;
};

struct Credentials {
	String ssid;
	String password;
};