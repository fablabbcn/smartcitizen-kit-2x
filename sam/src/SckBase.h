#pragma once

/* #define testing // Uncomment this line for running test protocol */

#include <Arduino.h>
#include <RTCZero.h>
#include <time.h>
#include <SPI.h>
#include "SdFat.h"
#include "SAMD_pinmux_report.h"
#include "wiring_private.h"
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <FlashStorage.h>
#include <ArduinoJson.h>
#include <LinkedList.h>

#include "Pins.h"
#include "SckLed.h"
#include "SckBatt.h"
#include "Shared.h"
#include "Config.h"
#include "Commands.h"
#include "Sensors.h"
#include "SckUrban.h"
#include "SckAux.h"
#include "SckList.h"

#include "version.h"

class Status
{

	private:
		uint32_t _lastTryMillis = 0; // ms
		uint32_t _timeout; 	// ms

	public:
		bool ok = false;
		bool error = false;
		uint8_t retrys = 0;
		uint8_t _maxRetrys;

		bool retry();
		void setOk();
		void reset();

	Status(uint8_t maxRetrys=5, uint32_t timeout=1000)
{
		_maxRetrys = maxRetrys;
		_timeout = timeout;
}
};

struct SckState
{
	bool onShell = false;
	bool onSetup = false;
	bool espON = false;
	bool espBooting = false;
	bool wifiSet = false;
	bool tokenSet = false;
	bool tokenError = false;
	bool helloPending = false;
	SCKmodes mode = MODE_NET;
	bool cardPresent = false;
	bool cardPresentErrorPrinted = false;
	bool sleeping = false;
	bool publishPending = false;
	bool dynamic = false;
	Status wifiStat = Status(1, 60000);
	Status timeStat = Status(2, 3000);
	Status helloStat = Status(3, 5000);
	Status infoStat = Status(3, 5000);
	Status publishStat = Status(3, 5000);
	errorType error = ERROR_NONE;
	uint32_t lastWiFiError = 0;
	uint16_t wifiErrorCounter = 0;

	inline bool operator==(SckState a) {
		if (	a.onSetup == onSetup
				&& a.espON == espON
				&& a.wifiSet == wifiSet
				&& a.tokenSet == tokenSet
				&& a.helloPending == helloPending
				&& a.mode == mode
				&& a.cardPresent == cardPresent
				&& a.sleeping == sleeping
		) return true;
		else return false;
	}
};

class SckBase
{
	private:
		// Input/Output
		String serialBuff;
		String previousCommand;
		uint8_t outRepetitions = 0;

		// **** ESP control
		uint32_t espStarted;

		// **** Time
		const uint8_t TIME_TIMEOUT = 20;		// seconds

		// **** Mode Control
		void reviewState();
		uint32_t reviewStateMillis = 0;
		void enterSetup();

		// ESP communication
		uint8_t netPack[NETPACK_TOTAL_SIZE];
		char netBuff[NETBUFF_SIZE];
		void ESPbusUpdate();
		void receiveMessage(SAMMessage wichMessage);
		bool sendConfig();
		uint32_t sendConfigTimer = 0;
		uint8_t sendConfigCounter = 0;
		bool pendingSyncConfig = false;
		uint32_t generalUpdateTimer = 0;

		// Button
		const uint16_t buttonLong = 5000;
		const uint16_t buttonVeryLong = 15000;
		bool butOldState = true;
		bool wakingUp = false;
		void buttonEvent();
		void buttonStillDown();

		// Configuration
		void loadConfig();
		bool publishInfo();
		bool espInfoUpdated = false;
		bool infoPublished = false;

		// STORAGE
		// files
		struct SckFile {char name[16]; File file;};
		SckFile configFile {"CONFIG.TXT"};
		SckFile postFile {};
		SckFile debugFile {"DEBUG.TXT"};
		SckFile speedFile {"SPEED.CSV"};
		SckFile infoFile {"INFO.TXT"};
		SckFile errorFile {"ERROR.LOG"};
		// Sd card
		bool sdSelect();
		volatile bool sdInitPending = false;
		bool sdInit();
		bool saveInfo();
		bool infoSaved = false;

		// Power
		uint8_t wakeUP_H = 3;
		uint8_t wakeUP_M = 0;
		void updatePower();
		uint32_t updatePowerMillis = 0;
		void configGCLK6(); 			// Taken from https://github.com/arduino-libraries/ArduinoLowPower
		void sleepLoop();

		// **** Sensors
		uint32_t lastPublishTime = 0; 			// seconds
		uint32_t lastSensorUpdate = 0;
		bool timeToPublish = false;
		void urbanStart();
		void updateSensors();
		bool netPublish();
		bool sdPublish(); 				//  Publishes the provided group of readings to sdcard (if available)
		LinkedList<SensorType> pendingSensorsLinkedList;

		SckList::GroupIndex wichGroupPublishing; 	// Index of the group beeing published (already sent to ESP and waiting for OK/ERROR response), -1 if there is none.
		uint32_t dynamicLast = 0; 			// Last time that we detected enough speed to trigger dynamic interval
		uint8_t dynamicCounter = 0;
		const uint8_t DYNAMIC_COUNTER_THRESHOLD = 3; 	// Number of high speed repetitions that triggers dynamic interval
		uint32_t lastSpeedMonitoring = 0; 		// Last time we check the speed to trigger dynamic interval
		const uint8_t DYNAMIC_TIMEOUT = 60; 		// When low speed is detected let this time (seconds) pass before changing dynamic interval off. (This helps with noise in speed data).
		const uint16_t DYNAMIC_HDOP_THRESHOLD = 350; 	// If HDOP is grater than this we ignore the reported speed (this help cleaning false high speed dynamic trigger's)
		const float SPEED_ALPHA = 2; 			// 0-10 - small values -> more smooth, big values -> closer to original data.
		void updateDynamic(uint32_t now);

	public:
		const String hardwareVer = "2.1";
		const String SAMversion	= SAMverNum + "-" + String(__GIT_HASH__); 		// mayor.minor.build-gitcommit
		const String SAMbuildDate = String(__ISO_DATE__);
		String ESPversion = "not synced";
		String ESPbuildDate = "not synced";
		// If mayor or minor version of ESP is different than SAM's we need to call a ESP update
		bool ESPupdateNeeded = false;

		void setup();
		void update();

		// **** Mode Control
		SckState st;

		// **** Time
		RTCZero rtc;
		bool timeSyncAfterBoot = false;
		uint32_t lastTimeSync = 0;
		char ISOtimeBuff[20];
		bool setTime(String epoch);
		bool ISOtime();
		void epoch2iso(uint32_t toConvert, char* isoTime);

		// Peripherals
		SckLed led;
		friend class SckButton;

		// **** Sensors
		AllSensors sensors;
		bool getReading(OneSensor *wichSensor);
		bool controlSensor(SensorType wichSensorType, String wichCommand);
		bool enableSensor(SensorType wichSensor);
		bool disableSensor(SensorType wichSensor);
		bool writeHeader = false;

		// Urban board
		bool urbanPresent = false;
		friend class urban;
		SckUrban urban = SckUrban(&rtc);

		// Flash readings store
		friend class SckList;
		SckList readingsList = SckList(this);

		// Configuration
		Configuration config;
		Configuration getConfig();
		void saveConfig(bool defaults=false);

		// Input/Output
		void inputUpdate();

		// ESP control
		enum ESPcontrols { ESP_OFF, ESP_FLASH, ESP_ON, ESP_REBOOT, ESP_SLEEP, ESP_WAKEUP };
		void ESPcontrol(ESPcontrols myESPControl);
		uint32_t espFlashSpeed = 115200;

		// ESP communication
		bool sendMessage(ESPMessage wichMessage, const char *content);
		bool sendMessage(ESPMessage wichMessage);
		bool sendMessage();
		String ipAddress;
		char hostname[17];
		void mqttCustom(const char *topic, const char *payload);

		// Output
		const char *outLevelTitles[OUT_COUNT] PROGMEM = { "Silent", "Normal", "Verbose"	};
		char outBuff[240];
		void sckOut(const char *strOut, PrioLevels priority=PRIO_MED, bool newLine=true);	// Accepts constant string
		void sckOut(PrioLevels priority=PRIO_MED, bool newLine=true);
		void prompt();
		void plot(String value, const char *title=NULL, const char *unit=NULL);

		// Button
		volatile bool butState = true;
		volatile uint32_t lastUserEvent = 0;
		void butFeedback();

		// Commands
		AllCommands commands;

		// SDcard
		SdFat sd;
		bool sdDetect();
		SckFile monitorFile {"MONITOR.CSV"};

		// Power
		void sck_reset();
		void goToSleep(uint32_t sleepPeriod=3000); 	// sleepPeriod in ms
		SckBatt battery;
		SckCharger charger;
		bool sckOFF = false;

		// Misc
		void getUniqueID();
		uint32_t uniqueID[4];
		char uniqueID_str[33];

		const char *modeTitles[MODE_COUNT] PROGMEM = {
			"not configured",		// modeTitles[MODE_NOT_CONFIGURED]
			"network",			// modeTitles[MODE_NET]
			"sdcard",			// modeTitles[MODE_SD]
			"sleep"				// modeTitles[MODE_SLEEP]
		};

#ifdef testing
		const bool inTest = true;
		friend class SckTest;
#else
		const bool inTest = false;
#endif
};

bool I2Cdetect(TwoWire *_Wire, byte address);
void ISR_button();
void ISR_sdDetect();
void ext_reset();
