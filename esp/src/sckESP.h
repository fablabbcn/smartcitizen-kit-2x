#pragma once

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "serialcomm.h"
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
	Configuration configuration;

	// WIFI
	void tryConnection();
	void wifiDisconnect();
	bool addNetwork();
	void clearNetworks();
	bool readNetwork(int index=-1);
	void removeNetwork(int index);
	int countSavedNetworks();
	void sendNetwork(uint8_t index, EspCommand comm=ESP_GET_WIFI_COM);
	void clearDuplicated();
	int8_t selectBestNetwork();
	void setGoodNet();
	int actualWIFIStatus;
	int prevWIFIStatus;
	Credentials credentials;
	int scanAP();

	// AP mode
	void startAP();
	void stopAP();
	void startWebServer();
	void stopWebserver();
	void webRoot();
	void webSet();
	void webShow();
	// for captive portal
	const byte DNS_PORT = 53;
	
	// Token
	char token[8];
	void saveToken();
	void readToken();

	// MQTT
	bool mqttStart();
	bool mqttHellow();
	bool mqttPublish();
	bool mqttSend(String payload);
	bool mqttSync();

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
void extRoot();
void extSet();
void extShow();


// https://kangax.github.io/html-minifier/
static const String htmlPage1 = "<!DOCTYPE html><meta name=viewport content='width=device-width,initial-scale=1'><style>body{color:#434343;font-family:'Helvetica Neue',Helvetica,Arial,sans-serif;font-size:22px;line-height:1.1;padding:20px}.container{margin:0 auto;max-width:400px}img{display:block;margin-left:auto;margin-right:auto}form .field-group{box-sizing:border-box;clear:both;padding:10px 0;position:relative;margin:1px 0;width:100%}form .field-group>label{color:#a5acb2;display:block;margin:0 0 5px 0;position:relative;word-wrap:break-word}input[type=text],select[type=text]{background:#fff;border:1px solid #d0d0d0;border-radius:2px;box-sizing:border-box;color:#434343;font-family:inherit;font-size:1.1em;height:2.2em;padding:4px 5px;margin:0;width:100%}input[type=text]:focus{border-color:#4c669f;outline:0}.button-container{text-align:center;box-sizing:border-box;clear:both;margin:1px 0 0;padding:4px 0;position:relative;width:100%}button[type=submit]{box-sizing:border-box;background:#f5f5f5;border:1px solid #bdbdbd;border-radius:2px;color:#434343;cursor:pointer;display:inline-block;font-family:inherit;font-size:22px;font-variant:normal;font-weight:400;height:2.5em;margin:0;padding:4px 10px;text-decoration:none;vertical-align:baseline;white-space:nowrap}</style><div class=container><img src='data:image/data:image/gif;base64,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'><h2 style=text-align:center>Configuraci&oacute WiFi</h2><form method=post action=/ ><div class=field-group><input name=epoch id=epoch type=hidden value=\"\" /><label>Nom de la xarxa</label><select name=ssid type=text length=64>";
static const String htmlPage2 = "</select></div><div class=field-group><label>Contrasenya de la xarxa</label><input name=password type=text length=64></div><div class=field-group><label>Codi del Kit</label><input name=token type=text maxlength=6 minlength=6></div><div class=button-container><button type=submit>Connectar</button></div></form></div><script type=text/javascript>document.getElementById(\"epoch\").value = Math.floor((new Date).getTime()/1000);</script></body></html>";

