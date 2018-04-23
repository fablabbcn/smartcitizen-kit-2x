#pragma once

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "FS.h"
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "RemoteDebug.h"

#include <Arduino.h>
#include "Shared.h"

class SckESP {
private:

	// Input/Output
	void debugOUT(String strOut);
	bool serialDebug = false;
	bool telnetDebug = true;

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
	

	void _ledToggle();
	
};

// Static led control
void ledToggle();

// Static webserver handlers
void extSet();