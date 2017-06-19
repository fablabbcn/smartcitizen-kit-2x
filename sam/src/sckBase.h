#pragma once

#include <Arduino.h>
#include <RTCZero.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include <ArduinoJson.h>
#include <FlashStorage.h>

#include <Bridge.h>
#include <Sensors.h>
#include "sckAux.h"
#include "ReadLight.h"
#include "sckUrban.h"
#include "Constants.h"


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
	void configOK();
	float brightnessFactor = 1;
	bool dim = false;

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
	RGBcolor orangeRGB 		= {235,	30,		0};
	RGBcolor lightBlueRGB 	= {0, 	140,	114};
	RGBcolor lightGreenRGB 	= {0, 	254,	50};
	RGBcolor offRGB 		= {0, 	0,		0};

	const RGBcolor pulseBlue[25] PROGMEM = {{0,1,9},{0,2,18},{0,3,27},{0,4,36},{0,5,45},{0,7,54},{0,8,63},{0,9,72},{0,10,81},{0,11,90},{0,13,99},{0,14,108},{0,15,117},{0,16,126},{0,17,135},{0,19,144},{0,20,153},{0,21,162},{0,22,171},{0,23,180},{0,25,189},{0,26,198},{0,27,207},{0,28,216},{0,29,225}};
	const RGBcolor pulseRed[25] PROGMEM	= {{10,0,0},{20,0,0},{30,0,0},{40,0,0},{50,0,0},{60,1,0},{70,1,0},{80,1,0},{90,1,0},{100,1,0},{110,2,0},{120,2,0},{130,2,0},{140,2,0},{150,2,0},{160,3,0},{170,3,0},{180,3,0},{190,3,0},{200,3,0},{210,4,0},{220,4,0},{230,4,0},{240,4,0},{250,4,0}};
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

	// Powerfeedback
	bool charging = false;
	bool lowBatt = false;
	bool finishedCharging = false;

private:
	bool dir;
	int colorIndex = 0;
	uint8_t heartBeat = 0;
	uint8_t dimLoops = 0;
	RGBcolor ledRGBcolor;
	const RGBcolor *currentPulse;
	bool inErrorColor = false;
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
		ACTION_GET_ESP_STATUS, 
		ACTION_LONG_PRESS,
		ACTION_VERY_LONG_PRESS, 
		ACTION_RESET,
		ACTION_UPDATE_SENSORS,
		ACTION_UPDATE_POWER,
		ACTION_CHECK_ESP_PUBLISH_TIMEOUT,
		ACTION_READ_NETWORKS,
		ACTION_DEBUG_LOG,
		ACTION_GOTO_SETUP,
		ACTION_RECOVER_ERROR
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
	void timerClearTasks();
	bool timerExists(TimerAction action);
	const uint8_t MAX_PUBLISH_FAILS_ALLOWED = 5;

	// Sensors
	AllSensors sensors;
	// RAM storing stuff
	// Groups
	static const uint16_t ram_max_readings = 1024;			// 5 bytes each
	SensorGroupByTime RAMgroups[ram_max_readings / 5];		// This array keeps track of sensor readings grouped by time
	int RAMgroupIndex = -1;
	SensorGroupBuffer groupBuffer;

	// Single readings
	int RAMreadingsIndex = -1;
	SingleSensorReading RAMreadings[ram_max_readings];
	uint8_t maxDiffBetweenReadings = minimal_sensor_reading_interval;		// If readings are inside this thime (sec) frame they will be considered simultaneous

	void enableSensor(SensorType wichSensor);
	void updateSensors();
	bool getReading(SensorType wichSensor);
	bool RAMstore(SensorType);
	uint16_t closestReading;		// in seconds
	bool RAMgetGroup(int groupIndex);
	bool publish();
	bool publishToSD();
	bool ESPpublish();
	uint32_t lastPublishTime = 0;
	

	// REVISAR DE ESTO QUE SE NECESITA REALMENTE BAJO EL NUEVO MODELO
	const uint8_t READING_MAX_TIME = 10;			// In seconds
	bool ESPpublishPending = false;
	const uint16_t ESP_publish_timeout_interval = 25000;	// In ms


	// Configuration ----------
	String hardwareVer 	= "1.5";
	String SAMversion	= "0.2.0";
	String SAMbuildDate = String(__DATE__) + '-' + String(__TIME__);
	String ESPversion 	= "null";
	String ESPbuildDate = "null";
	Credentials credentials;
	bool triggerHello = false;
	void sendNetwork();
	void clearNetworks();
	char token[8];
	void sendToken();
	void clearToken();
	Configuration config;
	void loadConfig();
	bool loadSDconfig();
	void saveConfig(bool factory=false);

	// Flags
	bool onWifi = false;
	bool hostNameSet = false;
	bool helloPublished = false;
	bool onTime = false;
	bool charging = false;
	bool onUSB = true;

	// Modes
	void changeMode(SCKmodes newMode);
	void errorMode();
	SCKmodes prevMode = MODE_SD;
	SCKmodes default_mode = MODE_SD;

	const char *modeTitles[MODE_COUNT] PROGMEM = {
		"setup",		// modeTitles[MODE_SETUP]
		"network",		// modeTitles[MODE_NET]
		"sdcard",		// modeTitles[MODE_SD]
		"esp flash",	// modeTitles[MODE_FLASH]
		"esp bridge",	// modeTitles[MODE_BRIDGE]
		"off"			// modeTitles[MODE_OFF]
	};

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
		EXTCOM_ESP_GET_FREEHEAP,

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
		EXTCOM_SET_CONFIG,
		EXTCOM_GET_CONFIG,

		// Mode commands
		EXTCOM_RESET,
				
		// Other configuration
		EXTCOM_SET_OUTLEVEL,
		EXTCOM_GET_OUTLEVEL,
		EXTCOM_SET_LED,
		EXTCOM_GET_URBAN_PRESENT,
		EXTCOM_READLIGHT_ON,
		EXTCOM_READLIGHT_OFF,
		EXTCOM_READLIGHT_RESET,
		EXTCOM_READLIGHT_TOGGLE_DEBUG,

		// Time configuration
		EXTCOM_GET_TIME,			// @params: iso (default), epoch
		EXTCOM_SET_TIME,			// @params: epoch time
		EXTCOM_SYNC_HTTP_TIME,

		// SD card
		EXTCOM_SD_PRESENT,

		// Sensors
		EXTCOM_GET_SENSOR,
		EXTCOM_RAM_COUNT,
		EXTCOM_RAM_READ,
		EXTCOM_PUBLISH,
		EXTCOM_LIST_SENSORS,
		EXTCOM_ENABLE_SENSOR,
		EXTCOM_DISABLE_SENSOR,
		EXTCOM_SET_INTERVAL_SENSOR,
		EXTCOM_CONTROL_SENSOR,

		// Print String to u8g2_oled screen
		EXTCOM_U8G_PRINT,			// @params: String to be printed
		EXTCOM_U8G_PRINT_SENSOR,	// @params: Sensor to be printed

		// Other
		EXTCOM_GET_POWER_STATE,
		EXTCOM_SET_CHARGER_CURRENT,
		EXTCOM_RESET_CAUSE,
		EXTCOM_GET_FREE_RAM,

		EXTCOM_HELP,

		// Count
		EXTCOM_COUNT

	};

	const char *comTitles[EXTCOM_COUNT] PROGMEM = {

	// EXTERNAL COMMAND TITLES
	// Esp Command
	"esp reboot",			// 	EXTCOM_ESP_REBOOT,
	"esp off",				// 	EXTCOM_ESP_OFF,
	"esp on",				// 	EXTCOM_ESP_ON,
	"esp start ap",			// 	EXTCOM_ESP_START_AP,
	"esp stop ap",			// 	EXTCOM_ESP_STOP_AP,
	"esp start web",		// 	EXTCOM_ESP_START_WEB,
	"esp stop web",			// 	EXTCOM_ESP_STOP_WEB,
	"esp sleep",			// 	EXTCOM_ESP_SLEEP,
	"esp wakeup",			// 	EXTCOM_ESP_WAKEUP,
	"get aplist",			// 	EXTCOM_GET_APLIST,
	"esp debug on",			// 	EXTCOM_ESP_SERIAL_DEBUG_ON,
	"esp debug off",		// 	EXTCOM_ESP_SERIAL_DEBUG_OFF,
	"esp led on",			// 	EXTCOM_ESP_LED_ON,
	"esp led off",			// 	EXTCOM_ESP_LED_OFF,
	"mqtt hello",			// 	EXTCOM_ESP_MQTT_HELLO,
	"esp heap",				// 	EXTCOM_ESP_GET_FREEHEAP,

	// Configuration commands
	"set wifi",				// EXTCOM_SET_WIFI,
	"get wifi",				// EXTCOM_GET_WIFI,
	"get best wifi",		// EXTCOM_GET_BEST_WIFI,
	"clear wifi",			// EXTCOM_CLEAR_WIFI,
	"get ip",				// EXTCOM_GET_IP,
	"set token",			// EXTCOM_SET_TOKEN,
	"get token",			// EXTCOM_GET_TOKEN,
	"clear token",			// EXTCOM_CLEAR_TOKEN,
	"get version",			// EXTCOM_GET_VERSION,
	"sync config",			// EXTCOM_SYNC_CONFIG,
	"download config",		// EXTCOM_DOWNLOAD_CONFIG,
	"set config",			// EXTCOM_SET_CONFIG,		@params: "factory", "publish interval" [seconds], "mode" [sdcard or network]
	"get config",			// EXTCOM_GET_CONFIG,

	// Mode commands
	"reset",				// EXTCOM_RESET,

	// Other configuration
	"set outlevel",			// EXTCOM_SET_OUTLEVEL,		
	"get outlevel",			// EXTCOM_GET_OUTLEVEL,
	"set led",				// EXTCOM_SET_LED,			@params: off, (to implement: red, blue, green, etc)
	"urban present",		// EXTCOM_GET_URBAN_PRESENT,
	"set readlight on",		// EXTCOM_READLIGHT_ON,
	"set readlight off",	// EXTCOM_READLIGHT_OFF,
	"set readlight reset",	// EXTCOM_READLIGHT_RESET,
	"set readlight debug",	// EXTCOM_READLIGHT_TOGGLE_DEBUG,

	// Time configuration
	"get time",				// EXTCOM_GET_TIME,			@params: iso (default), epoch
	"set time",				// EXTCOM_SET_TIME,			@params: epoch time
	"sync time",			// EXTCOM_SYNC_HTTP_TIME,

	// SD card
	"sd present",			// EXTCOM_SD_PRESENT,

	// Sensors
	"read",					// EXTCOM_GET_SENSOR, 		@params sensor Title
	"ram count",			// EXTCOM_RAM_COUNT,
	"ram read",				// EXTCOM_RAM_READ,
	"publish",				// EXTCOM_PUBLISH,
	"list sensors",			// EXTCOM_LIST_SENSORS,
	"enable",				// EXTCOM_ENABLE_SENSOR, 	@params wichSensor
	"disable",				// EXTCOM_DISABLE_SENSOR, 	@params wichSensor
	"interval",				// EXTCOM_SET_INTERVAL_SENSOR,	@params [seconds]
	"control",				// EXTCOM_CONTROL_SENSOR, 	@params wichSensor wichCommand

	// U8g_OLED commands
	"u8g print",			// EXTCOM_U8G_PRINT,		@params: String to be printed
	"u8g sensor",			// EXTCOM_U8G_PRINT_SENSOR,	@params: Sensor to be printed

	// Other
	"get power",			// EXTCOM_GET_POWER_STATE,
	"set charger",			// EXTCOM_SET_CHARGER_CURRENT,
	"rcause",				// EXTCOM_RESET_CAUSE,
	"get freeram",			// EXTCOM_GET_FREE_RAM,

	"help",					// EXTCOM_HELP,
	
	};

	//INPUT/OUTPUT
	float baudrate = 115200;
	void inputUpdate();
	void sckIn(String strIn);
	SensorType getSensorFromString(String strIn);
	enum OutLevels {
		OUT_SILENT,
		OUT_NORMAL,
		OUT_VERBOSE,
		OUT_COUNT
	};
	const char *outLevelTitles[OUT_COUNT] PROGMEM = {
		"Silent",
		"Normal",
		"Verbose"
	};
	enum PrioLevels {PRIO_LOW, PRIO_MED, PRIO_HIGH};
	char outBuff[128];
	void sckOut(const char *strOut, PrioLevels priority=PRIO_MED, bool newLine=true);
	void sckOut(String strOut, PrioLevels priority=PRIO_MED, bool newLine=true);
	void sckOut(PrioLevels priority=PRIO_MED, bool newLine=true);
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
	uint16_t veryLongPressInterval = 9000;
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
	BUS_Serial BUS_queue[8];
	const uint16_t answerTimeout = 250;
	int BUS_queueCount = 0;
	bool espSerialDebug = false;
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
	bool ISOtime();
	char ISOtimeBuff[20];
	void epoch2iso(uint32_t epochTime, char* isoTime);

	// SDcard
	uint32_t FileSizeLimit = 64000000;
	uint32_t headersChanged = 0;
	String publishFileName = "POST001.CSV";
	char configFileName[11] = "CONFIG.TXT";
	String logFileName = "SCK.LOG";
	String oldLogFileName = "SCK_OLD.LOG";
	bool sdPresent();
	bool openPublishFile(bool writeHeader=false);
	bool openLogFile();
	bool openConfigFile(bool onlyRead=false);
	void closeFiles();
	bool sdLogADC();

	// Battery
	uint16_t getBatteryVoltage();
	float getBatteryPercent();
	float getVUSB();
	uint16_t getCHG();
	uint16_t getISET();
	const uint16_t batteryMax = 4208;
	const uint16_t batteryMin = 3000;
	uint16_t readADC(byte channel);
	const uint16_t batTable[100] PROGMEM = {3078,3364,3468,3540,3600,3641,3682,3701,3710,3716,3716,3716,3720,3714,3720,3725,3732,3742,3739,3744,3744,3754,3760,3762,3770,3768,3774,3774,3774,3779,3784,3790,3788,3794,3798,3798,3804,3809,3809,3812,3817,3817,3822,3823,3828,3828,3828,3833,3838,3838,3842,3847,3852,3859,3858,3864,3862,3869,3877,3877,3883,3888,3894,3898,3902,3906,3912,3923,3926,3936,3942,3946,3960,3972,3979,3982,3991,3997,4002,4002,4012,4018,4028,4043,4057,4074,4084,4094,4098,4098,4109,4115,4123,4134,4142,4153,4158,4170,4180,4188};
	void updatePower();
	uint8_t lowBattLimit = 10;
	uint8_t lowBattEmergencySleep = 3;


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
uint8_t countMatchedWords(String title, String tested);
String cleanInput(String toRemove, String original);
size_t freeRAM(void);

// Hardware timers
void configureTimer5(uint16_t periodMS);
void TC5_Handler (void);