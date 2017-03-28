#pragma once

#include <Arduino.h>
#include <RTCZero.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include "WatchdogSAMD.h"
#include <ArduinoJson.h>
#include <FlashStorage.h>

#include <Bridge.h>
#include <Sensors.h>
#include "sckAux.h"
#include "ReadLight.h"
#include "sckUrban.h"
#include "Constants.h"


/* 	-----------------
 	|	 Modes 		|
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


/* 	---------------------------------------------------------
	|	Persistent Variables (eeprom emulation on flash)	|
	---------------------------------------------------------
*/
struct EppromMode {
	bool valid;
	SCKmodes mode;
};

struct EppromConf {
	bool valid;
	uint32_t readInterval;
};


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
	void configOK();

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
		ACTION_CHECK_ESP_PUBLISH_TIMEOUT,
		ACTION_READ_NETWORKS
	};
	struct OneTimer	{
		TimerAction action = ACTION_NULL;
		bool periodic = false;
		uint32_t interval = 0;
		uint32_t started = 0;
	};
	static const uint8_t timerSlots = 8;
	OneTimer timers[timerSlots];
	void timerSet(TimerAction action, uint32_t interval, bool isPeriodic=false);		// interval is in milliseconds
	bool timerClear(TimerAction action);
	bool timerExists(TimerAction action);

	// Sensors (REACOMODAR)
	AllSensors sensors;
	void sensorRead(SensorType wichSensor);
	bool getReading(SensorType wichSensor);

	float getBatteryVoltage();
	float getBatteryPercent();
	float getCharger();


	const uint8_t READING_MAX_TIME = 10;			// In seconds
	void publish();
	bool readingFinished();
	void sensorPublish();
	void ESPpublish();
	bool ESPpublishPending = false;
	const uint16_t ESP_publish_timeout_interval = 5000;	// In ms
	bool publishToSD(bool platformPublishedOK=false);
	float lastPublishTime;

	// Configuration
	String version = "SCK-1.5_0.1-";
	Credentials credentials;
	bool triggerHello = false;
	void sendNetwork();
	void clearNetworks();
	char token[8];
	void sendToken();
	Configuration configuration;
	void saveConf();
	void setReadInterval(uint32_t newReadInterval);
	
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
		EXTCOM_ESP_MQTT_HELLO,

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
		EXTCOM_GET_TIME,			// @params: iso (default), epoch
		EXTCOM_SET_TIME,			// @params: epoch time
		EXTCOM_SYNC_TIME,

		// SD card
		EXTCOM_SD_PRESENT,

		// Sensor readings
		EXTCOM_GET_SENSOR,
		EXTCOM_SET_SENSOR,
		EXTCOM_PUBLISH,
		EXTCOM_ENABLE_SENSOR,
		EXTCOM_DISABLE_SENSOR,

		// Set Alpha POTs
		EXTCOM_ALPHADELTA_POT,

		// Other
		EXTCOM_GET_CHAN0,
		EXTCOM_GET_CHAN1,
		EXTCOM_GET_FREEHEAP,
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
	uint16_t veryLongPressInterval = 10000;
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
	float statusPoolingInterval = 500;		// ESP status pooling interval in ms
	const uint32_t ESP_FLASH_SPEED = 230400;

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
	bool headersChanged = false;

	// Battery
	uint16_t getChann0();
	uint16_t getChann1();
	const uint16_t batteryMax = 4208;
	const uint16_t batteryMin = 3000;
	uint16_t readADC(byte channel);
	bool isCharging = false;
	const uint16_t batTable[100] = {3078,3364,3468,3540,3600,3641,3682,3701,3710,3716,3716,3716,3720,3714,3720,3725,3732,3742,3739,3744,3744,3754,3760,3762,3770,3768,3774,3774,3774,3779,3784,3790,3788,3794,3798,3798,3804,3809,3809,3812,3817,3817,3822,3823,3828,3828,3828,3833,3838,3838,3842,3847,3852,3859,3858,3864,3862,3869,3877,3877,3883,3888,3894,3898,3902,3906,3912,3923,3926,3936,3942,3946,3960,3972,3979,3982,3991,3997,4002,4002,4012,4018,4028,4043,4057,4074,4084,4094,4098,4098,4109,4115,4123,4134,4142,4153,4158,4170,4180,4188 };

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