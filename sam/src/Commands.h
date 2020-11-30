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
	COM_CONTROL_SENSOR,
	COM_MONITOR_SENSOR,
	COM_FLASH,
	COM_GET_FREERAM,
	COM_I2C_DETECT,
	COM_CHARGER,
	COM_CONFIG,
	COM_ESP_CONTROL,
	COM_NETINFO,
	COM_TIME,
	COM_HELLO,
	COM_DEBUG,
	COM_SHELL,
	COM_CUSTOM_MQTT,
	COM_OFFLINE,
	COM_MQTT_CONFIG,
	COM_NTP_CONFIG,

	COM_COUNT
};

// Declare command function
void reset_com(SckBase* base, String parameters);
void getVersion_com(SckBase* base, String parameters);
void resetCause_com(SckBase* base, String parameters);
void outlevel_com(SckBase* base, String parameters);
void help_com(SckBase* base, String parameters);
void pinmux_com(SckBase* base, String parameters);
void sensorConfig_com(SckBase* base, String parameters);
void readSensor_com(SckBase* base, String parameters);
void controlSensor_com(SckBase* base, String parameters);
void monitorSensor_com(SckBase* base, String parameters);
void flash_com(SckBase* base, String parameters);
void freeRAM_com(SckBase* base, String parameters);
void i2cDetect_com(SckBase* base, String parameters);
void power_com(SckBase* base, String parameters);
void config_com(SckBase* base, String parameters);
void esp_com(SckBase* base, String parameters);
void netInfo_com(SckBase* base, String parameters);
void time_com(SckBase* base, String parameters);
void hello_com(SckBase* base, String parameters);
void debug_com(SckBase* base, String parameters);
void shell_com(SckBase* base, String parameters);
void custom_mqtt_com(SckBase* base, String parameters);
void offline_com(SckBase* base, String parameters);
void mqttConfig_com(SckBase* base, String parameters);
void ntpConfig_com(SckBase* base, String parameters);

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

			//	place	type 			title 		help 																	function
			OneCom {10,	COM_RESET, 		"reset", 	"Resets the SCK", 															reset_com},
			OneCom {20,	COM_GET_VERSION, 	"version",	"Shows versions and Hardware ID",				 									getVersion_com},
			OneCom {30,	COM_RESET_CAUSE,	"rcause",	"Show last reset cause (debug)",													resetCause_com},
			OneCom {40,	COM_OUTLEVEL,		"outlevel",	"Shows/sets output level: outlevel [0:silent, 1:normal, 2:verbose]",									outlevel_com},
			OneCom {50,	COM_HELP,		"help",		"Duhhhh!!",																help_com},
			OneCom {60,	COM_PINMUX,		"pinmux",	"Shows SAMD pin mapping status",													pinmux_com},
			OneCom {90,	COM_FLASH,		"flash",	"Shows and manage flash memory state [no-param -> info] [-format (be carefull)] [-dump sect-num (0-2040)] [-sector sect-num] [-recover sect-num/all net/sd]", flash_com},
			OneCom {80,	COM_LIST_SENSOR,	"sensor",	"Shows/sets sensor state or interval: sensor sensor-name [-enable or -disable] [-interval interval(seconds)] [-oled]",			sensorConfig_com},
			OneCom {90,	COM_READ_SENSOR,	"read",		"Reads sensor: read [sensorName]",													readSensor_com},
			OneCom {90,	COM_CONTROL_SENSOR,	"control",	"Control sensor: control [sensorName] [command]",											controlSensor_com},
			OneCom {90,	COM_MONITOR_SENSOR,	"monitor",	"Continously read sensor: monitor [-sd] [-notime] [-noms] [-oled] [sensorName[,sensorNameN]]",						monitorSensor_com},
			OneCom {90,	COM_GET_FREERAM,	"free",		"Shows the amount of free RAM memory",													freeRAM_com},
			OneCom {90,	COM_I2C_DETECT,		"i2c",		"Search the I2C bus for devices",													i2cDetect_com},
			OneCom {90,	COM_CHARGER,		"power",	"Controls/shows power config: power [-info (extra info)] [-batcap mAh] [-otg on/off] [-charge on/off] [-sleep min (0-disable)]",	power_com},
			OneCom {90,	COM_CONFIG,		"config",	"Shows/sets configuration: config [-defaults] [-mode sdcard/network] [-pubint seconds] [-readint seconds] [-wifi \"ssid\" [\"pass\"]] [-token token]", config_com},
			OneCom {100,	COM_ESP_CONTROL,	"esp",		"Controls or shows info from ESP: esp [-on -off -sleep -wake -reboot -flash]",								esp_com},
			OneCom {100,	COM_NETINFO,		"netinfo",	"Shows network information",														netInfo_com},
			OneCom {100,	COM_TIME,		"time",		"Shows/sets date and time: time [epoch time] [-sync]",											time_com},
			OneCom {100,	COM_HELLO,		"hello",	"Sends MQTT hello to platform",														hello_com},
			OneCom {100,	COM_DEBUG, 		"debug", 	"Toggle debug messages: debug [-sdcard] [-esp] [-oled] [-flash] [-telnet]", 								debug_com},
			OneCom {100,	COM_SHELL, 		"shell", 	"Shows or sets shell mode: shell [-on] [-off]",												shell_com},
			OneCom {100,	COM_CUSTOM_MQTT,	"publish", 	"Publish custom mqtt message: mqtt [\"topic\" \"message\"]",										custom_mqtt_com},
			OneCom {100,	COM_OFFLINE, 		"offline", 	"Configure offline periods and WiFi retry interval: [-retryint seconds] [-period start-hour end-hour (UTC 0-23)]",			offline_com},
			OneCom {100,	COM_MQTT_CONFIG, 	"mqttsrv", 	"Configure mqtt server address and port: [-host serverName] [-port portNum]",								mqttConfig_com},
			OneCom {100,	COM_NTP_CONFIG, 	"ntpsrv", 	"Configure ntp server address and port: [-host serverName] [-port portNum]",								ntpConfig_com}
		};

		OneCom & operator[](CommandType type) {
			return com_list[type];
		};

		void in(SckBase* base, String strIn);
		void wildCard(SckBase* base, String strIn);

	private:

};
