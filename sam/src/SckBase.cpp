#include "SckBase.h"
#include "Commands.h"

// Software Serial ESP 
SoftwareSerial SerialESP(16, 15); // RX, TX

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {
	auxWire.onService();
}


void SckBase::setup() {

	led.setup();

	// Serial Port Configuration (ESP communication)
	SerialESP.begin(serialBaudrate);

	// Internal I2C bus setup
	Wire.begin();

	// Auxiliary I2C bus
	auxWire.begin();

	// Output
	outputLevel = OUT_VERBOSE;

	// Pause for a moment (for uploading firmware in case of problems)
	delay(2000);

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);
	
	// Power management configuration

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	digitalWrite(pinPOWER_ESP, HIGH);
	ESPcontrol(ESP_OFF);

	// Peripheral setup
	rtc.begin();

	// SD card
	pinMode(pinCS_SDCARD, OUTPUT);
	pinMode(pinCARD_DETECT, INPUT_PULLUP);
	attachInterrupt(pinCARD_DETECT, ISR_cardDetect, CHANGE);
	cardDetect();

	// Flash memory
	pinMode(pinCS_FLASH, OUTPUT);
	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash


	pinMode(31, OUTPUT);
	digitalWrite(31, HIGH);

	// Configuration
	
	// Urban board
	urbanPresent = urban.setup();
	if (urbanPresent) sckOut("Urban board detected");

	// TEMP
	// config.mode = MODE_NET;
}
void SckBase::update() {

	// Check Serial port input
	inputUpdate();

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

				serialBuff.replace("\n", "");		// Clean input
				serialBuff.replace("\r", "");
				serialBuff.trim();

				commands.in(this, serialBuff);		// Process input
				if (blen > 0) previousCommand = serialBuff;
				serialBuff = "";
				prompt();

			// Backspace
			} else if (buff == 127) {

				if (blen > 0) SerialUSB.print("\b \b");
				serialBuff.remove(blen-1);

			// Up arrow (previous command)
			} else if (buff == 27) {

				SerialUSB.read();				// drop next char (always 91)
				if (SerialUSB.read() == 65) {	// detect up arrow
					for (uint8_t i=0; i<blen; i++) SerialUSB.print("\b \b");	// clean previous command
					SerialUSB.print(previousCommand);
					serialBuff = previousCommand;
				}

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
		case ESP_OFF: {
			sckOut("Turning ESP off...");
			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);
			espStarted = 0;
			break;

		} case ESP_FLASH: {
			sckOut("Putting ESP in flash mode...");

			led.update(led.WHITE, led.PULSE_STATIC);

			ESPcontrol(ESP_OFF);

			SerialUSB.begin(ESP_FLASH_SPEED);
			SerialESP.begin(ESP_FLASH_SPEED);

			delay(500);


			// digitalWrite(pinESP_CH_PD, HIGH);
			// digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			// digitalWrite(pinPOWER_ESP, LOW);

			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
			digitalWrite(pinPOWER_ESP, LOW);

			flashingESP = true;

			uint32_t flashTimeout = millis();
			uint32_t startTimeout = millis();	// 5 seconds after start the timeout starts running.
			while(1) {
				if (SerialUSB.available()) {
					SerialESP.write(SerialUSB.read());
					// flashTimeout = millis();
				} else if (SerialESP.available()) {
					SerialUSB.write(SerialESP.read());
					// flashTimeout = millis();
				} //else {
					// if (millis() - flashTimeout > 1000) {
					// 	if (millis() - startTimeout > 5000) reset();
					// }
				//}
			}
			break;

		} case ESP_ON: {

			SerialUSB.begin(ESP_FLASH_SPEED);
			SerialESP.begin(ESP_FLASH_SPEED);

			delay(500);


			sckOut("Turning ESP on...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			digitalWrite(pinPOWER_ESP, LOW);
			espStarted = millis();

			delay(500);

			// temporal bridge mode on start esp
			while(1) {
				if (SerialUSB.available()) SerialESP.write(SerialUSB.read());
				else if (SerialESP.available()) SerialUSB.write(SerialESP.read());
			}


			break;

		} case ESP_REBOOT: {
			sckOut("Restarting ESP...");
			ESPcontrol(ESP_OFF);
			delay(10);
			ESPcontrol(ESP_ON);
			break;
		}
	}
}

// SD card
bool SckBase::cardDetect() {

	cardPresent = !digitalRead(pinCARD_DETECT);

	if (cardPresent) {
		sckOut("Sdcard inserted!!");
		sdBegin();
		return true;
	} else {
		sckOut("Sdcard removed!!");
		digitalWrite(pinCS_SDCARD, HIGH);
		return false;
	}
}
bool SckBase::sdBegin() {

	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash

	if (sd.cardBegin(pinCS_SDCARD)) {
		sckOut(F("Sdcard ready!!"), PRIO_LOW);
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		return false;
	}
}
bool SckBase::openConfigFile(bool onlyRead) {

	if (cardPresent) {

		sd.begin(pinCS_SDCARD);

		if (onlyRead) {
			// Open file only for reading
			configFile = sd.open(configFileName, FILE_READ);
		} else {
			// Open file
			configFile = sd.open(configFileName, FILE_WRITE);
		}

		if (configFile) {
			sprintf(outBuff, "Using %s for configuration.", configFileName);
			sckOut(PRIO_LOW);
			return true;
		}
	}
	return false;
}

void SckBase::reset() {
	sckOut("Bye!!");
	NVIC_SystemReset();
}


// Urban board