#pragma once

// #define ARDUINO_ARCH_SAMD

#include <Arduino.h>
#include <RTCZero.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "WatchdogSAMD.h"
// #include <Scheduler.h>
#include <FlashStorage.h>

#include "ReadLight.h"
#include "sckAux.h"

/* 	-------------------------------
	|	SCK Baseboard Pinout	  |
 	-------------------------------
*/
// ESP8266 Management
#define CH_PD   		31		// PB23
#define POWER_WIFI  	30		// PB22
#define GPIO0   		11		// PA16
#define CS_ESP  		13		// PA17

// RGB Led 
#define PIN_LED_RED 	6		// PA20
#define PIN_LED_GREEN 	12		// PA19
#define PIN_LED_BLUE 	10		// PA18

// Button 
#define PIN_BUTTON 		7		// PA21

// Sensor Board Conector
#define IO0 9					// PA7 -- CO Sensor Heather
#define IO1 8       			// PA6 -- NO2 Sensor Heater
#define IO2 3					// PA9 -- Unused
#define IO3 4					// PA8 -- Unused
#define S0 A3         			// PA4 -- CO Sensor
#define S1 A4         			// PA5 -- NO2 Sensor
#define S2 A1         			// PB8 -- CO Current Sensor
#define S3 A2         			// PB9 -- NO2 Current Sensor
#define S4 A5         			// PB2 -- Sound Sensor
#define S5 A0         			// PA2 -- Unused

// SPI Configuration
#define MOSI 	21				// PB10
#define SCK 	20				// PB11
#define MISO	18				// PA12
#define CS_SDCARD	2			// PA14 -- SPI Select SDcard

// Power Management
#define PS 	38					// PA13 -- TPS63001 PS/SYNC

// ACOMODAR Y CAMBIAR NOMBRES ESTO ES TEMP
#define POT1                 0x50    
#define POT2                 0x51    // Direction of the Potenciometer 2 for MICS heather voltage
#define POT3                 0x52    // Direction of the Potenciometer 3 for MICS measure
#define POT4                 0x53  		
#define  kr  392.1568     //Constante de conversion a resistencia de potenciometrosen ohmios


/* 	----------------------------
 	|	Settings Constants	   |
 	----------------------------
*/
#define LONG_PRESS_DURATION 3000 		// Button long press duration (ms)
#define VERY_LONG_PRESS_DURATION 15000	// Button very long press duration (ms)


enum SCKmodes {	MODE_AP, MODE_NET, MODE_SD, MODE_SHELL, MODE_FLASH, MODE_BRIDGE, MODE_ERROR, MODE_FIRST_BOOT, MODE_OFF, MODE_count };

/* 	-----------------
 	|	 Button 	|
 	-----------------
*/
class Button {
public:
	void setup();

	bool isDown;
	float lastPress;
	float lastRelease;

	float longPressDuration;
	float veryLongPressDuration;
};
void ISR_button();


/* Color definition
 *
 */
struct oneColor {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/* 	-------------
 	|	 Led	|
 	-------------
*/
class Led {
public:
	void setup();
	void update(SCKmodes newMode);
	void reading();
	void off();
	void bridge();
	void crcOK();
	void wifiOK();
	enum pulseModes {PULSE_SOFT, PULSE_HARD, PULSE_STATIC};
	void tick();

	oneColor white = {255,165,255};    	//hue 180, sat 0.50
	oneColor red = {255,20,8};			//hue 4, sat .92
	oneColor green = {0,255,0};			//hue 120, sat 1.0
	oneColor blue = {0,50,255};			//hue 233, sat 1.0
	oneColor pink = {100,15,35};		//hue 308, sat 0.85
	oneColor orange = {255,70,0};		//hue 12, sat 1.0
	//oneColor lightBLue = 				//hue 170, sat 1.0
	//oneColor yellow = 				//hue 26, sat 0.87

	uint8_t HARD_PULSE = 0;
	uint8_t SOFT_PULSE = 1;

	// Hardware timer
	uint8_t refreshPeriod = 50;

	pulseModes pulseMode = PULSE_SOFT;
	pulseModes newPulseMode = PULSE_SOFT;
	float timerReading;   //substituir esto por una libreria de timers
	float pulseduration = 500;
	uint16_t readingDuration = 1000;

private:
	float hue;
	float sat;
	float inten;
	float newHue;
	float newSat;
	bool dir;
	void setRGBColor(oneColor myColor);
	void setHSIColor(float h, float s, float i);
	void setRGB(uint8_t r, uint8_t g, uint8_t b);
};


struct sensorData {
		String time;
		float noise;
		float humidity;
		float temperature;
		uint16_t battery;
	};

/* 	----------------------------------
 	|	SmartCitizen Kit Baseboard   |
 	----------------------------------
*/
/* Sck Base board class
 *
 */
class SckBase {
public:
   
	void setup();
	void update();

	String version = "SCK-1.5_0.1-";

	// Sensors
	sensorData payloadData;
	float postInterval = 15;   // seconds
	float intervalTimer;
	float lastPublishTime;
	float publishAnswerTimeout = 5000;

	//FLAGS
	bool onWifi = false;
	bool hostNameSet = false;
	bool helloPublished = false;
	bool onTime = false;
	uint32_t lastTimeSync = 0;
	bool onBattery = false;
	bool charging = false;

	//MODES
	void changeMode(SCKmodes newMode);
	SCKmodes mode = MODE_FIRST_BOOT;
	SCKmodes prevMode = MODE_FIRST_BOOT;
	String modeTitles[MODE_count] PROGMEM;
		
	//EXTERNAL COMMANDS
	const String comTitles[30] PROGMEM = {

		"esp bridge on", 		// 0
		"esp bridge off",		// 1
		"esp flash",			// 2
		"esp reboot",			// 3
		"esp off",				// 4
		"esp on",				// 5
		"esp start",			// 6
		"esp listap",			// 7
		"setwifi",				// 8
		"settoken",				// 9
		"esp command",			// 10
		"outlevel",				// 11
		"urban present",		// 12
		"reset cause",			// 13
		"reset",				// 14
		"sdcard mode",			// 15
		"net mode",				// 16
		"time",					// 17
		"sync time",			// 18
		"last sync",			// 19
		"aux",					// 20
		"publish",				// 21
		"gettoken",				// 22
		"shell mode",			// 23
		"getmode",				// 24
		"getversion",			// 25
		"sleep",				// 26
		"led off",				// 27
		"get espTime",			// 28
		"help"					// 29

	};

	//INPUT/OUTPUT
	float baudrate = 115200;
	void inputUpdate();
	void sckIn(String strIn);
	enum outLevels {OUT_SILENT, OUT_NORMAL, OUT_VERBOSE};
	enum prioLevels {PRIO_LOW, PRIO_MED, PRIO_HIGH};
	void sckOut(String strOut, prioLevels priority=PRIO_MED, bool newLine=true);
	outLevels outputLevel = OUT_NORMAL;
	outLevels prevOutputLevel = OUT_NORMAL;
	void prompt();

	//BUTTON -- pensar de nuevo como organizar el boton y sus metodos
	void buttonEvent();
	void buttonDown();
	void buttonUp();
	void shortPress();
	void longPress();
	void veryLongPress();
	void softReset();
	float longPressInterval = 3000;
	float veryLongPressInterval = 15000;
	void longPressStillDown();
	void veryLongPressStillDown();
	bool longPressStillDownTrigered = false;
	bool veryLongPressStillDownTrigered = false;
	void factoryReset();

	//ESP8266
	enum ESPcontrols { ESP_OFF, ESP_FLASH, ESP_BRIDGE_ON, ESP_BRIDGE_OFF, ESP_ON, ESP_REBOOT };
	void ESPcontrol(ESPcontrols myESPControl);
	bool ESPon;
	bool ESPworking = false;
	void ESPsend(String payload);
	String ESPsendCommand(String command, float timeout=2000, bool external=false);
	void ESPsetWifi(String ssid, String pass);
	void ESPsetToken(String token);
	enum espMes {
		ESP_NOT_COMMAND,
		ESP_WIFI_CONNECTED,
		ESP_WIFI_ERROR,
		ESP_WIFI_ERROR_PASS,
		ESP_WIFI_ERROR_AP,
		ESP_TIME_FAIL,
		ESP_TIME_NEW,
		ESP_MODE_AP,
		ESP_MODE_STA,
		ESP_WEB_STARTED,
		ESP_MQTT_HELLO_OK,
		ESP_MQTT_PUBLISH_OK,
		ESP_MQTT_ERROR,
		ESP_HOSTNAME_UPDATED
	};
	void espMessage(String message);
	void ESPpublish();
	uint8_t publishRetryCounter = 0;
	uint8_t maxPublishRetry = 3;
	float netStatusTimer;
	float netStatusPeriod = 5000;
	float espLastOn;
	float espTotalOnTime = 0;

	// Time
	bool setTime(String epoch);
	String ISOtime();

	// SDcard
	float FileSizeLimit = 64000000;
	bool sdPresent();
	File publishFile;
	String publishFileName = "POST001.CSV";
	bool openPublishFile();
	// File logFile;
	// String logFileName = "sck.log";
	// void openLogFile();


	//TEMP hay que acomodar
	void writeResistor(byte resistor, float value );
	float readResistor(byte resistor);
	void writeCurrent(int current);
	byte readI2C(int deviceaddress, byte address );
  	void writeI2C(byte deviceaddress, byte address, byte data );

	// LightRead
	ReadLight readLight;
	dataLight lightResults;
	bool readLightEnabled = true;

	// Serial buffers
	String serialBuff;
	String espBuff;

	// Peripherals
	Button button;
	Led led;
	RTCZero rtc;
	
	// Urban board
	friend class SckUrban;
	bool urbanBoardDetected();

	// Power
	void goToSleep();
	void wakeUp();

private:
};

// Utility functions
String leadingZeros(String original, int decimalNumber);

// Hardware timers
void configureTimer5(uint16_t periodMS);
void TC5_Handler (void);
