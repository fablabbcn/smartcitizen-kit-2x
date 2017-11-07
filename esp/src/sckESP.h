#pragma once

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <PubSubClient.h>

#include <Bridge.h>
#include <Sensors.h>

// Led pins
#define LED_RIGHT	5		// GPIO5
#define LED_LEFT 	4		// GPIO4

// Reset pin
#define SAM_RESET	14		// GPIO14

// File names
#define CREDENTIALS_FILE "/nets.txt"
#define MAX_SAVED_CREDENTIALS 8
#define TOKEN_FILE "/token.txt"

// Time server
#define NTP_SERVER_NAME "pool.ntp.org"
#define HTTP_TIME_SERVER_NAME "data.smartcitizen.me"
#define MQTT_SERVER_NAME "mqtt.smartcitizen.me"
#define MQTT_SERVER_PORT 1883
#define MQTT_QOS 1


class SckESP {
public:
	void setup();
	void update();

	enum Modes { MODE_OTA_UPDATE, MODE_SLEEP, MODE_WIFI, MODE_NO_WIFI };

	// Communication with SAM
	BUS_Serial msgIn;
	BUS_Serial msgOut;
	bool processMsg();
	void SAMsendMsg();
	ESPstatus espStatus;
	void sendStatus();
	void clearParam();
	void SAMlistSavedNetworks();

	// Configuration Management
	bool saveConf();
	bool loadConf();
	Configuration config;
	char hostname[20];

	// WIFI
	void tryConnection();
	bool addNetwork();
	void clearNetworks();
	bool readNetwork();
	int countSavedNetworks();
	void sendNetwork(EspCommand comm=ESP_GET_WIFI_COM);
	int actualWIFIStatus;
	int prevWIFIStatus;
	void scanAP();
	int netNumber;
	uint8_t connectionRetrys = 0;

	// AP mode
	void startAP();
	void stopAP();
	void startWebServer();
	void stopWebserver();
	void webSet();
	void webStatus();
	bool flashReadFile(String path);
	// for captive portal
	bool captivePortal();
	bool isIp(String str);
	String toStringIp(IPAddress ip);
	const byte DNS_PORT = 53;
	String consoleBuffer;
	
	// Token
	bool saveToken();
	bool loadToken();
	void clearToken();
	void sendToken();
	
	// MQTT
	bool mqttConnect();
	bool mqttHellow();
	bool mqttPublish();
	bool mqttSync();
	bool mqttConfigSub(bool activate);
	uint32_t mqttSubTimer = 0;
	bool configSubscribed = false;

	// FileSystem
	FSInfo fs_info;
	uint16_t countLines(File myFile);

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

	// Time
	time_t getNtpTime();
	void sendNTPpacket(IPAddress &address);
	String ISOtime();
	String epoch2iso(uint32_t toConvert);
	bool getHttpTime();
	WiFiUDP Udp;
	byte packetBuffer[48];
	
	// Debug
	void debugOUT(String strOut);
	bool serialDebug = false;

	// Sensors
	AllSensors sensors;

private:
};

// Static functions for global calls.
void LedToggleLeft();
void LedToggleRight();

// Utility functions
String leadingZeros(String original, int decimalNumber);

// Time
time_t ntpProvider();

// Static webserver handlers
void extSet();
void extStatus();
void extAplist();
