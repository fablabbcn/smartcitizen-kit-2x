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

bool published = false;	// TODO remove this and put a proper timer

void SckBase::setup()
{

	// Led
	led.setup();

	// Internal I2C bus setup
	Wire.begin();

	// Auxiliary I2C bus
	pinMode(pinPOWER_AUX_WIRE, OUTPUT);
	digitalWrite(pinPOWER_AUX_WIRE, LOW);	// LOW -> ON , HIGH -> OFF
	pinPeripheral(11, PIO_SERCOM);
	pinPeripheral(13, PIO_SERCOM);
	auxWire.begin();
	delay(2000); 				// Give some time for external boards to boot

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

	// RTC setup
	rtc.begin();
	if (rtc.isConfigured() && (rtc.getEpoch() > 1514764800)) state.onTime = true;	// If greater than 01/01/2018
	else {
		rtc.setTime(0, 0, 0);
		rtc.setDate(1, 1, 15);
	}

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
	if (config.mode == MODE_NET) led.update(led.BLUE, led.PULSE_SOFT);
	else if (config.mode == MODE_SD) led.update(led.PINK, led.PULSE_SOFT);
	
	// Urban board
	analogReadResolution(12);
	urbanPresent = urban.setup();
	if (urbanPresent) {
		sckOut("Urban board detected");
		readLight.setup();
		//readLight.debugFlag = true;
	} else sckOut("No urban board detected!!");


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

	// TEMP 
	if (state.onSetup) {
		lightResults = readLight.read();
		if (lightResults.ok) parseLightRead();
	} else if (state.reading) {
		if (millis() % (config.publishInterval * 1000) == 0 && !published) {
			published = true; // TODO TEMP make a more elegant solution
			if (state.mode == MODE_NET) netPublish();
		} else published = false;
	}

	if (millis() % 100 == 0) reviewState();
	if (millis() % 50 == 0) if (buttonDown) buttonStillDown();	// TODO replace with interrupt timers
}

// **** Mode Control
void SckBase::reviewState() {

	/*
	struct SckState {
		bool onSetup
		bool espON
		bool wifiSet
		bool onWifi
		bool wifiError 
		bool tokenSet
		bool helloPending
		bool onTime
		bool timeAsked;
		bool timeError
		SCKmodes mode
		bool cardPresent
		bool reading
		bool sleeping
	};

	state can be changed by:
		parseLightRead()
		loadConfig()
		receiveMessage()
		setTime()
		sdDetect()
		buttonEvent();
	*/

	if (!(state == oldState)) {

		printState();
		oldState = state;

		if (state.mode == MODE_NOT_CONFIGURED) {
			
			enterSetup();

		} else if (state.onSetup) {
			
			// Start ap mode and webserver on ESP

		
		} else if (state.mode == MODE_NET) {

			state.reading = false;

			// TODO wifiError and timeError retrys/timeouts

			if (!state.wifiSet || !state.tokenSet) led.update(led.BLUE, led.PULSE_HARD_FAST);
			else if (state.helloPending) {
				if (!state.onWifi) {
					if (state.wifiError) led.update(led.BLUE, led.PULSE_HARD_FAST);
					if (!state.espON) ESPcontrol(ESP_ON);
				} else {
					sckOut("Hello sent!"); 
					sendMessage(ESPMES_MQTT_HELLO, ""); 
					state.helloPending = false;
				}
			} else if (!state.onTime) {
				if (!state.onWifi) {
					if (state.wifiError) led.update(led.BLUE, led.PULSE_HARD_FAST);
					if (!state.espON) ESPcontrol(ESP_ON);
				} else {
					if (state.timeError) led.update(led.BLUE, led.PULSE_HARD_FAST);
					if (!state.timeAsked) {
						sckOut("Asking time to ESP...");
						sendMessage(ESPMES_GET_TIME, "");
					}
				}
			} else if (!state.reading) state.reading = true;


		} else if  (state.mode == MODE_SD) {

			state.reading = false;

			/*
			* MODE_SD (!cardPresent || (!onTime && !wifiSet)) -> onSetup
				* MODE_SD (cardPresent && !onTime && wifiSet) -> WAITING_TIME
					* WAITING_TIME (!onWifi) -> WAITING_WIFI
						* WAITING_WIFI (!espON) -> ESP_ON
						* WAITING_WIFI -> (espON && wifiError || timeError) -> onSetup
				* MODE_SD (cardPresent && onTime) -> ESP_OFF && updateSensors
			*/

			/* if (!state.cardPresent || (!state.onTime && !state.wifiSet)) enterSetup(); */
			/* else if (!state.onTime) { */
			/* 	sckOut("sdcard NOT ON TIME!!"); */
			/* 	if (!state.onWifi) { */
			/* 		if (!state.espON) ESPcontrol(ESP_ON); */
			/* 		else if (state.wifiError) enterSetup(); */
			/* 	} else { */				
			/* 		sckOut("Asking time to ESP..."); */ 
			/* 		sendMessage(ESPMES_GET_TIME, ""); */
			/* 		if (state.timeError) enterSetup(); */
			/* 	} */
			/* } else if (!state.sleeping) { */
			/* 	ESPcontrol(ESP_OFF); */
			/* 	state.onSetup = false; */
			/* 	state.reading = true; */
			/* 	led.update(led.PINK, led.PULSE_SOFT); */
			/* } */
		}
	}

	if (state.reading) updateSensors();
}
void SckBase::enterSetup() {

	state.onSetup = true;

	// Update led
	led.update(led.RED, led.PULSE_SOFT);

	// Start wifi APmode
	if (!state.espON) ESPcontrol(ESP_ON);
	// TODO APmode on esp. decide how to manage wifiSet && !tokenSet
}
void SckBase::printState(bool all) {

	if ((oldState.onSetup != state.onSetup) | all) sprintf(outBuff, "%sonSetup: %s\r\n", outBuff, state.onSetup  ? "true" : "false");
	if ((oldState.espON != state.espON) | all) sprintf(outBuff, "%sespON: %s\r\n", outBuff, state.espON  ? "true" : "false");
	if ((oldState.wifiSet != state.wifiSet) | all) sprintf(outBuff, "%swifiSet: %s\r\n", outBuff, state.wifiSet  ? "true" : "false");
	if ((oldState.onWifi != state.onWifi) | all) sprintf(outBuff, "%sonWifi: %s\r\n", outBuff, state.onWifi  ? "true" : "false");
	if ((oldState.wifiError != state.wifiError) | all) sprintf(outBuff, "%swifiError: %s\r\n", outBuff, state.wifiError  ? "true" : "false");
	if ((oldState.tokenSet != state.tokenSet) | all) sprintf(outBuff, "%stokenSet: %s\r\n", outBuff, state.tokenSet  ? "true" : "false");
	if ((oldState.onTime != state.onTime) | all) sprintf(outBuff, "%sonTime: %s\r\n", outBuff, state.onTime  ? "true" : "false");
	if ((oldState.timeError != state.timeError) | all) sprintf(outBuff, "%stimeError: %s\r\n", outBuff, state.timeError  ? "true" : "false");
	if ((oldState.mode != state.mode) | all) sprintf(outBuff, "%smode: %s\r\n", outBuff, modeTitles[state.mode]);
	if ((oldState.cardPresent != state.cardPresent) | all) sprintf(outBuff, "%scardPresent: %s\r\n", outBuff, state.cardPresent  ? "true" : "false");
	if ((oldState.reading != state.reading) | all) sprintf(outBuff, "%sreading: %s\r\n", outBuff, state.reading  ? "true" : "false");
	if ((oldState.sleeping != state.sleeping) | all) sprintf(outBuff, "%ssleeping: %s\r\n", outBuff, state.sleeping  ? "true" : "false");
	sckOut(PRIO_LOW, false);
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

	state.wifiSet = config.credentials.set;
	state.tokenSet = config.token.set;
	state.mode = config.mode;
}
void SckBase::saveConfig(Configuration newConfig) {

	eepromConfig.write(newConfig);
	config = newConfig;

	state.mode = config.mode;
	state.wifiSet = config.credentials.set;
	state.tokenSet = config.token.set;

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["mo"] = (uint8_t)config.mode;
	json["pi"] = config.publishInterval;
	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;

	sprintf(netBuff, "%u", ESPMES_SET_CONFIG);
	json.printTo(&netBuff[1], json.measureLength() + 1);
	if (!state.espON) ESPcontrol(ESP_ON);
	delay(150);
	sckOut("Saved configuration!!", PRIO_LOW);
	if (sendMessage()) sckOut("Saved configuration on ESP!!", PRIO_LOW);
}
Configuration SckBase::getConfig() {

	return config;
}
bool SckBase::parseLightRead(){
	
	Configuration lightConfig;
	
	if (lightResults.lines[0].endsWith(F("wifi")) || lightResults.lines[0].endsWith(F("auth"))) {
		if (lightResults.lines[1].length() > 0) {
			lightResults.lines[1].toCharArray(lightConfig.credentials.ssid, 64);
			lightResults.lines[2].toCharArray(lightConfig.credentials.pass, 64);
			lightConfig.credentials.set = true;
		}
	}

	if (lightResults.lines[0].endsWith(F("auth"))) {
		if (lightResults.lines[3].length() > 0) {
			lightResults.lines[3].toCharArray(lightConfig.token.token, 7);
			lightConfig.token.set = true;
		}
		uint32_t receivedInterval = lightResults.lines[4].toInt();
		if (receivedInterval > minimal_publish_interval && receivedInterval < max_publish_interval) lightConfig.publishInterval = receivedInterval;
	}

	if (lightResults.lines[0].endsWith(F("time"))) setTime(lightResults.lines[1]);

	readLight.reset();
	state.onSetup = false;
	state.mode = MODE_NET;
	state.helloPending = true;
	led.update(led.GREEN, led.PULSE_STATIC);
	saveConfig(lightConfig);
	return true;
}

// **** ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF: {
			sckOut("ESP off...");
			state.espON = false;
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
			state.espON = true;
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
			// sckOut("Command: " + String(wichMessage), PRIO_LOW);

			// Get content from first package (1 byte less than the rest)
			memcpy(netBuff, &netPack[2], NETPACK_CONTENT_SIZE - 1);

			// Het the rest of the packages (if they exist)
			for (uint8_t i=0; i<netPack[TOTAL_PARTS]-1; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	{
					memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE) + (NETPACK_CONTENT_SIZE - 1)], &netPack[1], NETPACK_CONTENT_SIZE);
				}
				else return;
			}
			// sckOut("Content: " + String(netBuff), PRIO_LOW);

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

	if (!state.espON) {
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

		} case SAMMES_WIFI_CONNECTED: sckOut("Conected to wifi!!"); state.onWifi = true; wifiRetrys = 0; state.wifiError = false; break;
		case SAMMES_SSID_ERROR: sckOut("ERROR Access point not found!!"); state.wifiError = true; break;
		case SAMMES_PASS_ERROR: sckOut("ERROR wrong wifi password!!"); state.wifiError = true; break;
		case SAMMES_WIFI_UNKNOWN_ERROR: sckOut("ERROR unknown wifi error!!"); state.wifiError = true; break;
		case SAMMES_TIME : {
			state.timeAsked = false;
			String strTime = String(netBuff);
			setTime(strTime);
			break;
		} case SAMMES_MQTT_HELLO_OK: {
			led.update(led.BLUE, led.PULSE_SOFT);
			sckOut("MQTT hello OK!!");
			break;
		} case SAMMES_MQTT_PUBLISH_OK: sckOut("MQTT publish OK!!"); break;
		default: break;
	}
}

// **** SD card
bool SckBase::sdDetect() {

	// Wait 100 ms to avoid multiple triggered interrupts
	if (millis() - cardLastChange < 100) return state.cardPresent;

	cardLastChange = millis();
	state.cardPresent = !digitalRead(pinCARD_DETECT);

	if (state.cardPresent) return sdSelect();
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
void SckBase::updateSensors() {
	/*
	** updateSensors
		* reading (publishErrors < retrys) -> PUBLISH
				* PUBLISH (!onWifi) -> WAITING_WIFI
					* WAITING_WIFI (!espON) -> ESP_ON
					* WAITING_WIFI -> (espON && wifiError || timeout) -> errors ++
				* PUBLISH (onWifi) -> WAITING_MQTT_RESPONSE
					* WAITING_MQTT_RESPONSE (mqttError || timeout) -> errors ++
	*/
}
bool SckBase::getReading(SensorType wichSensor, bool wait) {

	sensors[wichSensor].valid = false;
	String result = "none";

	switch (sensors[wichSensor].location) {
		case BOARD_BASE: {
			switch (wichSensor) {
				case SENSOR_BATT_PERCENT: {

					uint32_t thisPercent = lipo.soc();
					if (thisPercent > 100) thisPercent = 0;
					result = String(thisPercent); 
					
					break;
				} case SENSOR_BATT_VOLTAGE: result = String(lipo.voltage()); break;
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
			result = String(auxBoards.getReading(wichSensor));	// TODO por auxBoards to String mode
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
bool SckBase::netPublish()  {
	
	sckOut("Publishing to platform...");

	// Prepare json for sending
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	
	// Epoch time of the grouped readings
	json["t"] = rtc.getEpoch();

	JsonArray& jsonSensors = json.createNestedArray("sensors");

	uint8_t count = 0;

	for (uint16_t sensorIndex=0; sensorIndex<SENSOR_COUNT; sensorIndex++) {

		SensorType wichSensor = static_cast<SensorType>(sensorIndex);

		if (sensors[wichSensor].enabled && sensors[wichSensor].id > 0) {
			//SerialUSB.print(wichSensor);
			if (getReading(wichSensor, true)) {
				//json[String(sensorIndex)] = sensors[wichSensor].reading;
				jsonSensors.add(sensors[wichSensor].reading);
				count ++;
			}
		}
	}

	json.printTo(SerialUSB);

	ISOtime();
	sprintf(outBuff, "%s: %i sensor readings on the go to platform...", ISOtimeBuff, count);
	sckOut();

	sprintf(netBuff, "%u", ESPMES_MQTT_PUBLISH);
	json.printTo(&netBuff[1], json.measureLength() + 1);
	return sendMessage();
}

// **** Time
bool SckBase::setTime(String epoch) {

	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		state.onTime = true;
		state.timeError = false;
		ISOtime();
		sprintf(outBuff, "RTC updated: %s", ISOtimeBuff);
		sckOut();
		return true;
	} else {
		state.timeError = true;
		sckOut("RTC update failed!!");
	}
	return false;
}
bool SckBase::ISOtime() {

	if (state.onTime) {
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
