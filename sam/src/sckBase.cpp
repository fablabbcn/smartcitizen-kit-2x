#include "sckBase.h"

void SckBase::setup() {

	// Serial Port Configuration (ESP communication)
	Serial1.begin(serialBaudrate);

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


	// ESTO SI FUNCIONA:::
	// rtc.setAlarmSeconds(rtc.getSeconds()+4);
	// rtc.enableAlarm(rtc.MATCH_SS);
	// rtc.attachInterrupt(ISR_longPress);
	// if (rtc.isConfigured() && rtc.getYear() >= 17) onTime = true;

}
void SckBase::update() {

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

