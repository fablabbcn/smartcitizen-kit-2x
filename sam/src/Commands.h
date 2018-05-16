#pragma once

#include <Arduino.h>
#include <Wire.h>

extern TwoWire auxWire;

class SckBase;

// Define command type
enum CommandType {

	COM_RESET,
	COM_GET_VERSION,
	COM_RESET_CAUSE,
	COM_OUTLEVEL,
	COM_HELP,
	COM_PINMUX,
	COM_LIST_SENSOR,
	COM_READ_SENSOR,
	COM_GET_FREERAM,
	COM_BATT_REPORT,
	COM_I2C_DETECT,
	COM_GET_CHARGER_CONF,
	COM_CONFIG,
	COM_ESP_CONTROL,
	COM_NETINFO,
	COM_TIME,
	COM_STATE,
	COM_HELLO,
	COM_DEBUG,

	COM_COUNT
};

// Declare command function
void reset_com(SckBase* base, String parameters);
void getVersion_com(SckBase* base, String parameters);
void resetCause_com(SckBase* base, String parameters);
void outlevel_com(SckBase* base, String parameters);
void help_com(SckBase* base, String parameters);
void pinmux_com(SckBase* base, String parameters);
void listSensor_com(SckBase* base, String parameters);
void readSensor_com(SckBase* base, String parameters);
void freeRAM_com(SckBase* base, String parameters);
void battReport_com(SckBase* base, String parameters);
void i2cDetect_com(SckBase* base, String parameters);
void getCharger_com(SckBase* base, String parameters);
void config_com(SckBase* base, String parameters);
void esp_com(SckBase* base, String parameters);
void netInfo_com(SckBase* base, String parameters);
void time_com(SckBase* base, String parameters);
void state_com(SckBase* base, String parameters);
void hello_com(SckBase* base, String parameters);
void debug_com(SckBase* base, String parameters);

typedef void (*com_function)(SckBase* , String);

class OneCom {
public:	
	uint16_t place;
	CommandType type;
	const char *title;
	const char *help;
	com_function function;

	OneCom(uint16_t nplace, CommandType ntype, const char *nTitle, const char *nhelp, com_function nfunction) {
		place = nplace;
		type = ntype;
		title = nTitle;
		help = nhelp;
		function = nfunction;
	}
};

class AllCommands {
public:

	OneCom com_list[COM_COUNT] {

		//			place	type 			title 				help 																	function
		OneCom { 	10,	COM_RESET, 			"reset", 			"Resets the SCK", 															reset_com},
		OneCom { 	20,	COM_GET_VERSION, 		"version",	 		"Shows versions and Hardware ID",				 									getVersion_com},
		OneCom {	30,	COM_RESET_CAUSE,		"rcause",			"Show last reset cause (debug)",													resetCause_com},
		OneCom {	40,	COM_OUTLEVEL,			"outlevel",			"Shows/sets outlevel [0:silent, 1:normal, 2:verbose]",											outlevel_com},
		OneCom {	50,	COM_HELP,			"help",				"Duhhhh!!",																help_com},
		OneCom {	60,	COM_PINMUX,			"pinmux",			"Shows SAMD pin mapping status",													pinmux_com},
		OneCom {	80,	COM_LIST_SENSOR,		"sensors",			"Shows a list of enabled/disabled sensors",												listSensor_com},
		OneCom {	90,	COM_READ_SENSOR,		"read",				"Reads sensor [sensorName]",														readSensor_com},
		OneCom {	90,	COM_GET_FREERAM,		"free",				"Shows the amount of free RAM memory",													freeRAM_com},
		OneCom {	90,	COM_BATT_REPORT,		"batt",				"Shows the battery state",														battReport_com},
		OneCom {	90,	COM_I2C_DETECT,			"i2c",				"Search the I2C bus for devices",													i2cDetect_com},
		OneCom {	90,	COM_GET_CHARGER_CONF,		"charger",			"Shows charger configuration",														getCharger_com},
		OneCom {	90,	COM_CONFIG,			"config",			"Shows/sets configuration [-defaults] [-mode sdcard/network] [-pubint seconds] [-wifi \"ssid\" [\"pass\"]] [-token token]",		config_com},
		OneCom {	100,	COM_ESP_CONTROL,		"esp",				"Controls ESP [on, off, reboot]", 													esp_com},
		OneCom {	100,	COM_NETINFO,			"netinfo",			"Shows network information",														netInfo_com},
		OneCom {	100,	COM_TIME,			"time",				"Shows/sets or syncs (if needed) time",													time_com},
		OneCom {	100,	COM_STATE,			"state",			"Shows state flags",															state_com},
		OneCom {	100,	COM_HELLO,			"hello",			"Sends MQTT hello to platform",														hello_com},
		OneCom {	100,	COM_DEBUG, 			"debug", 			"Toggle debug messages [-light]", 													debug_com},

	};

	OneCom & operator[](CommandType type) {
    	return com_list[type];
	};

	void in(SckBase* base, String strIn);

private:
	
};

// // External Commands
// enum ExternalCommand {
// 	// Esp commands
// 	EXTCOM_ESP_REBOOT,
// 	EXTCOM_ESP_OFF,
// 	EXTCOM_ESP_ON,
// 	EXTCOM_ESP_START_AP,
// 	EXTCOM_ESP_STOP_AP,
// 	EXTCOM_ESP_START_WEB,
// 	EXTCOM_ESP_STOP_WEB,
// 	EXTCOM_ESP_SLEEP,
// 	EXTCOM_ESP_WAKEUP,
// 	EXTCOM_GET_APLIST,
// 	EXTCOM_ESP_SERIAL_DEBUG_TOGGLE,
// 	EXTCOM_ESP_LED_ON,
// 	EXTCOM_ESP_LED_OFF,
// 	EXTCOM_ESP_MQTT_HELLO,
// 	EXTCOM_ESP_GET_FREEHEAP,
// 	EXTCOM_ESP_SHOW_STATUS,

// 	// Configuration commands
// 	EXTCOM_SET_WIFI,
// 	EXTCOM_GET_WIFI,
// 	EXTCOM_CLEAR_WIFI,
// 	EXTCOM_GET_NET_INFO,
// 	EXTCOM_SET_TOKEN,
// 	EXTCOM_CLEAR_TOKEN,
// 	EXTCOM_GET_VERSION,
// 	EXTCOM_SYNC_CONFIG,
// 	EXTCOM_DOWNLOAD_CONFIG,
// 	EXTCOM_SET_CONFIG,
// 	EXTCOM_GET_CONFIG,

// 	// Mode commands
// 	EXTCOM_RESET,
			
// 	// Other configuration
// 	EXTCOM_SET_OUTLEVEL,
// 	EXTCOM_GET_OUTLEVEL,
// 	EXTCOM_SET_LED,
// 	EXTCOM_GET_URBAN_PRESENT,
// 	EXTCOM_READLIGHT_ON,
// 	EXTCOM_READLIGHT_OFF,
// 	EXTCOM_READLIGHT_RESET,
// 	EXTCOM_READLIGHT_TOGGLE_DEBUG,
// 	EXTCOM_MQTT_CONFIG,			// @params: on/off

// 	// Time configuration
// 	EXTCOM_GET_TIME,			// @params: iso (default), epoch
// 	EXTCOM_SET_TIME,			// @params: epoch time
// 	EXTCOM_SYNC_HTTP_TIME,

// 	// SD card
// 	EXTCOM_SD_PRESENT,

// 	// Sensors
// 	EXTCOM_GET_SENSOR,
// 	EXTCOM_RAM_COUNT,
// 	EXTCOM_RAM_READ,
// 	EXTCOM_PUBLISH,
// 	EXTCOM_LIST_SENSORS,
// 	EXTCOM_ENABLE_SENSOR,
// 	EXTCOM_DISABLE_SENSOR,
// 	EXTCOM_SET_INTERVAL_SENSOR,
// 	EXTCOM_CONTROL_SENSOR,

// 	// Print String to u8g2_oled screen
// 	EXTCOM_U8G_PRINT,			// @params: String to be printed
// 	EXTCOM_U8G_PRINT_SENSOR,	// @params: Sensor to be printed

// 	// Power Management
// 	EXTCOM_SLEEP,
// 	EXTCOM_GET_POWER_STATE,
// 	EXTCOM_SET_CHARGER_CURRENT,
// 	EXTCOM_RESET_CAUSE,

// 	// Other
// 	EXTCOM_GET_FREE_RAM,
// 	EXTCOM_LIST_TIMERS,

// 	EXTCOM_HELP,

// 	// Count
// 	EXTCOM_COUNT

// };

// const char *comTitles[EXTCOM_COUNT] PROGMEM = {

// // EXTERNAL COMMAND TITLES
// // Esp Command
// "esp reboot",			// 	EXTCOM_ESP_REBOOT,
// "esp off",				// 	EXTCOM_ESP_OFF,
// "esp on",				// 	EXTCOM_ESP_ON,
// "esp start ap",			// 	EXTCOM_ESP_START_AP,
// "esp stop ap",			// 	EXTCOM_ESP_STOP_AP,
// "esp start web",		// 	EXTCOM_ESP_START_WEB,
// "esp stop web",			// 	EXTCOM_ESP_STOP_WEB,
// "esp sleep",			// 	EXTCOM_ESP_SLEEP,
// "esp wakeup",			// 	EXTCOM_ESP_WAKEUP,
// "get aplist",			// 	EXTCOM_GET_APLIST,
// "esp debug",			// 	EXTCOM_ESP_SERIAL_DEBUG_TOGGLE,
// "esp led on",			// 	EXTCOM_ESP_LED_ON,
// "esp led off",			// 	EXTCOM_ESP_LED_OFF,
// "mqtt hello",			// 	EXTCOM_ESP_MQTT_HELLO,
// "esp heap",				// 	EXTCOM_ESP_GET_FREEHEAP,
// "esp status",			// 	EXTCOM_ESP_SHOW_STATUS,

// // Configuration commands
// "set wifi",				// EXTCOM_SET_WIFI,
// "get wifi",				// EXTCOM_GET_WIFI,
// // "get best wifi",		// EXTCOM_GET_BEST_WIFI,
// "clear wifi",			// EXTCOM_CLEAR_WIFI,
// "get netinfo",			// EXTCOM_GET_NET_INFO,
// "set token",			// EXTCOM_SET_TOKEN,
// "clear token",			// EXTCOM_CLEAR_TOKEN,
// "get version",			// EXTCOM_GET_VERSION,
// "sync config",			// EXTCOM_SYNC_CONFIG,
// "download config",		// EXTCOM_DOWNLOAD_CONFIG,
// "set config",			// EXTCOM_SET_CONFIG,		@params: "factory", "publish interval" [seconds], "mode" [sdcard or network]
// "get config",			// EXTCOM_GET_CONFIG,

// // Mode commands
// "reset",				// EXTCOM_RESET,

// // Other configuration
// "set outlevel",			// EXTCOM_SET_OUTLEVEL,		
// "get outlevel",			// EXTCOM_GET_OUTLEVEL,
// "set led",				// EXTCOM_SET_LED,			@params: off, (to implement: red, blue, green, etc)
// "urban present",		// EXTCOM_GET_URBAN_PRESENT,
// "readlight on",			// EXTCOM_READLIGHT_ON,
// "readlight off",		// EXTCOM_READLIGHT_OFF,
// "readlight reset",		// EXTCOM_READLIGHT_RESET,
// "readlight debug",		// EXTCOM_READLIGHT_TOGGLE_DEBUG,
// "mqttconfig",			// EXTCOM_MQTT_CONFIG,

// // Time configuration
// "get time",				// EXTCOM_GET_TIME,			@params: iso (default), epoch
// "set time",				// EXTCOM_SET_TIME,			@params: epoch time
// "sync time",			// EXTCOM_SYNC_HTTP_TIME,

// // SD card
// "sd present",			// EXTCOM_SD_PRESENT,

// // Sensors
// "read",					// EXTCOM_GET_SENSOR, 		@params sensor Title
// "ram count",			// EXTCOM_RAM_COUNT,
// "ram read",				// EXTCOM_RAM_READ,
// "publish",				// EXTCOM_PUBLISH,
// "list sensors",			// EXTCOM_LIST_SENSORS,
// "enable",				// EXTCOM_ENABLE_SENSOR, 	@params wichSensor
// "disable",				// EXTCOM_DISABLE_SENSOR, 	@params wichSensor
// "interval",				// EXTCOM_SET_INTERVAL_SENSOR,	@params [seconds]
// "control",				// EXTCOM_CONTROL_SENSOR, 	@params wichSensor wichCommand

// // U8g_OLED commands
// "u8g print",			// EXTCOM_U8G_PRINT,		@params: String to be printed
// "u8g sensor",			// EXTCOM_U8G_PRINT_SENSOR,	@params: Sensor to be printed

// // Power Management
// "sleep",				// EXTCOM_SLEEP,
// "get power",			// EXTCOM_GET_POWER_STATE,
// "set charger",			// EXTCOM_SET_CHARGER_CURRENT,
// "rcause",				// EXTCOM_RESET_CAUSE,

// // Other
// "get freeram",			// EXTCOM_GET_FREE_RAM,
// "list timers",			// EXTCOM_LIST_TIMERS,

// "help",					// EXTCOM_HELP,

// };
