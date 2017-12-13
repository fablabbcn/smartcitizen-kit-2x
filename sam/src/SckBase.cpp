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

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	ESPcontrol(ESP_OFF);

	// This should be removed when ESP SPI flash access is fixed
	// ------------
	sd.cardBegin(pinCS_SDCARD, SPI_HALF_SPEED);
	delay(10);
	SPI.end();
	delay(10);
	// ------------

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

// ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF:
			sckOut("Turning ESP off...");
			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);
			espStarted = 0;
			break;

		case ESP_FLASH:
			sckOut("Putting ESP in flash mode...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);			// LOW for flash mode
			digitalWrite(pinPOWER_ESP, LOW);
			break;

		case ESP_ON:
			sckOut("Turning ESP on...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			digitalWrite(pinPOWER_ESP, LOW);
			espStarted = millis();
			break;

		case ESP_REBOOT:
			sckOut("Restarting ESP...");
			ESPcontrol(ESP_OFF);
			delay(10);
			ESPcontrol(ESP_ON);
			break;
	}
}

void SckBase::reset() {
	sckOut("Bye!!");
	NVIC_SystemReset();
}


