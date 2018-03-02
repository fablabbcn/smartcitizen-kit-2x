#pragma once

#include <Arduino.h>
#include <RTCZero.h>
#include <SPI.h>
#include "ArduinoLowPower.h"
#include "SdFat.h"
#include <SPIFlash.h>
#include <SoftwareSerial.h>		// To be removed
#include "SAMD_pinmux_report.h"
#include "wiring_private.h"

// Battery gauge
#include <SparkFunBQ27441.h>

#include "Pins.h"
#include "SckLed.h"
#include "SckPot.h"
#include "Shared.h"
#include "Config.h"
#include "Commands.h"
#include "Sensors.h"
#include "SckUrban.h"


// Output
enum OutLevels { OUT_SILENT, OUT_NORMAL, OUT_VERBOSE, OUT_COUNT	};
enum PrioLevels { PRIO_LOW, PRIO_MED, PRIO_HIGH };

class SckBase {
private:

	// Input
	String serialBuff;
	String previousCommand;

	// Button
	const uint16_t buttonLong = 5000;
	const uint16_t buttonVeryLong = 13000;
	enum ButtonStage { BUT_STARTED, BUT_LONG_PRESS, BUT_VERY_LONG };
	ButtonStage buttonStage = BUT_STARTED;
	bool alarmRunning_TC4 = false;
	uint32_t buttonLastEvent = 0;
	
	// Configuration
	Configuration config;
	// void saveSDconfig();

	// ESP8266
	const uint32_t ESP_FLASH_SPEED = 921600;

	// Urban board
	bool urbanPresent = false;
	friend class urban;
	SckUrban urban;

	// STORAGE
	// files
	struct SckFile {char name[13]; File file;};
	SckFile configFile {"CONFIG.TXT"};
	SckFile postFile {"POST.CSV"};
	SckFile debugFile {"DEBUG.CSV"};
	// Sd card
	SdFat sd;
	bool cardPresent = false;
	void sdSelect();
	bool sdOpenFile(SckFile wichFile, uint8_t oflag);
	// Flash memory
	SPIFlash flash = SPIFlash(pinCS_FLASH);
	void flashSelect();
	// bool flashOpenFile(SckFile wichFile, uint8_t oflag);
	// remember this for power management
	// flash.powerDown();
	// flash.powerUp();

	// Power
	uint16_t battCapacity = 2000;
	bool batteryPresent = false;
	bool onUSB = true;
	bool battSetup();

public:

	void setup();
	void update();

	// Peripherals
	SckLed led;
	friend class SckButton;
	RTCZero rtc;

	// Sensors
	AllSensors sensors;
	bool getReading(SensorType wichSensor, bool wait=true);

	// Input
	void inputUpdate();

	// Output
	const char *outLevelTitles[OUT_COUNT] PROGMEM = { "Silent",	"Normal", "Verbose"	};
	OutLevels outputLevel = OUT_VERBOSE;
	char outBuff[240];
	void sckOut(String strOut, PrioLevels priority=PRIO_MED, bool newLine=true);	// Accepts String object
	void sckOut(const char *strOut, PrioLevels priority=PRIO_MED, bool newLine=true);	// Accepts constant string
	void sckOut(PrioLevels priority=PRIO_MED, bool newLine=true);
	void prompt();

	// Button
	void buttonEvent();
	void buttonStillDown();
	void setAlarm_TC4(uint16_t lapse=0);

	// Commands
	AllCommands commands;

	// ESP control
	enum ESPcontrols { ESP_OFF, ESP_FLASH, ESP_ON, ESP_REBOOT };
	void ESPcontrol(ESPcontrols myESPControl);
	bool flashingESP = false;
	uint32_t espStarted;

	// SDcard
	bool sdDetect();

	// Power
	void reset();
	void batteryEvent();
	void batteryReport();

};

void ISR_button();
void ISR_battery();
void ISR_sdDetect();
