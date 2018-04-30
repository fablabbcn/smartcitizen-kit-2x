#include "SckBase.h"
#include "Commands.h"

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {

	auxWire.onService();
}

// Auxiliary I2C devices
AuxBoards auxBoards;

// Eeprom flash emulation to store persistent variables
FlashStorage(eepromConfig, Configuration);


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
	SerialESP.begin(serialBaudrate);

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Power management configuration
	// battSetup();
	charger.setup();
	
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
	loadConfig();
	
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

}

// **** Input
void SckBase::inputUpdate() {

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

			delayMicroseconds(200);
			SerialUSB.read();				// drop next char (always 91)
			delayMicroseconds(200);
			char tmp = SerialUSB.read();
			if (tmp != 65) tmp = SerialUSB.read(); // detect up arrow
			else {
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


// Output
// **** Output
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
	} else  {
		digitalWrite(pinLED_USB, HIGH);
	}
	
	strncpy(outBuff, "", 240);
}
void SckBase::prompt() {

	sckOut("SCK > ", PRIO_MED, false);
}

// **** Config
void SckBase::loadConfig() {

	sckOut("Loading configuration from eeprom...");

	Configuration savedConf = eepromConfig.read();

	if (savedConf.valid) {
		
		config.publishInterval = savedConf.publishInterval;
		config.workingMode = savedConf.workingMode;
		config.mode = savedConf.mode;
		strncpy(config.ssid, savedConf.ssid, 64);
		strncpy(config.pass, savedConf.pass, 64);
		strncpy(config.token, savedConf.token, 8);

		// Load per sensor config
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			sensors[wichSensor].enabled = savedConf.sensor[wichSensor].enabled;
			sensors[wichSensor].interval = savedConf.sensor[wichSensor].interval;
		}

		// Load sdcard config
		// loadSDconfig();

	} else {

		// If there is no sdcard valid config turn to factory defaults
		// if (!loadSDconfig()) {
			sckOut("Can't find valid configuration!!! loading defaults...");
			saveConfig(true);
			loadConfig();

		// } else {
		// 	saveConfig();
		// }
		return;
	}

	// Check if wifi is already set
	if (String(config.ssid).length() > 0) wifiSet = true;
	else {
		sckOut("Wifi is not configured!!!");
		wifiSet = false;
	}

	// Check if token is already set
	if (String(config.token).length() == 6) tokenSet = true;
	else {
		sckOut("No token configured!!");
		tokenSet = false;
	}

	// changeMode(config.persistentMode);
}
void SckBase::saveConfig(bool factory) {

	Configuration toSaveConfig;

	if (!factory) {
		
		toSaveConfig.valid = true;
		toSaveConfig.mode = config.mode;
		toSaveConfig.workingMode = config.workingMode;
		toSaveConfig.publishInterval = config.publishInterval;
		strncpy(toSaveConfig.ssid, config.ssid, 64);
		strncpy(toSaveConfig.pass, config.pass, 64);
		strncpy(toSaveConfig.token, config.token, 8);
		
		// Save per sensor config
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			toSaveConfig.sensor[wichSensor].enabled = sensors[wichSensor].enabled;
			toSaveConfig.sensor[wichSensor].interval = sensors[wichSensor].interval;
		}

		// Save to eeprom
		eepromConfig.write(toSaveConfig);
		sckOut("Saved configuration to eeprom!!!");

	} else {

		sckOut("Reseting to factory defaults...");

		// Default values
		toSaveConfig.valid = true;
		toSaveConfig.mode = config.mode = MODE_NOT_CONFIGURED;
		toSaveConfig.workingMode = config.workingMode = MODE_NET;
		toSaveConfig.publishInterval = config.publishInterval =  default_publish_interval;
		strncpy(toSaveConfig.ssid, "", 64);
		strncpy(toSaveConfig.pass, "", 64);
		strncpy(toSaveConfig.token, "null", 8);

		// Default sensor values
		AllSensors tempSensors;

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			toSaveConfig.sensor[wichSensor].enabled = sensors[wichSensor].enabled = tempSensors[wichSensor].enabled;
			toSaveConfig.sensor[wichSensor].interval = sensors[wichSensor].interval = default_sensor_reading_interval;
		}

		// Save to eeprom
		eepromConfig.write(toSaveConfig);
		sckOut("Saved configuration to eeprom!!!");
		// saveSDconfig();
	}
}
char* SckBase::getToken() {
	sendMessage(ESPMES_GET_TOKEN, true);
	return config.token;
}
void SckBase::saveToken(char newToken[8]) {

	if (String(newToken).length() == 6) {
		
		sprintf(outBuff, "Saving new token: %s", newToken);
		sckOut();

		strncpy(config.token, newToken, 8);
		tokenSet = true;
		saveConfig();

		sendMessage(ESPMES_SET_TOKEN, config.token, true);
	
	} else sckOut("Token must have 6 characters!!!");
}
void SckBase::saveToken() {

	sckOut(F("Clearing token..."));
	strncpy(config.token, "null", 8);

	tokenSet = false;
	saveConfig();

	sendMessage(ESPMES_SET_TOKEN, "null");
}

// **** ESP
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

			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			delay(500);

			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
			digitalWrite(pinPOWER_ESP, LOW);

			flashingESP = true;

			led.update(led.WHITE, led.PULSE_STATIC);

			uint32_t flashTimeout = millis();
			uint32_t startTimeout = millis();
			while(1) {
				if (SerialUSB.available()) {
					SerialESP.write(SerialUSB.read());
					flashTimeout = millis();	// If something is received restart timer
				}
				if (SerialESP.available()) {
					SerialUSB.write(SerialESP.read());
				} 
				if (millis() - flashTimeout > 1000) {
					if (millis() - startTimeout > 8000) reset();		// Giva an initial 8 seconds for the flashing to start
				}
			}
			break;

		} case ESP_ON: {

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

// **** SD card
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

// **** Flash memory
void SckBase::flashSelect() {

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
}

// **** Power
void SckBase::battSetup() {

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
	// If charge is finished disable it, to avoid multiple interrupts 
	// Maybe when we have batt detection on top this is not necesary!!!
	if (charger.getChargeStatus() == charger.CHRG_CHARGE_TERM_DONE) {
		sckOut("Batterry fully charged... or removed");
		charger.chargeState(0);
		// led.update(led.YELLOW, led.PULSE_STATIC);
	}

	while (charger.getDPMstatus()) {} // Wait for DPM detection finish
	
	if (charger.getPowerGoodStatus()) {
		
		onUSB = true;
		// led.update(led.GREEN, led.PULSE_STATIC);
		// charger.OTG(false);
		
	} else { 
		
		onUSB = false;
		// led.update(led.BLUE, led.PULSE_STATIC);
		// charger.OTG(true);

	}

	// TODO, React to any charger fault
	if (charger.getNewFault() != 0) {
		sckOut("Charger fault!!!");
		// led.update(led.YELLOW, led.PULSE_STATIC);
	}
	
	if (!onUSB) digitalWrite(pinLED_USB, HIGH); 	// Turn off Serial leds
}

// **** Sensors
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
