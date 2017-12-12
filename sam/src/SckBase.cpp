#include "SckBase.h"
#include "Commands.h"

void SckBase::setup() {

	// Serial Port Configuration (ESP communication)
	Serial1.begin(serialBaudrate);

	// Output
	outputLevel = OUT_VERBOSE;

	// Power management configuration
  	pinMode(pinPwrSave, OUTPUT);
  	digitalWrite(pinPwrSave, HIGH);

  	// Pause for a moment (for uploading firmware in case of problems)
	delay(2000);

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Peripheral setup
	led.setup();
	rtc.begin();

	// Configuration
	
	// TEMP
	config.mode = MODE_NET;
}
void SckBase::update() {

	if (config.mode == MODE_FLASH) {

	} else {

		// Check Serial port input
		inputUpdate();

	}
}


// Input
void SckBase::inputUpdate() {

	if (onUSB) {
		if (SerialUSB.available()) {

			char buff = SerialUSB.read();
			uint16_t blen = serialBuff.length();
			
			// New line
			if (buff == 13 || buff == 10) {

				SerialUSB.println();				// Newline
				commands.in(this, serialBuff);		// Process input
				serialBuff = "";
				prompt();

			// Backspace
			} else if (buff == 127) {

				if (blen > 0) SerialUSB.print("\b \b");
				serialBuff.remove(blen-1);

			// Normal char
			} else {

				serialBuff += buff;
				SerialUSB.print(buff);				// Echo

			}
		}
	}
}

// Output
void SckBase::sckOut(String strOut, PrioLevels priority, bool newLine) {
	strOut.toCharArray(outBuff, strOut.length()+1);
	sckOut(priority, newLine);
}
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine) {
	strncpy(outBuff, strOut, 240);
	sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine) {

	// Output via USB console
	if (onUSB) {
		if (outputLevel + priority > 1) {
			if (newLine) SerialUSB.println(outBuff);
			else SerialUSB.print(outBuff);
		}
	}
	
	strncpy(outBuff, "", 240);
}
void SckBase::prompt() {

	sckOut("SCK > ", PRIO_MED, false);
}

void SckBase::reset() {

	sckOut("Reseting!!!!!");
}


