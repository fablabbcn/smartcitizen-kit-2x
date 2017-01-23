#pragma once

#include <ESP8266WiFi.h>
// #include <DNSServer.h>
// #include <ESP8266WebServer.h>
// #include <WiFiManager.h>
#include <Ticker.h>
#include <Bridge.h>
#include <TimeLib.h>
#include "FS.h"
#include "serialcomm.h"

// Led pins
#define LED_RIGHT	5		// GPIO5
#define LED_LEFT 	4		// GPIO4

// Reset pin
#define SAM_RESET	14		// GPIO14

// File names
#define CREDENTIALS_FILE "/cred.txt"


class SckESP {
public:
	void setup();
	void start();
	void update();

	enum Modes { MODE_OTA_UPDATE, MODE_SLEEP, MODE_WIFI, MODE_NO_WIFI };

	// Communication with SAM
	bool processCommand(EspCommand command);
	// SerialComm serialSAM;

	// Configuration Management
	bool saveConf();
	bool loadConf();

	// WIFI
	String scanAP();
	bool setWifi();
	bool setToken(String token);
	void readCredentials();
	void saveCredentials();
	Credentials credentials;
	String token;


	// MQTT
	bool mqttStart(String server);
	bool mqttHellow();
	bool mqttSend(String payload);
	bool mqttSync();

	// FileSystem
	FSInfo fs_info;

	// Led control
	void ledSet(uint8_t wichLed, uint8_t value);
	void ledToggle(uint8_t wichLed);
	void ledBlink(uint8_t wichLed, float rate);
	void ledOff();
	uint8_t ledLeft = 0;
	uint8_t ledRight = 1;
	uint8_t ledPin[2] = {LED_LEFT, LED_RIGHT};
	uint8_t ledValue[2] = {0, 0};
	Ticker Lblink;
	Ticker Rblink;

private:
};

// Static functions for global calls.
void LedToggleLeft();
void LedToggleRight();
