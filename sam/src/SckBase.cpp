#include "SckBase.h"
#include "Commands.h"

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {

	auxWire.onService();
}

// ESP communication
RH_Serial driver(SerialESP);
RHReliableDatagram manager(driver, SAM_ADDRESS);

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
	SerialESP.begin(serialBaudrate);
	manager.init();

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

	// Configuration
	loadConfig();
	
	// Urban board
	urbanPresent = urban.setup();
	if (urbanPresent) {
		sckOut("Urban board detected");
	}

	// Detect and enable auxiliary boards // TEMP this should be done in aux setup
	// for (uint8_t i=0; i<SENSOR_COUNT; i++) {

	// 	OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];

	// 	// Only try to find auxiliary sensors
	// 	if (wichSensor->location == BOARD_AUX) {

	// 		sprintf(outBuff, "Detecting: %s... ", wichSensor->title);
	// 		sckOut(PRIO_MED, false);

	// 		if (auxBoards.begin(wichSensor->type)) {

	// 			if (!wichSensor->enabled) {
	// 				sckOut("found!!!");
	// 				// enableSensor(wichSensor);
	// 			} else {
	// 				sckOut("found, already enabled!!!");
	// 				sckOut();
	// 			}

	// 		} else {
	// 			if (wichSensor->enabled) {
	// 				sckOut("not found!!!");
	// 				// disableSensor(wichSensor);
	// 			} else sckOut("nothing!");
	// 		}
	// 	}
	// }
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

	ESPbusUpdate();
}

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

	if (savedConf.valid) config = savedConf;
	else {

		// TODO check if there is a valid sdcard config and load it
		sckOut("Can't find valid configuration!!! loading defaults...");
		Configuration  newConfig;
		newConfig.valid = true;
		config = newConfig;
		saveConfig(config);
	}
}
bool SckBase::saveConfig(Configuration newConfig) {

	config = newConfig;
	eepromConfig.write(config);

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["mo"] = (uint8_t)config.mode;
	json["wm"] = (uint8_t)config.workingMode;
	json["pi"] = config.publishInterval;
	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;

	sprintf(netBuff, "%u", ESPMES_SET_CONFIG);
	json.printTo(&netBuff[1], json.measureLength() + 1);
	if (!espON) ESPcontrol(ESP_ON);
	if (sendMessage()) {
		sckOut("Saved configuration!!", PRIO_LOW);
		return true;	
	}
	return false;
}
Configuration SckBase::getConfig() {

	return config;
}

// **** ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF: {
			sckOut("ESP off...");
			espON = false;
			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);
			espStarted = 0;
			break;

		} case ESP_FLASH: {

			sckOut("Putting ESP in flash mode...");

			SerialUSB.begin(ESP_FLASH_SPEED);
			SerialESP.begin(ESP_FLASH_SPEED);
			delay(100);

			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
			delay(100);

			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinPOWER_ESP, LOW);

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
					if (millis() - startTimeout > 5000) reset();		// Giva an initial 5 seconds for the flashing to start
				}
			}
			break;

		} case ESP_ON: {

			sckOut("ESP on...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			digitalWrite(pinPOWER_ESP, LOW);
			espON = true;
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
void SckBase::ESPbusUpdate() {

	if (manager.available()) {

		uint8_t len = NETPACK_TOTAL_SIZE;

		if (manager.recvfromAck(netPack, &len)) {

			// Identify received command
			uint8_t pre = String((char)netPack[1]).toInt();
			SAMMessage wichMessage = static_cast<SAMMessage>(pre);
			sckOut("Command: " + String(wichMessage), PRIO_LOW);

			// Get content from first package (1 byte less than the rest)
			memcpy(netBuff, &netPack[2], NETPACK_CONTENT_SIZE - 1);

			// Het the rest of the packages (if they exist)
			for (uint8_t i=0; i<netPack[TOTAL_PARTS]-1; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	{
					memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE) + (NETPACK_CONTENT_SIZE - 1)], &netPack[1], NETPACK_CONTENT_SIZE);
				}
				else return;
			}
			sckOut("Content: " + String(netBuff), PRIO_LOW);

			// Process message
			receiveMessage(wichMessage);
		}
	}
}
bool SckBase::sendMessage(ESPMessage wichMessage) {

	// This function is used when &netBuff[1] is already filled with the content

	sprintf(netBuff, "%u", wichMessage);
	return sendMessage();
}
bool SckBase::sendMessage(ESPMessage wichMessage, const char *content) {

	sprintf(netBuff, "%u%s", wichMessage, content);
	return sendMessage();
}
bool SckBase::sendMessage() {

	// This function is used when netbuff is already filled with command and content

	if (!espON) {
		sckOut("ESP is off please turn it ON !!!");
		return false;
	}

	uint16_t totalSize = strlen(netBuff);
	uint8_t totalParts = (totalSize + NETPACK_CONTENT_SIZE - 1)  / NETPACK_CONTENT_SIZE;

	for (uint8_t i=0; i<totalParts; i++) {
		netPack[TOTAL_PARTS] = totalParts;
		memcpy(&netPack[1], &netBuff[(i * NETPACK_CONTENT_SIZE)], NETPACK_CONTENT_SIZE);
		if (!manager.sendtoWait(netPack, NETPACK_TOTAL_SIZE, ESP_ADDRESS)) return false;
	}

	return true;
}
void SckBase::receiveMessage(SAMMessage wichMessage) {

	switch(wichMessage) {
		case SAMMES_SET_CONFIG: {

			sckOut("Received new config from ESP");
			sckOut(netBuff);
			break;

		} case SAMMES_DEBUG: {

			SerialUSB.print("ESP --> ");
			SerialUSB.print(netBuff);
			break;

		} case SAMMES_NETINFO: {

			StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(netBuff);
			const char* tip = json["ip"];
			const char* tmac = json["mac"];
			const char* thostname = json["hn"];
			sprintf(outBuff, "\r\nHostname: %s\r\nIP address: %s\r\nMAC address: %s", thostname, tip, tmac);
			sckOut();
			break;

		} case SAMMES_WIFI_CONNECTED: {
			
			onWifi = true;
			sckOut("Conected to wifi!!");
			if (!onTime) sckOut("Asking time to ESP..."); sendMessage(ESPMES_GET_TIME, "");
			break;

		} case SAMMES_SSID_ERROR: sckOut("ERROR Access point not found!!"); break;
		case SAMMES_PASS_ERROR: sckOut("ERROR wrong wifi password!!"); break;
		case SAMMES_WIFI_UNKNOWN_ERROR: sckOut("ERROR unknown wifi error!!"); break;
		case SAMMES_TIME : {

			String strTime = String(netBuff);
			setTime(strTime);
			break;

		} default: break;
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
	attachInterrupt(pinBATTERY_ALARM, ISR_battery, LOW);

	lipo.enterConfig();
	lipo.setCapacity(battCapacity);
	lipo.setGPOUTPolarity(LOW);
	lipo.setGPOUTFunction(SOC_INT);
	lipo.setSOCIDelta(1);
	lipo.exitConfig();
}
void SckBase::batteryEvent(){

	SerialUSB.println("battery event");

	// if (batteryPresent) {

	// 	if (lipo.batPresent()) {
	// 		getReading(SENSOR_BATT_PERCENT);
	// 		sprintf(outBuff, "Battery charge %s%%", sensors[SENSOR_BATT_PERCENT].reading.c_str());
	// 	} else {
	// 		batteryPresent = false;
	// 		sprintf(outBuff, "Battery removed!!");
	// 	}

	// } else {

	// 	if (lipo.batPresent()) {
	// 		SerialUSB.println("1");
	// 		if (lipo.begin()) {
	// 			batteryPresent = true;
	// 			getReading(SENSOR_BATT_PERCENT);
	// 			sprintf(outBuff, "Battery connected!! charge %s%%", sensors[SENSOR_BATT_PERCENT].reading.c_str());
	// 		}
	// 	} else {
	// 		sprintf(outBuff, "Received unknown interrupt from battery gauge");
	// 	}
	// }
	// sckOut();
}
void SckBase::batteryReport() {

	SerialUSB.println(lipo.voltage());

	// sprintf(outBuff, "Charge: %u %%\r\nVoltage: %u V\r\nCharge current: %i mA\r\nCapacity: %u/%u mAh\r\nState of health: %i",
	// 	lipo.soc(),
	// 	lipo.voltage(),
	// 	lipo.current(AVG),
	// 	lipo.capacity(REMAIN),
	// 	lipo.capacity(FULL),
	// 	lipo.soh()
	// );
	// sckOut();
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


// **** Time
bool SckBase::setTime(String epoch) {

	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		onTime = true;
		ISOtime();
		sprintf(outBuff, "RTC updated: %s", ISOtimeBuff);
		sckOut();
		return true;
	} else sckOut("RTC update failed!!");
	return false;
}
bool SckBase::ISOtime() {

	if (onTime) {
		epoch2iso(rtc.getEpoch(), ISOtimeBuff);
		return true;
	} else {
		sprintf(ISOtimeBuff, "0");
		return false;
	}
}
void SckBase::epoch2iso(uint32_t toConvert, char* isoTime) {

	time_t tc = toConvert;
    struct tm* tmp = gmtime(&tc);

    sprintf(isoTime, "20%02d-%02d-%02dT%02d:%02d:%02dZ", 
    	tmp->tm_year - 100, 
    	tmp->tm_mon + 1,
    	tmp->tm_mday,
		tmp->tm_hour,
		tmp->tm_min,
		tmp->tm_sec);
}
