#pragma once

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "../lib/bridge.h"

// Led pins
#define LED_RIGHT	5		// GPIO5
#define LED_LEFT 	4		// GPIO4

// Reset pin
#define SAM_RESET	14		// GPIO14



class SckESP {
public:
	void setup();
	void start();

	bool processCommand(EspCommand command);

	// Configuration Management
	bool saveConf();
	bool loadConf();

	// WIFI
	bool setWifi(Credentials credentials);
	bool setToken(String token);

	// MQTT
	bool mqttStart(String server);
	bool mqttHellow();
	bool mqttSend(String payload);
	bool mqttSync();

	// Led control
	void ledSet(uint8_t wichLed, uint8_t value);
	void ledToggle(uint8_t wichLed);
	void ledBlink(uint8_t wichLed, float rate);
	uint8_t ledLeft = 0;
	uint8_t ledRight = 1;
	uint8_t ledPin[2] = {LED_LEFT, LED_RIGHT};
	uint8_t ledValue[2] = {0, 0};
	Ticker Lblink;
	Ticker Rblink;

private:
};

// Static functions for global calls.
void ledToggleLeft();
void ledToggleRight();
