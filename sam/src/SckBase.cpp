#include "SckBase.h"
#include "Commands.h"

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {

	auxWire.onService();
}

// Auxiliary I2C devices
AuxBoards auxBoards;

void SckBase::setup() {

	// Led
	led.setup();

	// Pause for a moment (for uploading firmware in case of problems)
	delay(4000);
	
	// Internal I2C bus setup
	Wire.begin();

	// Auxiliary I2C bus
	pinMode(pinPOWER_AUX_WIRE, OUTPUT);
	digitalWrite(pinPOWER_AUX_WIRE, LOW);	// LOW -> ON , HIGH -> OFF
	pinPeripheral(11, PIO_SERCOM);
	pinPeripheral(13, PIO_SERCOM);
	auxWire.begin();

	// Output
	outputLevel = OUT_VERBOSE;

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Power management configuration
	// batteryEvent();
	pinMode(pinCHARGER_INT, INPUT);
	LowPower.attachInterruptWakeup(pinCHARGER_INT, ISR_charger, CHANGE);
	charger.resetConfig();
	charger.getNewFault();

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	ESPcontrol(ESP_OFF);

	// Peripheral setup
	rtc.begin();

	// SDcard and flash select pins
	pinMode(pinCS_SDCARD, OUTPUT);
	pinMode(pinCS_FLASH, OUTPUT);
	digitalWrite(pinCS_SDCARD, HIGH);
	digitalWrite(pinCS_FLASH, HIGH);
	pinMode(pinCARD_DETECT, INPUT_PULLUP);

	// SD card
	attachInterrupt(pinCARD_DETECT, ISR_sdDetect, CHANGE);
	sdDetect();

	// Flash memory 
	// TODO disable debug messages from library
	// flashSelect();
	// flash.begin();
	// flash.setClock(133000);
	// flash.eraseChip(); // we need to do this on factory reset? and at least once on new kits.

	// *****************REMOVE
	// uint8_t b1, b2;
	// uint32_t JEDEC = flash.getJEDECID();
	// uint32_t maxPage = flash.getMaxPage();
	// uint32_t capacity = flash.getCapacity();
	// b1 = (JEDEC >> 16);
	// b2 = (JEDEC >> 8);
	// sprintf(outBuff, "Manufacturer ID: %02xh\r\nMemory Type: %02xh\r\nCapacity: %lu bytes\r\nMaximum pages: %lu", b1, b2, capacity, maxPage);
	// sckOut();

	// byte d = 35;
	// uint32_t addr = random(0, 0xFFFFF);
	// SerialUSB.println(flash.writeByte(addr, d));
	// uint8_t _data = flash.readByte(addr);
	// SerialUSB.println(_data, DEC);
	// if (_data == d) sckOut("Flash write byte test passed!!!");
	// else sckOut("Flash write byte test failed!!!");

	// if (flash.powerDown()) sckOut("Succesfully power down flash memory!!!");
	// if (flash.powerUp()) sckOut("Succesfully waked up flash memory!!!");
	// **********************

	// Configuration
	
	// Urban board
	urbanPresent = urban.setup();
	if (urbanPresent) {
		sckOut("Urban board detected");
	}

	// Detect and enable auxiliary boards // TEMP this should be done in aux setup
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];

		// Only try to find auxiliary sensors
		if (wichSensor->location == BOARD_AUX) {

			sprintf(outBuff, "Detecting: %s... ", wichSensor->title);
			sckOut(PRIO_MED, false);

			if (auxBoards.begin(wichSensor->type)) {

				if (!wichSensor->enabled) {
					sckOut("found!!!");
					// enableSensor(wichSensor);
				} else {
					sckOut("found, already enabled!!!");
					sckOut();
				}

			} else {
				if (wichSensor->enabled) {
					sckOut("not found!!!");
					// disableSensor(wichSensor);
				} else sckOut("nothing!");
			}
		}
	}
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

// Configuration
// void SckBase::saveSDconfig() {

// 	sckOut("Trying to save configuration to SDcard...");

// 	if (cardPresent) {
// 		// Remove old sdcard config
// 		if (sd.exists(configFileName)) sd.remove(configFileName);

// 		// Save to sdcard
// 		if (openConfigFile()) {
// 			char lineBuff[128];

// 			configFile.println("# -------------------\r\n# General configuration\r\n# -------------------");
// 			configFile.println("\r\n# mode:sdcard or network");
// 			// sprintf(lineBuff, "mode:%s", modeTitles[config.workingMode]);
// 			configFile.println(lineBuff);
// 			configFile.println("\r\n# publishInterval:period in seconds");
// 			sprintf(lineBuff, "publishInterval:%lu", config.publishInterval);
// 			configFile.println(lineBuff);

// 			configFile.println("\r\n# -------------------\r\n# Network configuration\r\n# -------------------");
// 			sprintf(lineBuff, "ssid:%s", config.ssid);
// 			configFile.println(lineBuff);
// 			sprintf(lineBuff, "pass:%s", config.pass);
// 			configFile.println(lineBuff);
// 			sprintf(lineBuff, "token:%s", config.token);
// 			configFile.println(lineBuff);

// 			configFile.println("\r\n# -------------------\r\n# Sensor configuration\r\n# ej. sensor name:reading interval or disabled\r\n# -------------------\r\n");

// 			bool externalTitlePrinted = false;

// 			// Sensors config
// 			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
// 				SensorType wichSensor = static_cast<SensorType>(i);

// 				if (!externalTitlePrinted && sensors[wichSensor].location == BOARD_AUX) {
// 					configFile.println("\r\n# External sensors (Not included in SCK board)"); 
// 					externalTitlePrinted = true;
// 				}

// 				if (sensors[wichSensor].enabled) sprintf(lineBuff, "%s:%lu", sensors[wichSensor].title, sensors[wichSensor].interval);
// 				else sprintf(lineBuff, "%s:disabled", sensors[wichSensor].title);

// 				configFile.println(lineBuff);
// 			}

// 			configFile.close();
// 			sckOut("Saved configuration to sdcard!!!");
// 		}
// 	}
// }

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

			SerialUSB.begin(ESP_FLASH_SPEED);
			SerialESP.begin(ESP_FLASH_SPEED);

			ESPcontrol(ESP_OFF);

			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
			digitalWrite(pinPOWER_ESP, LOW);

			flashingESP = true;

			led.update(led.WHITE, led.PULSE_STATIC);

			uint32_t flashTimeout = millis();
			uint32_t startTimeout = millis();
			while(1) {
				if (SerialUSB.available()) {
					//SerialFlashESP.write(SerialUSB.read());
					SerialESP.write(SerialUSB.read());
					flashTimeout = millis();	// If something is received restart timer
				}
				if (SerialESP.available()) {
					//SerialUSB.write(SerialFlashESP.read());
					SerialUSB.write(SerialESP.read());
				} 
				if (millis() - flashTimeout > 1000) {
					if (millis() - startTimeout > 8000) reset();		// Giva an initial 8 seconds for the flashing to start
				}
			}
			break;

		} case ESP_ON: {

			SerialUSB.begin(serialBaudrate);
			SerialESP.begin(serialBaudrate);

			sckOut("Turning ESP on...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			digitalWrite(pinPOWER_ESP, LOW);
			espStarted = rtc.getEpoch();

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
bool SckBase::sdDetect() {

	// Wait 100 ms to avoid multiple triggered interrupts
	if (millis() - cardLastChange < 100) return cardPresent;

	cardLastChange = millis();
	cardPresent = !digitalRead(pinCARD_DETECT);

	if (cardPresent) return sdSelect();
	else sckOut("No Sdcard found!!");
	return false;
}
bool SckBase::sdSelect() {

	if (!cardPresent) return false;

	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash
	digitalWrite(pinCS_SDCARD, LOW);
	
	if (sd.begin(pinCS_SDCARD)) {
		sckOut(F("Sdcard ready!!"), PRIO_LOW);
		return true;
	} else {
		sckOut(F("Sdcard ERROR!!"));
		return false;
	}
}
bool SckBase::sdOpenFile(SckFile wichFile, uint8_t oflag) {

	if (sdSelect()) {
		if (oflag == O_CREAT) sd.remove(wichFile.name);	// Delete the file if we need a new one.
		wichFile.file = sd.open(wichFile.name, oflag);
		return true;
	}
	return false;
}

// Flash memory
void SckBase::flashSelect() {

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
}

// Power
bool SckBase::battSetup() {

	pinMode(pinBATTERY_ALARM, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBATTERY_ALARM, ISR_battery, CHANGE);
	
	if (lipo.begin()) {
		
		lipo.enterConfig();
		lipo.setCapacity(battCapacity);
		lipo.setGPOUTPolarity(LOW);
		lipo.setGPOUTFunction(SOC_INT);
		lipo.setSOCIDelta(1);
		lipo.exitConfig();

		// Force an update
		return true;
	}
	return false;
}
void SckBase::batteryEvent(){

	getReading(SENSOR_BATT_PERCENT);

	if (sensors[SENSOR_BATT_PERCENT].reading.toInt() != 0) {
		if (!batteryPresent) {
			battSetup();
			batteryPresent = true;
		}
		sprintf(outBuff, "Battery charge %s%%", sensors[SENSOR_BATT_PERCENT].reading.c_str());
	} else {

		// TODO
		// To confirm no battery is present we should check the state of charger here
		batteryPresent = false;
		sprintf(outBuff, "No battery present!!");
	}
	sckOut();
}
void SckBase::batteryReport() {

	sprintf(outBuff, "Charge: %u %%\r\nVoltage: %u V\r\nCharge current: %i mA\r\nCapacity: %u/%u mAh\r\nState of health: %i",
		lipo.soc(),
		lipo.voltage(),
		lipo.current(AVG),
		lipo.capacity(REMAIN),
		lipo.capacity(FULL),
		lipo.soh()
	);
	sckOut();
}
void SckBase::reset() {
	sckOut("Bye!!");
	NVIC_SystemReset();
}
void SckBase::chargerEvent() {
	sckOut("charger event!!!");
	led.update(led.YELLOW, led.PULSE_STATIC);
}
// Sensors
bool SckBase::getReading(SensorType wichSensor, bool wait) {

	sensors[wichSensor].valid = false;
	String result = "none";

	switch (sensors[wichSensor].location) {
		case BOARD_BASE: {
			switch (wichSensor) {
				case SENSOR_BATT_PERCENT: result = String(lipo.soc()); break;
				case SENSOR_BATT_VOLTAGE: result = String(lipo.voltage()); break;
				case SENSOR_BATT_CHARGE_RATE: result = String(lipo.current(AVG)); break;
				case SENSOR_VOLTIN: {

					break;
				} default: break;
			}
			break;
		} case BOARD_URBAN: {
			result = urban.getReading(wichSensor, wait);
			if (result.startsWith("none")) return false;
			break;
		}
		case BOARD_AUX: {
			SerialUSB.println(auxBoards.getReading(wichSensor));
			break;
		}
	}

	sensors[wichSensor].reading = result;
	sensors[wichSensor].lastReadingTime = rtc.getEpoch();
	sensors[wichSensor].valid = true;
	return true;;
}


void SckBase::getUniqueID() {

	volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
	uniqueID[0] = *ptr1;

	volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
	uniqueID[1] = *ptr;
	ptr++;
	uniqueID[2] = *ptr;
	ptr++;
	uniqueID[3] = *ptr;
}
