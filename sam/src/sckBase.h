#pragma once

#include <Arduino.h>
#include <RTCZero.h>
#include <SPI.h>
#include "ArduinoLowPower.h"

#include "sckLed.h"
#include "sckPot.h"

// Output
enum OutLevels { OUT_SILENT, OUT_NORMAL, OUT_VERBOSE, OUT_COUNT	};
enum PrioLevels { PRIO_LOW, PRIO_MED, PRIO_HIGH };

class SckBase {
private:

	const uint8_t pinPwrSave = 38; 		// PA13 -- TPS63001 PS/SYNC (Enable/disable power-save mode (1 disabled, 0 enabled)

	// Button intervals
	const uint16_t buttonLong = 4000;
	const uint16_t buttonVeryLong = 11000;
	const uint32_t buttonLastEvent = 0;

	// ESP Comunication
	const uint32_t serialBaudrate = 115200;

	// Output
	const char *outLevelTitles[OUT_COUNT] PROGMEM = { "Silent",	"Normal", "Verbose"	};
	char outBuff[240];

	// Button
	const uint8_t pinBUTTON = 7;		// PA21
	bool alarmConfigured_TC4 = false;
	const uint16_t longPressInterval = 5000;
	const uint16_t veryLongPressInterval = 13000;
	uint32_t butLastEvent = 0;
	enum ButStage { BUT_STARTED, BUT_LONG_PRESS, BUT_VERY_LONG };
	ButStage butStage = BUT_STARTED;
	


	// To ORGANIZE
	bool onUSB = true;



public:

	void setup();
	void update();

	// Peripherals
	SckLed led;
	friend class SckButton;
	RTCZero rtc;

	// Output
	void sckOut(String strOut, PrioLevels priority=PRIO_MED, bool newLine=true);	// Accepts String object
	void sckOut(const char *strOut, PrioLevels priority=PRIO_MED, bool newLine=true);	// Accepts constant string
	void sckOut(PrioLevels priority=PRIO_MED, bool newLine=true);
	void prompt();
	OutLevels outputLevel = OUT_VERBOSE;

	// Button
	void buttonEvent();
	void buttonStillDown();
	void longPress();
	void veryLongPress();
	void configureAlarm_TC4(uint16_t periodMS);
	void disableAlarm_TC4();


};

void ISR_button();
void ISR_longPress();