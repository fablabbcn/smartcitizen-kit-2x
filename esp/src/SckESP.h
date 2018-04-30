#pragma once

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "FS.h"
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "RemoteDebug.h"
#include <ArduinoJson.h>
#include <RHReliableDatagram.h>
#include <RH_Serial.h>

#include <Arduino.h>
#include "Shared.h"
#include "Config.h"

class SckESP {
private:

	// Input/Output
	void debugOUT(String strOut);
	bool serialDebug = false;
	bool telnetDebug = true;

	// SAM communication
	uint8_t netPack[NETPACK_TOTAL_SIZE];			// bytes -> 0:PART_NUMBER, 1:TOTAL_PARTS, 2:59 content
	char netBuff[NETPACK_CONTENT_SIZE * 8];
	bool sendMessage(SAMMessage wichMessage, const char *content);
	void receiveMessage(ESPMessage wichMessage);

	// Led control
	const uint8_t pinLED = 4; 	// GPIO5
	uint8_t ledValue = 0;
	Ticker blink;
	void ledSet(uint8_t value);
	void ledBlink(float rate);
	void ledOff();

	// Wifi related
	char hostname[20];
	int currentWIFIStatus;

	// Config
	Configuration config;

	const char *credentialsFileName = "/nets.txt";
	// const static char *credentialsFile = F("/nets.txt");
	bool saveCredentials();
	bool readCredentials();
	bool clearCredentials();
	const char *tokenFileName = "/token.txt";
	bool saveToken();
	bool loadToken();
	bool clearToken();
	bool sendToken();
	
	// AP mode
	void startAP();
	void stopAP();
	// void startWebServer();
	bool flashReadFile(String path);
	const byte DNS_PORT = 53;

public:
	void setup();
	void update();

	void espOut(String strOut);
	void inputUpdate();	

	void _ledToggle();
	void WifiConnected();
	
};

// Static led control
void ledToggle();

// Static webserver handlers
void extSet();

// Wifi event handlers
void onStationConnected(const WiFiEventStationModeConnected& evt);
