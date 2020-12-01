#pragma once

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <TimeLib.h>
#include "FS.h"
#include <DNSServer.h>
#include "RemoteDebug.h"
#include <ArduinoJson.h>
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>

#include <Arduino.h>
#include "Shared.h"
#include "version.h"

#define MQTT_QOS 1
#define MQTT_BUFF_SIZE 4096
#define MQTT_KEEP_ALIVE 120

#include "index.html.gz.h"

struct Credentials { bool set=false; char ssid[64]="null"; char pass[64]="null"; };
struct Token { bool set=false; char token[7]="null"; };
struct Mqtt { char server[64]; uint16_t port; };
struct Ntp { char server[64]; uint16_t port; };
struct ESP_Configuration {
	Credentials credentials;
	Token token;
	Mqtt mqtt;
	Ntp ntp;
	bool debug_telnet = false;
};

class SckESP
{
	private:
		// Input/Output
		bool serialDebug = false;		// Interfere with ESP <-> SAM comunnication (use with caution)
		void SAMbusUpdate();
		void debugOUT(String strOut);

		// SAM communication
		uint8_t netPack[NETPACK_TOTAL_SIZE];
		char netBuff[NETBUFF_SIZE];
		bool sendMessage(SAMMessage wichMessage);
		bool sendMessage(SAMMessage wichMessage, const char *content);
		bool sendMessage();
		void receiveMessage(ESPMessage wichMessage);
		bool bootedPending = false;

		// Notifications
		bool sendToken();
		bool sendCredentials();
		bool sendNetinfo();
		bool sendTime();
		bool sendStartInfo();

		// **** MQTT
		bool mqttConnect();
		bool mqttHellow();
		bool mqttPublish();
		bool mqttPublishRaw();
		bool mqttInfo();
		bool mqttInventory();
		bool mqttCustom();

		// Led control
		const uint8_t pinLED = 4; 	// GPIO5
		uint8_t ledValue = 0;
		Ticker blink;
		uint16_t LED_SLOW = 350;
		uint16_t LED_FAST = 100;
		void ledSet(uint8_t value);
		void ledBlink(float rate);
		void ledOff();

		// Wifi related
		char hostname[20];
		String macAddr;
		String ipAddr;
		int currentWIFIStatus;
		void tryConnection();
		void wifiOFF();

		// Config
		ESP_Configuration config;
		const char *configFileName = "/config.txt";
		bool saveConfig(ESP_Configuration newConfig);
		bool saveConfig();
		bool loadConfig();
		bool sendConfig();
		enum SamMode { SAM_MODE_SD, SAM_MODE_NET, SAM_MODE_COUNT };
		SamMode sendMode = SAM_MODE_COUNT;
		uint32_t sendPubInt = 0;
		bool sendConfigPending = false;

		// AP mode
		void startAP();
		void stopAP();
		bool apStarted = false;
		void scanAP();
		int netNumber;
		void startWebServer();
		bool captivePortal();
		bool isIp(String str);
		String toStringIp(IPAddress ip);
		const byte DNS_PORT = 53;
		char last_modified[50];
		bool shouldReboot = false; 	// After OTA update
		String OTAstatus = "";

		// Time
		void setNTPprovider();
		void sendNTPpacket(IPAddress &address);
		String ISOtime();
		String epoch2iso(uint32_t toConvert);
		String leadingZeros(String original, int decimalNumber);
		WiFiUDP Udp;
		byte packetBuffer[48];


	public:
		const String ESPversion = ESPverNum + "-" + String(__GIT_HASH__); 	// mayor.minor.build-gitcommit
		const String ESPbuildDate = String(__ISO_DATE__);
		String SAMversion = "not synced";
		String SAMbuildDate = "not synced";
		// If mayor or minor version of ESP is different than SAM's we need to call a ESP update
		bool updateNeeded = false;

		void setup();
		void update();
		void webSet(AsyncWebServerRequest *request);
		void webStatus(AsyncWebServerRequest *request);
		void webRoot(AsyncWebServerRequest *request);

		// External calls
		void _ledToggle();
		time_t getNtpTime();
};

// Static led control
void ledToggle();

// Static webserver handlers
void extSet(AsyncWebServerRequest *request);
void extStatus(AsyncWebServerRequest *request);
void extRoot(AsyncWebServerRequest *request);

// Time
time_t ntpProvider();
