#pragma once

#include <SPI.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TimeLib.h>
#include "FS.h"
#include "SPIFFS.h" 	// TODO replace this 
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <Arduino.h>
#include "version.h"
#include "SckSerial.h"

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
	bool debug_serial = true;
};
struct VersionInt { 
	uint8_t mayor; 
	uint8_t minor; 
	uint8_t build; 
};

class SckESP
{
	private:
		// Input/Output
		bool serialDebug = false;		// Interfere with ESP <-> SAM comunnication (use with caution)
		void SAMbusUpdate();
		void debugOUT(String strOut);

		// SAM communication
		VersionInt parseVersionStr(String versionStr);
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
		bool mqttPublishRaw();
		bool mqttInfo();
		bool mqttInventory();
		bool mqttCustom();

		// Led control
		const uint8_t pinLED = 2; 	// TODO return this to the Data board led pin (4)
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
		// void startWebServer();
		bool captivePortal();
		bool isIp(String str);
		const byte DNS_PORT = 53;
		char last_modified[50];
		bool shouldReboot = false; 	// After OTA update
		String OTAstatus = "";

		// Time
		void setNTPprovider();
		void sendNTPpacket(IPAddress &address);
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
		// void webSet(AsyncWebServerRequest *request);
		// void webStatus(AsyncWebServerRequest *request);
		// void webRoot(AsyncWebServerRequest *request);

		// External calls
		void _ledToggle();
		time_t getNtpTime();
};

// Static led control
void ledToggle();

// Static webserver handlers
// void extSet(AsyncWebServerRequest *request);
// void extStatus(AsyncWebServerRequest *request);
// void extRoot(AsyncWebServerRequest *request);

// Time
time_t ntpProvider();

// TODO ESP32
// [ ] Setup Wifi with ESP32 (check https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
// [ ] Setup OTA
// 		https://github.com/chrisjoyce911/esp32FOTA
// 		https://github.com/Fishwaldo/esp_ghota
// [ ] Setup deep sleep
// [ ] Replace AsyncWebserver (research options)
// 		https://github.com/khoih-prog/HTTPS_Server_Generic
// 		https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer
// 		https://github.com/cesanta/mongoose
// 		https://github.com/khoih-prog/WiFiWebServer
//
// [X] Replace Radiohead with SCKSerial
// 	[ ] For now it is not working, some problem is happening and no data is flowing SAM -> ESP
// [X] AP and STa mode at the same time (tested on single ESP32) 
// [ ] Test pubSub and review current  alternatives
// [ ] Support for Eduroam
// [ ] SSL
// [ ] Multiple wifi config
// [ ] Change filesystem to littleFS and remove SPIFFS
// 		https://github.com/littlefs-project/littlefs
// [ ] Clean code to simplify API functions development
// [ ] find the SCK when connected to local WiFi with sck.local or something similar
// * Web simple status screen
// 		* error/warnings
// 		* scard
// 		* flash fill level
// 		* Simple detected sensor list
// * Web config screen
// 		* Configured Wifi
// 		* token
// * Web sensor config
// 		* send to net
// 		* save to sdcard
// 		* reading interval
// 		* per sensor interval
// 		* publish interval
// 		* Estimated battery duration with current config
// * Web system config/info screen
// 		* Firmware versions and builds
// 		* Mac address
// 		* Ip address
// 		* mqtt, ntp servers
// 		* sleep timer
// [ ] Web shell with raw access to SAM shell (tested with webSerial and kind of works)
// [ ] Bluetooth config (NO support for buetotth on C3 arduino core, espidf blufi could be an option)
// * Web sensor dashboard with force update button
// * Web sensor monitor plotter
// * Sdcard file list, data viewer???
// * Flash info and state page
// * Reset via Web interface
// * Enable/disable telnet debug
// * Check if ap and station can work at the same time.
// * Subscribe to a token/config to receive config commands from platform (only after SSL and security is ready)
