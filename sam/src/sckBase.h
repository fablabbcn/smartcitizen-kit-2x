#pragma once

// #define ARDUINO_ARCH_SAMD

#include <Arduino.h>
#include <RTCZero.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include "WatchdogSAMD.h"
// #include <Scheduler.h>
#include <ArduinoJson.h>
#include <FlashStorage.h>

#include <Bridge.h>
#include <Sensors.h>
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
#define VCC 	3300.			// mV
#define PS 		38				// PA13 -- TPS63001 PS/SYNC

// ACOMODAR Y CAMBIAR NOMBRES ESTO ES TEMP
#define RESOLUTION_ANALOG    4095.   //Resolucion de las entradas analogicas
#define ADC_DIR             0x48    // Direction of the ADC
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

#define ESP_FLASH_SPEED 230400


/* 	-----------------
 	|	 Modes 	|
 	-----------------
*/
enum SCKmodes {
	MODE_AP,
	MODE_NET,
	MODE_SD,
	MODE_SHELL,
	MODE_FLASH,
	MODE_BRIDGE,
	MODE_ERROR,
	MODE_FIRST_BOOT,
	MODE_OFF,
	MODE_COUNT
};


/* 	-----------------
 	|	 Button 	|
 	-----------------
*/
void ISR_button();

void ISR_alarm();


/* Color definition
 *
 */
struct HSIcolor {
	float h;
	float s;
	float i;
};

struct RGBcolor {
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
	enum pulseModes {PULSE_SOFT, PULSE_HARD_SLOW, PULSE_HARD_FAST, PULSE_STATIC};
	void setup();
	void update(SCKmodes newMode, uint8_t newPulseMode=0);
	void reading();
	void off();
	void bridge();
	void wifiOK();
	void tick();

	// Need a retouch
	HSIcolor whiteHSI 		= {180,	0.5,	1.0};
	HSIcolor redHSI 		= {4,	0.92,	1.0};
	HSIcolor greenHSI 		= {120,	1.0,	1.0};
	HSIcolor blueHSI 		= {233,	1.0,	1.0};
	HSIcolor pinkHSI 		= {308,	0.85,	1.0};
	HSIcolor yellowHSI 		= {26,	0.87,	1.0};
	HSIcolor orangeHSI 		= {10,	1.0,	1.0};
	HSIcolor lightBLueHSI 	= {170, 1.0,	1.0};


	RGBcolor whiteRGB 		= {254,	254,	254};
	RGBcolor redRGB 		= {250,	4,		6};
	RGBcolor greenRGB 		= {0,	254,	0};
	RGBcolor blueRGB 		= {0,	29,		225};
	RGBcolor pinkRGB 		= {129,	12,		112};
	RGBcolor yellowRGB 		= {154,	100,	0};
	RGBcolor orangeRGB 		= {215,	39,		0};
	RGBcolor lightBlueRGB 	= {0, 	140,	114};
	RGBcolor lightGreenRGB 	= {0, 	254,	50};
	RGBcolor offRGB 		= {0, 	0,		0};

	const RGBcolor pulseBlue[25] PROGMEM = {{0,1,9},{0,2,18},{0,3,27},{0,4,36},{0,5,45},{0,7,54},{0,8,63},{0,9,72},{0,10,81},{0,11,90},{0,13,99},{0,14,108},{0,15,117},{0,16,126},{0,17,135},{0,19,144},{0,20,153},{0,21,162},{0,22,171},{0,23,180},{0,25,189},{0,26,198},{0,27,207},{0,28,216},{0,29,225}};
	const RGBcolor pulseRed[25] PROGMEM	= {{250,4,0},{240,4,0},{230,4,0},{220,4,0},{210,4,0},{200,3,0},{190,3,0},{180,3,0},{170,3,0},{160,3,0},{150,2,0},{140,2,0},{130,2,0},{120,2,0},{110,2,0},{100,1,0},{90,1,0},{80,1,0},{70,1,0},{60,1,0},{50,0,0},{40,0,0},{30,0,0},{20,0,0},{10,0,0}};
	const RGBcolor pulsePink[25] PROGMEM = {{5,0,4},{10,1,8},{15,1,13},{20,2,17},{25,2,22},{31,3,26},{36,3,31},{41,4,35},{46,4,40},{51,5,44},{57,5,49},{62,6,53},{67,6,58},{72,7,62},{77,7,67},{83,8,71},{88,8,76},{93,9,80},{98,9,85},{103,10,89},{109,10,94},{114,11,98},{119,11,103},{124,12,107},{129,12,112}};
	const RGBcolor pulseOFF[25] PROGMEM = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};

	// Hardware timer
	uint8_t refreshPeriod = 40;

	// Hard pulses
	uint16_t slowHard = 300;
	uint16_t fastHard = 80;
	uint32_t hardTimer; 

	pulseModes pulseMode = PULSE_SOFT;
	uint32_t timerReading;   //substituir esto por una libreria de timers

private:
	bool dir;
	int colorIndex = 0;
	RGBcolor ledRGBcolor;
	const RGBcolor *currentPulse;
	void setRGBColor(RGBcolor myColor);
	void setHSIColor(float h, float s, float i);
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

	// Timer
	bool timerRun();
	enum TimerAction { 
		ACTION_NULL, 
		ACTION_CLEAR_ESP_BOOTING,
		ACTION_ESP_ON,
		ACTION_ESP_REBOOT,
		ACTION_CHECK_SD,
		ACTION_GET_ESP_STATUS, 
		ACTION_LONG_PRESS,
		ACTION_VERY_LONG_PRESS, 
		ACTION_FACTORY_RESET,
		ACTION_READING_FINISHED,
		ACTION_PUBLISH,
	};
	struct OneTimer	{
		TimerAction action = ACTION_NULL;
		bool periodic = false;
		uint32_t interval = 0;
		uint32_t started = 0;
	};
	static const uint8_t timerSlots = 8;
	OneTimer timers[timerSlots];
	void timerSet(TimerAction action, uint16_t interval, bool isPeriodic=false);
	bool timerClear(TimerAction action);
	bool timerExists(TimerAction action);

	// Sensors (REACOMODAR)
	Sensors sensors;
	const uint8_t READING_MAX_TIME = 5;			// In seconds
	void sensorRead(SensorType toRead);
	void sensorReadAll();
	bool readingFinished();
	void sensorPublish();
	bool ESPpublish();
	bool publishToSD(bool platformPublishedOK=false);
	float lastPublishTime;

	// Configuration
	String version = "SCK-1.5_0.1-";
	Credentials credentials;
	void sendNetwork();
	void clearNetworks();
	char token[8];
	void sendToken();
	Configuration configuration;
	void saveConf();
	
	// Flags
	bool onWifi = false;
	bool hostNameSet = false;
	bool helloPublished = false;
	bool onTime = false;
	bool onBattery = false;
	bool charging = false;

	// Modes
	void changeMode(SCKmodes newMode);
	SCKmodes mode = MODE_FIRST_BOOT;
	SCKmodes prevMode = MODE_FIRST_BOOT;
	String modeTitles[MODE_COUNT] PROGMEM;


	// External Commands
	enum ExternalCommand {
		// Esp commands
		EXTCOM_ESP_REBOOT,
		EXTCOM_ESP_OFF,
		EXTCOM_ESP_ON,
		EXTCOM_ESP_START_AP,
		EXTCOM_ESP_STOP_AP,
		EXTCOM_ESP_START_WEB,
		EXTCOM_ESP_STOP_WEB,
		EXTCOM_ESP_SLEEP,
		EXTCOM_ESP_WAKEUP,
		EXTCOM_GET_APLIST,
		EXTCOM_ESP_SERIAL_DEBUG_ON,
		EXTCOM_ESP_SERIAL_DEBUG_OFF,
		EXTCOM_ESP_LED_ON,
		EXTCOM_ESP_LED_OFF,

		// Configuration commands
		EXTCOM_SET_WIFI,
		EXTCOM_GET_WIFI,
		EXTCOM_GET_BEST_WIFI,
		EXTCOM_CLEAR_WIFI,
		EXTCOM_GET_IP,
		EXTCOM_SET_TOKEN,
		EXTCOM_GET_TOKEN,
		EXTCOM_CLEAR_TOKEN,
		EXTCOM_GET_VERSION,
		EXTCOM_SYNC_CONFIG,
		EXTCOM_DOWNLOAD_CONFIG,
		EXTCOM_SET_CONFIG,			// @params: ToDo
		EXTCOM_GET_CONFIG,

		// Mode commands
		EXTCOM_RESET,
		EXTCOM_RESET_CAUSE,
		EXTCOM_GET_MODE,
		EXTCOM_SET_MODE,			// @params: net, shell, sdcard, bridge, flash, sleep, off
		
		// Other configuration
		EXTCOM_SET_OUTLEVEL,
		EXTCOM_GET_OUTLEVEL,
		EXTCOM_SET_LED,				// @params: off, (to implement: red, blue, green, etc)
		EXTCOM_GET_URBAN_PRESENT,
		EXTCOM_READLIGHT_ON,
		EXTCOM_READLIGHT_OFF,
		EXTCOM_READLIGHT_RESET,

		// Time configuration
		EXTCOM_SET_TIME,			// @params: epoch time
		EXTCOM_SYNC_TIME,

		// SD card
		EXTCOM_SD_PRESENT,

		// Sensor readings
		EXTCOM_GET_TIME,			// @params: iso (default), epoch
		EXTCOM_GET_APCOUNT,
		EXTCOM_GET_NOISE,
		EXTCOM_GET_HUMIDITY,
		EXTCOM_GET_TEMPERATURE,
		EXTCOM_GET_BATTERY,
		EXTCOM_GET_LIGHT,
		EXTCOM_GET_CO,
		EXTCOM_GET_NO2,
		EXTCOM_GET_VOLTIN,
		EXTCOM_GET_ALPHADELTA,
		EXTCOM_ALPHADELTA_POT,

		EXTCOM_PUBLISH,

		// Other
		EXTCOM_GET_CHAN0,
		EXTCOM_GET_CHAN1,
		EXTCOM_HELP,

		// Count
		EXTCOM_COUNT

	};

	String comTitles[EXTCOM_COUNT] PROGMEM;

	//INPUT/OUTPUT
	float baudrate = 115200;
	void inputUpdate();
	void sckIn(String strIn);
	enum OutLevels {
		OUT_SILENT,
		OUT_NORMAL,
		OUT_VERBOSE,
		OUT_COUNT
	};
	const String outLevelTitles[OUT_COUNT] PROGMEM = {
		"Silent",
		"Normal",
		"Verbose"
	};
	enum PrioLevels {PRIO_LOW, PRIO_MED, PRIO_HIGH};
	void sckOut(String strOut, PrioLevels priority=PRIO_MED, bool newLine=true);
	void changeOutputLevel(OutLevels newLevel);
	OutLevels outputLevel = OUT_NORMAL;
	OutLevels prevOutputLevel = OUT_NORMAL;
	void prompt();

	//BUTTON -- pensar de nuevo como organizar el boton y sus metodos
	void buttonEvent();
	void buttonDown();
	void buttonUp();
	void longPress();
	void veryLongPress();
	void softReset();
	uint16_t longPressInterval = 5000;
	uint16_t veryLongPressInterval = 15000;
	uint32_t butLastEvent = 0;
	bool butIsDown = false;
	void checkFactoryReset();
	void factoryReset();


	//ESP8266
	enum ESPcontrols { ESP_OFF, ESP_FLASH, ESP_ON, ESP_REBOOT };
	void ESPcontrol(ESPcontrols myESPControl);
	void ESPqueueMsg(bool sendParam=true, bool waitAnswer=false);
	void ESPbusUpdate();
	void ESPprocessMsg();
	BUS_Serial msgIn;
	BUS_Serial msgOut;
	BUS_Serial msgBuff;
	BUS_Serial BUS_queue[4];
	const uint16_t answerTimeout = 250;
	int BUS_queueIndex = -1;
	bool espSerialDebug = false;
	bool ESPon = false;
	bool ESPbooting = false;
	float espLastOn;
	float espTotalOnTime = 0;
	// --- esp status
	void getStatus();
	void processStatus();
	ESPstatus espStatus;
	ESPstatus prevEspStatus;
	float statusPoolingInterval = 1000;		// ESP status pooling interval in ms

	// Time
	bool setTime(String epoch);
	String ISOtime();
	String epoch2iso(uint32_t epochTime);

	// SDcard
	// SdFat sd;
	float FileSizeLimit = 64000000;
	bool sdPresent();
	// File publishFile;
	String publishFileName = "POST001.CSV";
	bool openPublishFile();
	// File logFile;
	String logFileName = "SCK.LOG";
	String oldLogFileName = "SCK_OLD.LOG";
	bool openLogFile();

	// Battery
	uint16_t getBatteryVoltage();
	uint16_t getCharger();
	uint16_t getChann0();
	uint16_t getChann1();
	const uint16_t batteryMax = 4208;
	const uint16_t batteryMin = 3000;
	uint16_t readADC(byte channel);
	bool isCharging = false;


	//TEMP hay que acomodar
	void writeResistor(byte resistor, float value );
	float readResistor(byte resistor);
	void writeCurrent(int current);
	byte readI2C(int deviceaddress, byte address );
  	void writeI2C(byte deviceaddress, byte address, byte data );

	// LightRead
	ReadLight readLight;
	dataLight lightResults;
	bool readLightEnabled = false;

	// Serial buffers
	String serialBuff;
	String espBuff;

	// Peripherals
	Led led;
	RTCZero rtc;
	
	// Urban board
	friend class SckUrban;
	bool urbanBoardDetected();
	bool urbanPresent = false;

	// Power
	void goToSleep(bool wakeToCheck = true);
	void wakeUp();

private:
};

// Utility functions
String leadingZeros(String original, int decimalNumber);

// Hardware timers
void configureTimer5(uint16_t periodMS);
void TC5_Handler (void);