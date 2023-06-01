#pragma once

#include <HardwareSerial.h>
#include <Arduino.h>

#define NETBUFF_SIZE 512

enum SCKMessage {

	SCKMES_PLACEHOLDER,

	ESPMES_SET_CONFIG,		// 1 SAM->ESP, Sends new config
	ESPMES_GET_NETINFO,		// 2 SAM->ESP, ESP return network info
	ESPMES_GET_TIME,		// 3 SAM->ESP, ESP returns epoch time
	ESPMES_MQTT_HELLO,		// 4 SAM->ESP, ESP publish Hello and returns result
	ESPMES_MQTT_PUBLISH,	// 5 SAM->ESP, ESP publish readings and returns results
	ESPMES_CONNECT, 		// 6 SAM->ESP, ESP trys wifi conection
	ESPMES_START_AP, 		// 7 SAM->ESP, ESP starts AP
	ESPMES_STOP_AP, 		// 8 SAM->ESP, ESP stops AP
	ESPMES_LED_OFF, 		// 9 SAM->ESP, ESP turns off led (esud before sleep)
	ESPMES_MQTT_INVENTORY, 	// 10 SAM->ESP, ESP publish inventory and returns result
	ESPMES_MQTT_INFO, 		// 11 SAM->ESP, ESP publish info and return result
	ESPMES_MQTT_CUSTOM, 	// 12 SAM->ESP, ESP publish custom message on custom topic
	ESPMES_WEBSERIAL, 		// 13 SAM->ESP, Send output of webserial command to ESP

	SAMMES_BOOTED, 				// 14 ESP->SAM, On finished booting
	SAMMES_NETINFO,				// 15 ESP->SAM, Send network info
	SAMMES_WIFI_CONNECTED,		// 16 ESP->SAM, On wifi succesfull conection
	SAMMES_SSID_ERROR,			// 17 ESP->SAM, On ssid not found
	SAMMES_PASS_ERROR,			// 18 ESP->SAM, On wrong password
	SAMMES_WIFI_UNKNOWN_ERROR,	// 19 ESP->SAM, On wifi unknown error
	SAMMES_TIME,				// 20 ESP->SAM, Epoch time
	SAMMES_MQTT_HELLO_OK,		// 21 ESP->SAM, On MQTT hello OK
	SAMMES_MQTT_PUBLISH_OK,		// 22 ESP->SAM, On MQTT publish ok
	SAMMES_MQTT_PUBLISH_ERROR, 	// 23 ESP->SAM, On MQTT publish error
	SAMMES_MQTT_INFO_OK, 		// 24 ESP->SAM, On MQTT info publish ok
	SAMMES_MQTT_INFO_ERROR, 	// 25 ESP->SAM, On MQTT info publish error
	SAMMES_MQTT_CUSTOM_OK, 		// 26 ESP->SAM, On MQQT custom publish OK
	SAMMES_MQTT_CUSTOM_ERROR,	// 27 ESP->SAM, On MQQT custom publish error
	SAMMES_SET_CONFIG,			// 28 ESP->SAM, Sends new config

	SCKMES_COUNT
};

class SckSerial {

private:
	HardwareSerial& _serial;
	const uint8_t maxTry = 3;
	bool _send();
	void debugP(char *msg, bool force=false);
	void debugPln(char *msg, bool force= false);

public:
	SckSerial(HardwareSerial& _s): _serial(_s) { 
		_serial = _s; 
	}
	void begin(uint32_t bauds=115200);
	bool send(SCKMessage wichMessage); 
	bool send(SCKMessage wichMessage, const char *content);
	void sendACK();
	bool receive();

	bool debug = false;
	byte msg;
	char buff[NETBUFF_SIZE];
	bool error = false;
};
