#include "sckBase.h"

// Hardware Timers
bool timer5Up = false;
void configureTimer5(uint16_t periodMS) {

	if (!timer5Up) {

		// clock the timer with the core cpu clock (48MHz)
		GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
		while(GCLK->STATUS.bit.SYNCBUSY);

		// Reset the TC
		TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		while(TC5->COUNT16.CTRLA.bit.SWRST);

		// Set Timer counter Mode to 16 bits
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

		// Set TC5 mode as match frequency
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE;

		TC5->COUNT16.CC[0].reg = uint16_t(46.875 * periodMS);

		// TC5->COUNT16.CC[0].reg = (uint16_t) 187; //0.5us = 240 clock cycle at 48MHz (core clock)
													// 4ms = 187 clock cycle at 46875
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		// Configure interrupt request
		NVIC_DisableIRQ(TC5_IRQn);
		NVIC_ClearPendingIRQ(TC5_IRQn);
		NVIC_SetPriority(TC5_IRQn, 2); //you can change priority between 0 (Highest priority) and 2 (lowest)
		NVIC_EnableIRQ(TC5_IRQn);

		// Enable the TC5 interrupt request
		TC5->COUNT16.INTENSET.bit.MC0 = 1;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		//enable the counter (from now your getting interrupt)
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);

		timer5Up = true;
	}
};
void disableTimer5() {

	if(timer5Up) {
		//use this to disable the counter :
		TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		timer5Up = false;
	}
};


// Eeprom flash emulation to store persistent variables
FlashStorage(eepromConfig, Configuration);

// SAM <<>> ESP communication
EasyTransfer BUS_in, BUS_out;

// Urban board
SckUrban urban;

// Auxiliary I2C devices
AuxBoards auxBoards;

// Sdcard
SdFat sd;
File publishFile;
File logFile;
File configFile;
File debugLogFile;

void SckBase::setup() {

	// Serial Port Configuration 
	Serial1.begin(baudrate);

	// SAM <<>> ESP comunication
	BUS_in.begin(details(msgIn), &Serial1);
	BUS_out.begin(details(msgOut), &Serial1);
	
	// ESP Configuration
	pinMode(POWER_WIFI, OUTPUT);
	pinMode(CH_PD, OUTPUT);
	pinMode(GPIO0, OUTPUT);
	digitalWrite(POWER_WIFI, HIGH);
	ESPcontrol(ESP_OFF);
	timerClear(ACTION_CLEAR_ESP_BOOTING);

	// I2C Configuration
	Wire.begin();				// Init wire library

	// Sensor Board Conector
	pinMode(urban.SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN, OUTPUT);	// PA7 -- CO Sensor Heather
	pinMode(urban.SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN, OUTPUT);	// PA6 -- NO2 Sensor Heater
	pinMode(S0, INPUT);		// PA4 -- CO Sensor
	pinMode(S1, INPUT);		// PA5 -- NO2 Sensor
	pinMode(S2, INPUT);		// PB8 -- CO Current Sensor
	pinMode(S3, INPUT);		// PB9 -- NO2 Current Sensor
	pinMode(S4, INPUT);		// PB2 -- Sound Sensor

	// SD card
	pinMode(SS, OUTPUT);

	// Power management configuration
  	pinMode(PS, OUTPUT);
  	digitalWrite(PS, HIGH);
  	writeCurrent(200);
	
 	// Button
 	pinMode(PIN_BUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(PIN_BUTTON, ISR_button, CHANGE);
	LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, ISR_alarm, CHANGE);

	// Pause for a moment (for uploading firmware in case of problems)
	delay(2000);

 	// Peripheral setup
 	rtc.begin();
 	if (rtc.isConfigured() && rtc.getYear() >= 17) onTime = true;
	led.setup();
	sdPresent();
	urbanPresent = urbanBoardDetected();
	if (urbanPresent) {
		readLightEnabled = true;
		readLight.setup();
		urban.setup();
	}

	// Output level
	outputLevel = OUT_NORMAL;

	analogReadResolution(12);				// Set Analog resolution to MAX
	analogWriteResolution(8);

	// Configuration
	loadConfig();

	// Detect and enable auxiliary boards
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		
		SensorType wichSensor = static_cast<SensorType>(i);

		// Only try to find auxiliary sensors
		if (sensors[wichSensor].location == BOARD_AUX) {

			sprintf(outBuff, "Detecting: %s... ", sensors[wichSensor].title);
			sckOut(PRIO_MED, false);
			
			if (auxBoards.begin(wichSensor)) {

				// Exception for OLED screen (smallest interval)
				if (wichSensor == SENSOR_GROOVE_OLED) sensors[wichSensor].interval = 2;

				if (!sensors[wichSensor].enabled) {
					sckOut("found!!!");
					enableSensor(wichSensor);
				} else {
					sckOut("found, already enabled!!!");
					sckOut();
				} 

			} else {
				if (sensors[wichSensor].enabled) {
					sckOut("not found!!!");
					disableSensor(wichSensor);
				} else sckOut("nothing!");
			}
		}
	}

	// Dont go to sleep until some time has passed
	userLastAction = rtc.getEpoch();

	// Check if USB connected and charging status
	updatePower();
}
void SckBase::update() {

	// Flash and bridge modes
	if (config.mode == MODE_FLASH || config.mode == MODE_BRIDGE){

		// IT seems that this sometimes causes errors in data transmission to ESP
		// if (SerialUSB.available()) {
		// 	char buff = SerialUSB.read();
		// 	serialBuff += buff;
		// 	if (serialBuff.length() > 4) serialBuff.remove(0);
		// 	if (serialBuff.startsWith("Bye")) softReset();
		// 	Serial1.write(buff);
		// }
		if (SerialUSB.available()) Serial1.write(SerialUSB.read());
		if (Serial1.available()) SerialUSB.write(Serial1.read());
	} else if (config.mode != MODE_SHELL) {

		// update ESP communications
		if (!digitalRead(POWER_WIFI)) ESPbusUpdate();

		// Update timers
		timerRun();

		// Check Serial ports inputs
		inputUpdate();

		// Power Management
		updatePower();

		//----------------------------------------
		// 	MODE_SETUP
		//----------------------------------------
		if (config.mode == MODE_SETUP) {
			if (readLightEnabled) {
				lightResults = readLight.read();
				if (lightResults.ok) {

					if (lightResults.lines[0].endsWith(F("wifi")) || lightResults.lines[0].endsWith(F("auth"))) {
						if (lightResults.lines[1].length() > 0) {
							char newSsid[64];
							char newPass[64];
							lightResults.lines[1].toCharArray(newSsid, 64);
							lightResults.lines[2].toCharArray(newPass, 64);
							saveWifi(newSsid, newPass);
							setESPwifi();
						}
					}
					if (lightResults.lines[0].endsWith(F("auth"))) {
						if (lightResults.lines[3].length() > 0) {
							char newToken[8];
							lightResults.lines[3].toCharArray(newToken, 8);
							saveToken(newToken);
							setESPtoken();
						}
						if (lightResults.lines[4].toInt() > 0 && lightResults.lines[4].toInt() < ONE_DAY_IN_SECONDS) {
							uint32_t receivedInterval = lightResults.lines[4].toInt();
							if (receivedInterval > minimal_publish_interval && receivedInterval < max_publish_interval) {
								config.publishInterval = receivedInterval;
								sckOut(String F("New reading interval: ") + String(config.publishInterval));	
							}
							
						}
					}
					if (lightResults.lines[0].endsWith(F("time"))) setTime(lightResults.lines[1]);

					led.configOK();
					triggerHello = true;
				 	readLight.reset();
				 	readLightEnabled = false;
					if (digitalRead(POWER_WIFI)) ESPcontrol(ESP_ON);
					saveConfig();
				}
			} else if (readSoundEnabled) {
				readSound.read();
				sckOut(String(readSound.out));
			}
		}
	}
}

//	 ---------------------
// 	 |	Configuration 	 |
//	 ---------------------
//
void SckBase::saveWifi(char newSsid[64], char newPass[64]) {
	sprintf(outBuff, "Saving new credentials: \"%s\" - \"%s\"", newSsid, newPass);
	sckOut();

	strncpy(config.ssid, newSsid, 64);
	strncpy(config.pass, newPass, 64);
	wifiSet = true;
	saveConfig();
}
void SckBase::clearWifi() {
	
	sckOut("Clearing networks...");
	strncpy(config.ssid, "", 64);
	strncpy(config.pass, "", 64);

	wifiSet = false;
	saveConfig();

	msgBuff.com = ESP_CLEAR_WIFI_COM;
	ESPqueueMsg(false, true);
}
void SckBase::setESPwifi() {

	sckOut("Sending wifi to ESP!!!");
	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["ssid"] = config.ssid;
	jsonNet["pass"] = config.pass;
	jsonNet.printTo(msgBuff.param, 240);
	msgBuff.com = ESP_SET_WIFI_COM;
	sckOut(String F("Sending wifi settings to ESP: ") + String(msgBuff.param), PRIO_LOW);
	ESPqueueMsg(true, true);
}
void SckBase::getESPwifi() {
	msgBuff.com = ESP_GET_WIFI_COM;
	ESPqueueMsg(false, true);
}
void SckBase::saveToken(char newToken[8]) {

	if (String(newToken).length() == 6) {
		
		sprintf(outBuff, "Saving new token: %s", newToken);
		sckOut();

		strncpy(config.token, newToken, 8);
		tokenSet = true;
		saveConfig();
	
	} else sckOut("Token must have 6 characters!!!");
}
void SckBase::clearToken() {

	sckOut(F("Clearing token..."));
	strncpy(config.token, "null", 8);

	tokenSet = false;
	saveConfig();

	msgBuff.com = ESP_CLEAR_TOKEN_COM;
	ESPqueueMsg(false, true);
}
void SckBase::setESPtoken() {
	sckOut("Sending token to ESP...", PRIO_LOW);
	strncpy(msgBuff.param, config.token, 8);
	msgBuff.com = ESP_SET_TOKEN_COM;
	ESPqueueMsg(true, true);
}
void SckBase::getESPtoken() {
	sckOut("Asking token to ESP...");
	msgBuff.com = ESP_GET_TOKEN_COM;
	ESPqueueMsg(false, true);
}
void SckBase::loadConfig() {

	sckOut("Loading configuration from eeprom...");

	Configuration savedConf = eepromConfig.read();

	if (savedConf.valid) {
		
		config.publishInterval = savedConf.publishInterval;
		config.persistentMode = savedConf.persistentMode;
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

		sckOut("Updated configuration from eeprom!!!");

		// Load sdcard config
		loadSDconfig();

	} else {

		// If there is no sdcard valid config turn to factory defaults
		if (!loadSDconfig()) {
			sckOut("Can't find valid configuration!!! loading defaults...");
			saveConfig(true);
			loadConfig();
		} else {
			saveConfig();
		}
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

	changeMode(config.persistentMode);
}
bool SckBase::loadSDconfig() {

	sckOut("Loading configuration file from sdcard!");

	// Open file only for reading
	if (openConfigFile(true)) {

		bool manualConfigDetected = false;
		
		// Read all file
		while (configFile.available()) {

			String lineBuff;

			// Read one line
			while (true) {
				char buff = configFile.read();
				if (buff == 13 || buff == 10) break;	// New line detected
				lineBuff += buff;
			}

			lineBuff.replace("\n", "");
			lineBuff.replace("\r", "");
			lineBuff.trim();
			

			// Ignore comments on text file
			if (!lineBuff.startsWith("#") && lineBuff.length() > 0) {

				// Get mode configuration
				if (lineBuff.startsWith("mode:")) {
					lineBuff.replace("mode:", "");
					for (uint8_t i=0; i<MODE_COUNT; i++) {
						SCKmodes wichMode = static_cast<SCKmodes>(i);
						if (lineBuff.equals(modeTitles[wichMode])) {
							if ((wichMode == MODE_SD || wichMode == MODE_NET) && wichMode != config.persistentMode) {
								sprintf(outBuff, "config.txt mode: %s", modeTitles[wichMode]);
								sckOut();
								manualConfigDetected = true;
								config.persistentMode = wichMode;
							}
						}
					}

				// Get publish interval
				} else if (lineBuff.startsWith("publishInterval:")) {
					lineBuff.replace("publishInterval:", "");
					uint32_t newPeriod = lineBuff.toInt();
					if (newPeriod > minimal_publish_interval && newPeriod < max_publish_interval) {
						if (newPeriod != config.publishInterval) {
							sprintf(outBuff, "config.txt publish interval: %lu", newPeriod);
							sckOut();
							manualConfigDetected = true;
							config.publishInterval = newPeriod;	
						}
					}

				// Get wifi ssid
				} else if (lineBuff.startsWith("ssid:")) {
					lineBuff.replace("ssid:", "");
					if (lineBuff.length() > 0) {
						lineBuff.toCharArray(config.ssid, 64);
						wifiSet = true;
					}

				// Get wifi pass
				} else if (lineBuff.startsWith("pass:")) {
					lineBuff.replace("pass:", "");
					if (lineBuff.length() > 0) {
						lineBuff.toCharArray(config.pass, 64);
					}

				// Get token
				} else if (lineBuff.startsWith("token:")) {
					lineBuff.replace("token:", "");
					if (lineBuff.length() == 6) {
						lineBuff.toCharArray(config.token, 8);
						tokenSet = true;
					}

				// Get sensor settings
				} else {

					// Get sensor title
					String sensorString = lineBuff.substring(0, lineBuff.indexOf(":"));
					lineBuff.replace(sensorString, "");
					lineBuff.replace(":", "");
					lineBuff.trim();

					// Prepare string for matching
					sensorString.toLowerCase();

					// Find out wich sensor is
					SensorType wichSensor = getSensorFromString(sensorString);
					
					if (wichSensor < SENSOR_COUNT) {
						
						uint32_t newInterval = lineBuff.toInt();

						// Sensor disabled
						if (newInterval == 0) {
							if (sensors[wichSensor].enabled) {
								sprintf(outBuff, "config.txt %s: disabled", sensors[wichSensor].title);
								sckOut();
								manualConfigDetected = true;
								sensors[wichSensor].enabled = false;
							}

						// New reading interval
						} else if (newInterval >= minimal_sensor_reading_interval && newInterval <= max_sensor_reading_interval) {
							
							// Enable sensor if it was disabled
							if (!sensors[wichSensor].enabled) {
								sprintf(outBuff, "config.txt: %s enabled", sensors[wichSensor].title);
								sckOut();
								manualConfigDetected = true;
								sensors[wichSensor].enabled = true;
							}
							
							// Change interval if it is different
							if (sensors[wichSensor].interval != newInterval) {
								sprintf(outBuff, "config.txt %s reading interval: %lu", sensors[wichSensor].title, newInterval);
								sckOut();
								manualConfigDetected = true;
								sensors[wichSensor].interval = newInterval;
							}

						} else {
							sprintf(outBuff, "Wrong value for %s configuration!!", sensors[wichSensor].title);
							sckOut();
						}
					}
				}
			}
		}

		configFile.close();

		// If something has been configured manually
		if (manualConfigDetected) saveConfig();

	} else {
		sckOut("Error loading config.txt!!!");
		saveSDconfig();
		return false;
	}

	sckOut("Updated configuration from sdcard!!!");

	return true;
}
void SckBase::saveConfig(bool factory) {

	Configuration toSaveConfig;

	if (!factory) {
		
		toSaveConfig.valid = true;
		toSaveConfig.mode = config.mode;
		toSaveConfig.persistentMode = config.persistentMode;
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

		// Strange thigs we have to do to keep ESP alive and use sdcard config.txt
		// A timer that will check for the right oportunity to save config to sdcard without disrupting too much
		if (!timerExists(ACTION_SAVE_SD_CONFIG)) timerSet(ACTION_SAVE_SD_CONFIG, 400, true);

	} else {

		sckOut("Reseting to factory defaults...");

		// Default values
		toSaveConfig.valid = true;
		toSaveConfig.mode = config.mode = toSaveConfig.persistentMode = config.persistentMode = default_mode;
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
		saveSDconfig();
	}
}
void SckBase::saveSDconfig() {

	sckOut("Trying to save configuration to SDcard...");

	if (sdPresent()) {

		// Remove old sdcard config
		if (sd.exists(configFileName)) sd.remove(configFileName);

		// Save to sdcard
		if (openConfigFile()) {

			char lineBuff[128];

			configFile.println("# -------------------\r\n# General configuration\r\n# -------------------");
			configFile.println("\r\n# mode:sdcard or network");
			sprintf(lineBuff, "mode:%s", modeTitles[config.persistentMode]);
			configFile.println(lineBuff);
			configFile.println("\r\n# publishInterval:period in seconds");
			sprintf(lineBuff, "publishInterval:%lu", config.publishInterval);
			configFile.println(lineBuff);

			configFile.println("\r\n# -------------------\r\n# Network configuration\r\n# -------------------");
			sprintf(lineBuff, "ssid:%s", config.ssid);
			configFile.println(lineBuff);
			sprintf(lineBuff, "pass:%s", config.pass);
			configFile.println(lineBuff);
			sprintf(lineBuff, "token:%s", config.token);
			configFile.println(lineBuff);

			configFile.println("\r\n# -------------------\r\n# Sensor configuration\r\n# ej. sensor name:reading interval or disabled\r\n# -------------------\r\n");

			bool externalTitlePrinted = false;

			// Sensors config
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = static_cast<SensorType>(i);

				if (!externalTitlePrinted && sensors[wichSensor].location == BOARD_AUX) {
					configFile.println("\r\n# External sensors (Not included in SCK board)"); 
					externalTitlePrinted = true;
				}

				if (sensors[wichSensor].enabled) sprintf(lineBuff, "%s:%lu", sensors[wichSensor].title, sensors[wichSensor].interval);
				else sprintf(lineBuff, "%s:disabled", sensors[wichSensor].title);

				configFile.println(lineBuff);
			}

			configFile.close();
			sckOut("Saved configuration to sdcard!!!");
		}
	}
	if (config.mode == MODE_SETUP) ESPcontrol(ESP_ON);
}
void SckBase::mqttConfig(bool activate) {

	// Subscribe or unsubscribe to mqtt config topic
	if (activate) {
		msgBuff.com = ESP_MQTT_SUBSCRIBE_COM;
		sprintf(outBuff, "Subscribing ");
	} else {
		msgBuff.com = ESP_MQTT_UNSUBSCRIBE_COM;
		sprintf(outBuff, "Unsubscribing ");
	}

	sprintf(outBuff, "%sto MQTT config topic", outBuff);
	sckOut();

	ESPqueueMsg(true, true);
}
void SckBase::changeMode(SCKmodes newMode) {

	// Start with a new clear state
	// --------------------------------------
	closestAction = 0;

	// Reset SDcard detection flag
	sdcardAlreadySearched = false;

	// Clear timers
	timerClearTasks();

	// Stop searching for light signals (only do it on setup mode)
	readLightEnabled = false;

	// Configure things depending on new mode
	// Restore previous output level
	if (config.mode == MODE_BRIDGE) changeOutputLevel(prevOutputLevel);

	// Actions for each mode
	switch(newMode) {
		case MODE_SETUP: {

			sckOut("Entering Setup mode!");

			publishRuning = false;

			// Start ESP
			if (digitalRead(POWER_WIFI)) timerSet(ACTION_ESP_ON, 200);

			timerSet(ACTION_MQTT_SUBSCRIBE, 1000);

			// Restart lightread for receiving new data
			readLightEnabled = true;
			readLight.reset();
			lightResults.commited = false;
			break;

		} case MODE_NET: {

			sckOut("Entering Network mode!");

			timerSet(ACTION_UPDATE_SENSORS, 500, true);
			sckOut(String F("Publishing every ") + String(config.publishInterval) + F(" seconds"));
			break;

		} case MODE_SD: {

			ESPcontrol(ESP_OFF);

			sckOut("Entering SD card mode!");

			timerSet(ACTION_UPDATE_SENSORS, 500, true);
			sckOut(String F("Publishing every ") + String(config.publishInterval) + " seconds");
			break;

		} case MODE_BRIDGE: {
			ESPcontrol(ESP_ON);
			changeOutputLevel(OUT_SILENT);
			publishRuning = false;
			break;

		} case MODE_FLASH: {
			changeOutputLevel(OUT_SILENT);
			ESPcontrol(ESP_FLASH);
			publishRuning = false;
			break;
		} case MODE_SHELL: {
			ESPcontrol(ESP_OFF);
			for (uint8_t i=0; i<timerSlots; i++) {
				timers[i].action = ACTION_NULL;
				timers[i].interval = 0;
				timers[i].started = 0;
				timers[i].periodic = false;
			}
			publishRuning = false;
			break;
		} case MODE_OFF: {

			sckOut(F("Entering off mode..."));

			// Dont turn off ESP yet, only turn off its leds...
			if (!digitalRead(POWER_WIFI)) {
				msgBuff.com = ESP_LED_OFF;
				ESPqueueMsg(false, true);
			}
			
			// Turn off leds
			digitalWrite(SERIAL_TX_LED, HIGH);
			digitalWrite(SERIAL_RX_LED, HIGH);
			led.off();
			
			timerClear(ACTION_RECOVER_ERROR);

			break;
		} default: {
			;
		}
	}

	// Save changes
	if (newMode != prevMode && (newMode == MODE_SD || newMode == MODE_NET)) saveConfig();

	// Keep previous mode
	if (config.mode == MODE_NET || config.mode == MODE_SD) prevMode = config.mode;
	
	// Set new mode
	config.mode = newMode;

	// Update led
	led.update(newMode, 0);
}
void SckBase::errorMode() {
	
	// Clear timer for updating sensors 
	timerClear(ACTION_UPDATE_SENSORS);

	// Try to recover error every n milliseconds
	if (!timerExists(ACTION_RECOVER_ERROR)) timerSet(ACTION_RECOVER_ERROR, 1000, true);

	// Give led feedback about the error
	led.update(config.mode, 2);

	// Change to setup mode in n seconds
	if (!timerExists(ACTION_GOTO_SETUP)) timerSet(ACTION_GOTO_SETUP, 5000);

	// Start ESP to try time Sync
	if (digitalRead(POWER_WIFI)) ESPcontrol(ESP_ON);
}
void SckBase::changeOutputLevel(OutLevels newLevel) {
	prevOutputLevel = outputLevel;
	outputLevel = newLevel;
}
void SckBase::inputUpdate() {

	if (onUSB) {
		if (SerialUSB.available()) {
			char buff = SerialUSB.read();
			serialBuff += buff;
			sckOut((String)buff, PRIO_MED, false);			// Shell echo
			if (buff == 13 || buff == 10) { 				// New line detected
				sckOut("");
				sckIn(serialBuff);							// Process input
				serialBuff = "";
				prompt();
			}
		}
	}
}
void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF:
			if (!digitalRead(POWER_WIFI) && config.mode != MODE_BRIDGE) {
				closeFiles();
				sprintf(outBuff, "Turning off ESP: on for %.2f seconds", (millis() - espLastOn)/1000);
				sckOut();
				timerClear(ACTION_GET_ESP_STATUS);
				BUS_queueCount = 0;
				espSerialDebug = false;
				digitalWrite(POWER_WIFI, HIGH);		// Turn off ESP
				digitalWrite(GPIO0, LOW);
				espTotalOnTime += millis() - espLastOn;

				// Clears ESP status values
				for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) espStatus.value[i] = ESP_NULL_EVENT;
			}
			break;

		case ESP_FLASH:			// The only way out off flash mode is clicking button or resetting
			led.bridge();
			disableTimer5();
			sckOut("Putting ESP in flash mode...\r\nRemember to reboot ESP after flashing (esp reboot)!");
			if (!digitalRead(POWER_WIFI)) ESPcontrol(ESP_OFF);
			SerialUSB.begin(ESP_FLASH_SPEED);
			Serial1.begin(ESP_FLASH_SPEED);
			delay(500);
			digitalWrite(CH_PD, HIGH);
			digitalWrite(GPIO0, LOW);			// LOW for flash mode
			digitalWrite(POWER_WIFI, LOW); 		// Turn on ESP
			break;

		case ESP_ON:
			if (digitalRead(POWER_WIFI)) {
				closeFiles();
				sdPresent();
				sckOut("Turning on ESP...");
				SPI.end();						// Important for SCK-1.5.3 so the sdfat lib releases the SPI bus and the ESP can access his flash
				delay(10);
				digitalWrite(CH_PD, HIGH);
				digitalWrite(GPIO0, HIGH);		// HIGH for normal mode
				digitalWrite(POWER_WIFI, LOW); 	// Turn on ESP
				espLastOn = millis();
				delay(10);
				timerSet(ACTION_CLEAR_ESP_BOOTING, 500);
			}
			break;

		case ESP_REBOOT:
			sckOut("Restarting ESP...");
			ESPcontrol(ESP_OFF);
			delay(10);
			ESPcontrol(ESP_ON);
			break;
	}
}
void SckBase::ESPbusUpdate() {

	// If there are pending messages to send
	if (BUS_queueCount > 0) {

		// Send first message in queue
		msgOut.com = BUS_queue[1].com;
		strncpy(msgOut.param, BUS_queue[1].param, 240);
		msgOut.waitAnswer = BUS_queue[1].waitAnswer;
		BUS_out.sendData();

		// If we need to wait for confirmation
		bool waitAnswer = BUS_queue[1].waitAnswer;
		bool mesgReceived = false;
		if (waitAnswer) {
			uint32_t millisNow = millis();
			while (millis() - millisNow < answerTimeout) {
				if (BUS_in.receiveData()) {
					if (msgIn.com == msgOut.com) mesgReceived = true;
					ESPprocessMsg();
					if (mesgReceived) break;
				}
			}
			if (!mesgReceived) sckOut(".", PRIO_MED, false);
		}
		// Asume message sent and received
		if (!waitAnswer || mesgReceived) {
			// If we have more than one message, remove first one and move the rest
			if (BUS_queueCount > 1) {
				for (uint8_t i=2; i<=BUS_queueCount; i++){
					BUS_queue[i-1].com = BUS_queue[i].com;
					strncpy(BUS_queue[i-1].param, BUS_queue[i].param, 240);
					BUS_queue[i-1].waitAnswer = BUS_queue[i].waitAnswer;
				}
			}
		
			// Update queue index
			BUS_queueCount--;
		}
	}

	// If there is something the ESP want to tell to us
	if (BUS_in.receiveData()) ESPprocessMsg();
}
void SckBase::ESPqueueMsg(bool sendParam, bool waitAnswer) {

	// sprintf(outBuff, "ESP queueing command: %i", msgBuff.com);
	// sckOut();

	ESPcontrol(ESP_ON);

	bool alreadyQueued = false;

	// Don't accept message if its already on queue
	for (uint8_t i=1; i<=BUS_queueCount; i++){
		if (BUS_queue[i].com == msgBuff.com) alreadyQueued = true;
	}

	if (!alreadyQueued) {

		BUS_queueCount++;

		// Put command message buffer in queue
		BUS_queue[BUS_queueCount].com = msgBuff.com;

		// Do we need to wait answer for this message??
		if (waitAnswer) BUS_queue[BUS_queueCount].waitAnswer = true;
		else BUS_queue[BUS_queueCount].waitAnswer = false;

		// If no need to send params
		if (!sendParam) strncpy(BUS_queue[BUS_queueCount].param, "", 240);
		else strncpy(BUS_queue[BUS_queueCount].param, msgBuff.param, 240);
	}

	// If there is an urgent message...
	if (waitAnswer) ESPbusUpdate();
}
void SckBase::ESPprocessMsg() {

	sckOut("Processing message from ESP...", PRIO_LOW);
	sckOut(String F("Command: ") + String(msgIn.com), PRIO_LOW);
	sckOut(String F("Parameters: ") + String(msgIn.param), PRIO_LOW);

	switch(msgIn.com) {
		case ESP_GET_STATUS_COM: {
			processStatus();
			break;
		} case ESP_DEBUG_EVENT: {

			sckOut(String F("ESP > ") + String(msgIn.param));
			break;

		} case ESP_SET_WIFI_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);

			int comparison = strcmp(config.ssid, jsonNet["s"]);
			if (comparison == 0) {
				sprintf(outBuff, "New ESP network: %s - %s", config.ssid, config.pass);
				sckOut();
			} else setESPwifi();
			break;

		} case ESP_SET_TOKEN_COM: {

			int comparison = strcmp(config.token, msgIn.param);
			if (comparison == 0) {
				sprintf(outBuff, "New ESP token: %s", config.token);
				sckOut();
			} else setESPtoken();
			break;

		} case ESP_CLEAR_WIFI_COM: {

			sckOut(F("Wifi networks deleted!!!"));
			break;

		} case ESP_GET_WIFI_COM: {
			
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			String tssid = jsonNet["s"];
			String tpass = jsonNet["p"];

			
			if (tssid.length() <= 0) {
				sckOut("No wifi configured on ESP!!!");
			} else {
				sckOut("Received new Wifi from ESP!!!");

				char newSsid[64];
				tssid.toCharArray(newSsid, 64);

				char newPass[64];
				tpass.toCharArray(newPass, 64);

				saveWifi(newSsid, newPass);
			}
			break;

		} case ESP_GET_NET_INFO_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonIP = jsonBuffer.parseObject(msgIn.param);
			String tip = jsonIP["ip"];
			String tmac = jsonIP["mac"];
			String thostname = jsonIP["hn"];
			sprintf(outBuff, "\r\nHostname: %s\r\nIP address: %s\r\nMAC address: %s", thostname.c_str(), tip.c_str(), tmac.c_str());
			sckOut(PRIO_HIGH);
			prompt();
			break;

		} case ESP_GET_TOKEN_COM: {

			char newToken[8];
			strncpy(newToken, msgIn.param, 8);
			saveToken(newToken);
			break;

		} case ESP_GET_CONF_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonConf = jsonBuffer.parseObject(msgIn.param);
			
			String tmode = jsonConf["mo"];
			sckOut(String("modo --> ") + tmode);
			config.persistentMode = static_cast<SCKmodes>(tmode.toInt());
			config.publishInterval 	= jsonConf["ri"];
			String tssid = jsonConf["ss"];
			tssid.toCharArray(config.ssid, 64);
			String tpass = jsonConf["pa"];
			tpass.toCharArray(config.pass, 64);
			String ttoken = jsonConf["to"];
			ttoken.toCharArray(config.token, 64);
					
			sckOut(F("Configuration updated:"));
			sprintf(outBuff, "Publish Interval: %lu\r\nWifi: %s - %s\r\nToken: %s", config.publishInterval, config.ssid, config.pass, config.token);
			sckOut();
			saveConfig();
			break;

		} case ESP_GET_APCOUNT_COM: {

			break;

		} case ESP_GET_VERSION_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonVer = jsonBuffer.parseObject(msgIn.param);
			String ESPv = jsonVer["ver"];
			String ESPbd = jsonVer["date"];
			sckOut(String F("ESP version:\t\t") + ESPv + F(" (") + ESPbd + F(")"));
			break;

		} case ESP_CONSOLE_COM: {

			espConsole = true;
			
			// Process command
			sckIn(String(msgIn.param));

			// Publish result
			// msgBuff.com = ESP_CONSOLE_PUBLISH;
			// ESPqueueMsg(false, true);

			// espConsole = false;
			break;

		} case ESP_MQTT_CONSOLE_COM: {

			sprintf(outBuff, "MQTT console: %s", msgIn.param);

			if (config.mode == MODE_SETUP) sckIn(String(msgIn.param));
			break;

		} case ESP_GET_APLIST_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			int number = jsonNet["n"];
			String ssid = jsonNet["s"];
			int32_t rssid = jsonNet["r"];
			sckOut(String(number+1) + F(": ") + ssid + F(" (") + rssid + F(")"));
			prompt();
			break;

		} case ESP_GET_TIME_COM: {
			String epochSTR = String(msgIn.param);
			setTime(epochSTR);
			break;

		} case ESP_MQTT_PUBLISH_COM: {
			sckOut(F("ESP sensor readings updated!!!"), PRIO_LOW);
			break;

		} case ESP_MQTT_CLEAR_STATUS :{
			espStatus.mqtt = ESP_NULL_EVENT;
			sckOut(F("ESP MQTT status cleared!!"), PRIO_LOW);
			break;

		} case ESP_MQTT_HELLOW_COM: {
			sckOut(F("ESP trying MQTT hellow..."));
			break;

		} case ESP_GET_FREE_HEAP_COM:{

			sckOut(String F("ESP free heap: ") + String(msgIn.param));
			break;

		} //case ESP_WEB_CONFIG_SUCCESS: {

			// sckOut(F("Configuration changed via WebServer!!!"));
			// led.configOK();
		 // 	readLightEnabled = false;
		 // 	readLight.reset();

		 // 	//MQTT Hellow for Onboarding process
			// triggerHello = true;
			// break;

		//}
	}

	// Clear msg
	msgIn.com = 0;
	strncpy(msgIn.param, "", 240);
}
void SckBase::getStatus() {

	msgBuff.com = ESP_GET_STATUS_COM;
	strncpy(msgBuff.param, "", 240);
	ESPqueueMsg(false);
}
void SckBase::processStatus() {

	espStatusTypes statusReceived = ESP_STATUS_TYPES_COUNT;

	for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) {

		statusReceived = static_cast<espStatusTypes>(i);
		espStatus.value[statusReceived] = static_cast<espStatusEvents>(msgIn.param[i]); 

	}

	// Wifi status has changed
	if (espStatus.wifi != prevEspStatus.wifi) {

		bool wasError = false;

		switch (espStatus.wifi) {
			case ESP_WIFI_CONNECTED_EVENT: {
				sckOut(F("Conected to wifi!!"));

				// Feedback
				if (config.mode != MODE_SETUP) led.update(config.mode, 0);

				// Send MQTT Hello
				if (triggerHello) {
					sckOut(F("Sending MQTT Hello..."));
					msgBuff.com = ESP_MQTT_HELLOW_COM;
					ESPqueueMsg(false, true);
				}

				// Forced Time sync
				if (!rtc.isConfigured() || rtc.getYear() < 17) {
					sckOut(F("OUT OF TIME, Asking time to ESP..."));
					msgBuff.com = ESP_GET_TIME_COM;
					ESPqueueMsg(false, true);
				}
				break;

			} case ESP_WIFI_ERROR_EVENT: {
				sckOut(F("Wifi ERROR: undefined!!"));
				wasError = true;
				break;

			} case ESP_WIFI_ERROR_PASS_EVENT: {
				sckOut(F("Wifi ERROR: wrong password!!"));
				wasError = true;
				break;

			} case ESP_WIFI_ERROR_AP_EVENT: {
				sckOut(F("Wifi ERROR: can't find access point!!"));
				wasError = true;
				break;

			} case ESP_WIFI_NOT_CONFIGURED: {
				if (!wifiSet) sckOut("ESP: Wifi is not configured!!!");
				else setESPwifi();
				wasError = true;
				break;

			} default: break;
		}

		// If there was ANY wifi error...
		if (wasError) {
			// If we NEED network go to error mode
			if (config.mode == MODE_NET) errorMode();
		}
	}

	// Net status has changed
	if (espStatus.net != prevEspStatus.net) {

	}

	// Mqtt status has changed
	if (espStatus.mqtt != prevEspStatus.mqtt) {

		if (espStatus.mqtt != ESP_NULL_EVENT) {

			// Clear mqtt status on queue
			msgBuff.com = ESP_MQTT_CLEAR_STATUS;
			ESPqueueMsg(false, false);
			
			// Get status on queue
			msgBuff.com = ESP_GET_STATUS_COM;
			strncpy(msgBuff.param, "", 240);
			ESPqueueMsg(false, true);
		}

		switch (espStatus.mqtt) {
			case ESP_MQTT_PUBLISH_OK_EVENT: {
				sckOut(F("MQTT publish OK!!"));

				// Remember at wich time we publish
				lastPublishTime = rtc.getEpoch();

				// Clear the published readings
				RAMreadingsIndex = RAMreadingsIndex - RAMgroups[RAMgroupIndex].numberOfReadings;
				RAMgroupIndex = RAMgroupIndex - 1;

				// Check for more readings pending to be published
				if (RAMgroupIndex >= 0) {
					publishRuning = false;
					publish();
				}

				// If we finish network publish start with sdcard (Can't do it at the same time because of the SPI bug)
				else publishToSD();

				break;

			} case ESP_MQTT_HELLO_OK_EVENT: {
				sckOut(F("MQTT Hello OK!!"));
				triggerHello = false;

				// Go to network mode
				if(config.mode != MODE_NET && config.persistentMode == MODE_NET) changeMode(MODE_NET);
				break;

			} case ESP_MQTT_ERROR_EVENT: {
				if (config.mode == MODE_NET) led.update(config.mode, 2);
				sckOut(F("ERROR: MQTT failed!!"));

				// For now keep them in SD and wait for next publish...
				publishToSD();
				break;

			} case ESP_NULL_EVENT: {
				sckOut("MQTT status cleared!!!");
				break;

			} default: break;
		}

		if (espStatus.mqtt != ESP_NULL_EVENT) {
			msgBuff.com = ESP_MQTT_CLEAR_STATUS;
			ESPqueueMsg(false, false);
		}
	}

	// Time status has changed
	if (espStatus.time != prevEspStatus.time) {
		switch (espStatus.time) {
			case ESP_TIME_FAIL_EVENT: {

				sckOut(F("NTP sync ERROR!!"));
				if (config.mode != MODE_SETUP) {
					if (!onTime) led.update(config.mode, 1);	
				}
				break;

			} case ESP_TIME_UPDATED_EVENT: {
				
				// Time sync
				sckOut(F("Asking time to ESP..."));
				msgBuff.com = ESP_GET_TIME_COM;
				ESPqueueMsg(false, false);
				break;

			} default: break;
		}
	}

	// Mode status has changed
	if (espStatus.ap != prevEspStatus.ap) {
		switch (espStatus.ap) {
			case ESP_AP_ON_EVENT: {
				sckOut(F("Started APmode"));
				break;

			} case ESP_AP_OFF_EVENT: {
				if (prevEspStatus.ap == ESP_AP_ON_EVENT) sckOut(F("Stoped APmode"));
				break;
			} default: break;
		}
	}

	// Make sure we have AP on in setup mode
	if (config.mode == MODE_SETUP && espStatus.ap == ESP_AP_OFF_EVENT) {
		if (!timerExists(ACTION_START_AP_MODE)) timerSet(ACTION_START_AP_MODE, 5000);
	}

	// WebServer status has changed
	if (espStatus.web != prevEspStatus.web) {
		switch (espStatus.web) {
			case ESP_WEB_ON_EVENT: {
				sckOut(F("Web server started"));
				break;

			} case ESP_WEB_OFF_EVENT: {
				if (prevEspStatus.web == ESP_WEB_ON_EVENT) sckOut(F("Web server stoped"));
				break;

			} default: break;
		}
	}

	// Token status has changed
	if (espStatus.token != prevEspStatus.token) {
		switch(espStatus.token) {
			case ESP_TOKEN_ERROR: {
				sckOut("ESP: No token configured!!");
				if (tokenSet) setESPtoken();
				break;

			} case ESP_TOKEN_OK: {
				sckOut("ESP: Token OK!!", PRIO_LOW);
				break;

			} default: break;
		}
	}

	// Conf status has changed
	if (espStatus.conf != prevEspStatus.conf) {
		switch(espStatus.conf) {
			case ESP_CONF_CHANGED_EVENT: {

				sckOut(F("Configuration changed via ESP WebServer!!!"));
				led.configOK();
			 	readLightEnabled = false;
			 	readLight.reset();

		 		//MQTT Hellow for Onboarding process
				triggerHello = true;

				msgBuff.com = ESP_GET_CONF_COM;
				ESPqueueMsg(false, false);
				break;
			} default: break;
		} 
	}

	// Make a copy of status
	for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) prevEspStatus.value[i] = espStatus.value[i];
}
bool SckBase::onWifi() {

	if (espStatus.wifi == ESP_WIFI_CONNECTED_EVENT) return true;
	return false;
}

/* Process text inputs and executes commands
 *
 *	@params strIn Text input
 */
void SckBase::sckIn(String strIn) {

	//soportar multiple words in commands
	strIn.replace("\n", "");
	strIn.replace("\r", "");
	strIn.trim();

	uint8_t executedCommand = EXTCOM_COUNT + 1;

	if (strIn.length() > 0) {
		// Search in title list
		for (uint8_t i=0; i < EXTCOM_COUNT; ++i) {
			if (strIn.startsWith(comTitles[i])) {
				executedCommand = i;
				strIn.replace(comTitles[i], "");
				strIn.trim();
				break;
			}
		}
	} else {
		executedCommand = EXTCOM_COUNT;
	}

	switch(executedCommand) {

		// ESP commands
		case EXTCOM_ESP_REBOOT: {
			ESPcontrol(ESP_REBOOT);
			break;

		} case EXTCOM_ESP_OFF: {
			ESPcontrol(ESP_OFF);
			break;

		} case EXTCOM_ESP_ON: {
			ESPcontrol(ESP_ON);
			break;

		} case EXTCOM_ESP_START_AP: {
			sckOut(F("Asking ESP start ap mode..."), PRIO_HIGH);
			msgBuff.com = ESP_START_AP_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_STOP_AP: {
			sckOut(F("Asking ESP stop ap mode..."), PRIO_HIGH);
			msgBuff.com = ESP_STOP_AP_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_START_WEB: {
			sckOut(F("Asking ESP start web server..."), PRIO_HIGH);
			msgBuff.com = ESP_START_WEB_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_STOP_WEB: {
			sckOut(F("Asking ESP stop web server..."), PRIO_HIGH);
			msgBuff.com = ESP_STOP_WEB_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_SLEEP: {

			sckOut(F("Sleeping ESP..."), PRIO_HIGH);
			msgBuff.com = ESP_DEEP_SLEEP_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_WAKEUP: {

			digitalWrite(CH_PD, LOW);
			delay(10);
			digitalWrite(CH_PD, HIGH);
			break;

		} case EXTCOM_GET_APLIST: {
			msgBuff.com = ESP_GET_APLIST_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_LED_OFF: {
			msgBuff.com = ESP_LED_OFF;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_LED_ON: {
			msgBuff.com = ESP_LED_ON;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_ESP_SERIAL_DEBUG_TOGGLE: {
			if (!digitalRead(POWER_WIFI)) {
				msgBuff.com = ESP_SERIAL_DEBUG_TOGGLE;
				espSerialDebug = true;
				ESPqueueMsg(false);
			} else {
				sckOut(F("Please start ESP first!"));
			}
			break;

		} case EXTCOM_ESP_MQTT_HELLO: {

			msgBuff.com = ESP_MQTT_HELLOW_COM;
			ESPqueueMsg(false, true);
			break;

		} case EXTCOM_ESP_SHOW_STATUS: {

			for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) {
				sprintf(outBuff, "%s: %s", espStatus.name[i], espStatus.eventTitle[espStatus.value[i]]);
				sckOut();
			}
			break;

		// Configuration commands
		} case EXTCOM_SET_WIFI: {
			String separator;
			if (strIn.indexOf('"') >= 0) separator = '"';
			else if (strIn.indexOf("'") >= 0) separator = "'";
			uint8_t first = strIn.indexOf(separator);
			uint8_t second = strIn.indexOf(separator, first + 1);
			uint8_t third = strIn.indexOf(separator, second + 1);
			uint8_t fourth = strIn.indexOf(separator, third + 1);

			if (strIn.substring(first + 1, second).length() > 0) {

				char newSsid[64];
				strIn.substring(first + 1, second).toCharArray(newSsid, 64);

				char newPass[64];
				strIn.substring(third + 1, fourth).toCharArray(newPass, 64);

				saveWifi(newSsid, newPass);
				setESPwifi();

			} else {
				sckOut("Error setting wifi, please try again!!!");
			}
			break;

		} case EXTCOM_GET_WIFI: {

			if (wifiSet) {
				sprintf(outBuff, "SSID: %s\r\nPASS: %s", config.ssid, config.pass);
				sckOut();
			} else {
				sckOut("No network configured!!!");
			}
			break;

		// } case EXTCOM_GET_BEST_WIFI: {
		// 	msgBuff.com = ESP_GET_BEST_WIFI_COM;
		// 	ESPqueueMsg(false);
		// 	break;

		} case EXTCOM_CLEAR_WIFI: {
			clearWifi();
			saveConfig();
			break;

		} case EXTCOM_GET_NET_INFO: {
			sckOut("Asking ESP for network info...");
			msgBuff.com = ESP_GET_NET_INFO_COM;
			ESPqueueMsg(false, true);
			break;

		} case EXTCOM_SET_TOKEN: {
			if (strIn.length() != 6) {
				sckOut(F("Token should have 6 characters!!!"), PRIO_HIGH);
			} else {
				char newToken[8];
				strIn.toCharArray(newToken, 8);
				saveToken(newToken);
				sprintf(outBuff, "New token: %s", config.token);
				sckOut();
				setESPtoken();
			}
			break;
		
		} case EXTCOM_CLEAR_TOKEN: {

			clearToken();
			saveConfig();
			break;

		} case EXTCOM_GET_VERSION: {
			msgBuff.com = ESP_GET_VERSION_COM;
			ESPqueueMsg(false, true);

			prompt();
			sckOut(String F("Hardware version:\t") + hardwareVer);
			prompt();
			sckOut(String F("SAM version:\t\t") + SAMversion + F(" (") + SAMbuildDate + F(")"));
			break;

		} case EXTCOM_SYNC_CONFIG: {
			loadConfig();
			break;

		} case EXTCOM_DOWNLOAD_CONFIG: {
			sckOut(F("To be implemented!!!"), PRIO_HIGH);
			break;

		} case EXTCOM_SET_CONFIG: {

			strIn.toLowerCase();

			// TODO poner los params en el help
			if (strIn.startsWith("factory"))  {

				sckOut(String F("Saving factory defaults, reseting in 1 second..."), PRIO_HIGH);
				factoryReset();

			} else if (strIn.startsWith("publish interval")) {
				strIn.replace("publish interval", "");
				strIn.trim();
				uint32_t intTinterval = strIn.toInt();

				if (intTinterval > 0 && intTinterval < 86400) {
					config.publishInterval = intTinterval;
					saveConfig();
					sckOut(String F("Change publish interval to: ") + String(config.publishInterval), PRIO_HIGH);
				}

			} else if (strIn.startsWith("mode")) {

				strIn.replace("mode", "");
				strIn.trim();

				SCKmodes requestedMode = MODE_COUNT;

				for (uint8_t i=0; i < MODE_COUNT; ++i) {
					if (strIn.startsWith(modeTitles[i])) {
						requestedMode = static_cast<SCKmodes>(i);
						if (requestedMode == MODE_SD || requestedMode == MODE_NET) {
							config.persistentMode = requestedMode;
							changeMode(requestedMode);
							break;
						} else if (requestedMode == MODE_FLASH) {
							changeMode(MODE_FLASH);
							break;
						}
					}
				}
				if (requestedMode == MODE_COUNT) sckOut(F("Unrecognized mode, please try again!"));

			}
			break;

		} case EXTCOM_GET_CONFIG: {

			sprintf(outBuff, "Token: %s\r\nCurrent mode: %s\r\nPublish Interval: %ld", config.token, modeTitles[config.persistentMode], config.publishInterval);
			sckOut();
			break;

		// Mode commands
		} case EXTCOM_RESET: {
			sckOut(F("Bye!"), PRIO_HIGH);
			softReset();
			break;

		} case EXTCOM_RESET_CAUSE: {
			uint8_t resetCause = PM->RCAUSE.reg;
			switch(resetCause){
				case 1:
					sckOut(F("POR: Power On Reset"));
					break;
				case 2:
					sckOut(F("BOD12: Brown Out 12 Detector Reset"));
					break;
				case 4:
					sckOut(F("BOD33: Brown Out 33 Detector Reset"));
					break;
				case 16:
					sckOut(F("EXT: External Reset"));
					break;
				case 32:
					sckOut(F("WDT: Watchdog Reset"));
					break;
				case 64:
					sckOut(F("SYST: System Reset Request"));
					break;
			}
			break;

		// Other configuration
		} case EXTCOM_SET_OUTLEVEL: {
			if (strIn.length() > 0) {
				uint8_t newLevel = (uint8_t)strIn.toInt();
				if (newLevel >= 0 && newLevel <= 2) {
					if (newLevel == 0) outputLevel = OUT_SILENT;
					else if (newLevel == 1) outputLevel = OUT_NORMAL;
					else outputLevel = OUT_VERBOSE;
					sckOut(String F("Output level set to ") + outLevelTitles[outputLevel], PRIO_HIGH);
				}
			}
			break;

		} case EXTCOM_GET_OUTLEVEL: {
			sckOut(String F("Current output level is ") + outLevelTitles[outputLevel], PRIO_HIGH);
			break;

		} case EXTCOM_SET_LED: {
			if (strIn.startsWith("off")) {
				led.off();
			} else if (strIn.startsWith("white")) {
				led.reading();
			}
			break;

		} case EXTCOM_GET_URBAN_PRESENT: {
			if (urbanBoardDetected()) sckOut(F("Urban board detected!!"), PRIO_HIGH);
			else sckOut(F("Urban board not found!!"), PRIO_HIGH);
			break;

		} case EXTCOM_READLIGHT_ON: {

			sckOut(F("Turning readlight ON..."));
			readLightEnabled = true;
			break;

		} case EXTCOM_READLIGHT_OFF: {

			sckOut(F("Turning readlight OFF..."));
			readLightEnabled = false;
			break;

		} case EXTCOM_READLIGHT_RESET: {

			readLight.reset();
			readLightEnabled = true;
			break;

		} case EXTCOM_READLIGHT_TOGGLE_DEBUG: {

			readLight.debugFlag = !readLight.debugFlag;
			sckOut(F("Readlight debug flag: "), PRIO_MED, false);
			if (readLight.debugFlag) sckOut(F("true"));
			else sckOut(F("false"));
			break;

		} case EXTCOM_MQTT_CONFIG: {

			if (strIn.startsWith("on")) mqttConfig(true);
			else if (strIn.startsWith("off")) mqttConfig(false);
			break;

		// Time configuration
		} case EXTCOM_SET_TIME: {
			setTime(strIn);

			strncpy(msgBuff.param, strIn.c_str(), 64);
			msgBuff.com = ESP_SET_TIME_COM;
			ESPqueueMsg(true, true);
			break;

		} case EXTCOM_GET_TIME: {
			if (ISOtime()) {
				if (strIn.equals("epoch")) {
					sckOut(String(rtc.getEpoch()));
				} else {
					sckOut(ISOtimeBuff);
				}
			} else sckOut(F("Time NOT synced since last reset!!!"), PRIO_HIGH);
			
			break;

		} case EXTCOM_SYNC_HTTP_TIME: {
			sckOut("Asking ESP for HTTP time sync...");
			msgBuff.com = ESP_SYNC_HTTP_TIME_COM;
			ESPqueueMsg(false, true);

			break;

		// SD card
		} case EXTCOM_SD_PRESENT: {
			if (sdPresent()) sckOut(F("Sdcard ready!!!"));
			break;

		} case EXTCOM_GET_SENSOR: {

			// prepare string for matching
			strIn.toLowerCase();

			// fin out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			// Failed to found your sensor
			if (wichSensor < SENSOR_COUNT) {
				
				// Get reading
				if (getReading(wichSensor)) {
					
					OneSensor *thisSensor = &sensors[wichSensor];
					sckOut(String(thisSensor->title) + ": " + String(thisSensor->reading) + " " + thisSensor->unit);
				
				} else {
					retrySensor = wichSensor;
					timerSet(ACTION_RETRY_READ_SENSOR, 200);
				}
			}
			break;

		} case EXTCOM_RAM_COUNT: {
		
			if (RAMreadingsIndex > 0) {
				sprintf(outBuff, "%u readings stored in RAM (max:%u)", RAMreadingsIndex + 1, ram_max_readings);
				sckOut();
			} else sckOut("No readings stored in RAM");
			

			break;

		} case EXTCOM_RAM_READ: {

			uint16_t startIndex = 0;

			if (RAMgroupIndex > 4) startIndex = RAMgroupIndex - 4;

			for (uint16_t i=startIndex; i<=RAMgroupIndex; ++i)	{

				RAMgetGroup(i);

				char tmpTime[20];
				epoch2iso(groupBuffer.time, tmpTime);

				sprintf(outBuff, "* %s:", tmpTime);
				sckOut();

				for (uint8_t ii=0; ii<groupBuffer.numberOfReadings; ii++) {

				 	sprintf(outBuff, " - %s: %.2f %s", sensors[groupBuffer.data[ii].sensor].title, groupBuffer.data[ii].reading, sensors[groupBuffer.data[ii].sensor].unit);
				 	sckOut();
				}
			}
			break;

		} case EXTCOM_GET_POWER_STATE: {

			sprintf(outBuff,   "CHG:  %0.2f v\r\nISET:  %0.2f v\r\nVUSB:  %0.2f v\r\nBatt volt:  %0.2f v\r\nBatt:  %0.2f %%\r\nOn USB:  %i\r\nCharging:  %i",
								getVoltage(CHG_CHAN),
								// getCHG(),
								getVoltage(ISET_CHAN),
								// getISET(),
								getVoltage(USB_CHAN),
								// getUSBvoltage(),
								getVoltage(BATT_CHAN),
								// getBatteryVoltage(),
								getBatteryPercent(),
								onUSB,
								charging
					);
			sckOut();
			break;

		} case EXTCOM_SLEEP: {

			goToSleep();
			break;

		} case EXTCOM_SET_CHARGER_CURRENT: {

			uint16_t currentToSet = strIn.toInt();
			if (currentToSet > 0 && currentToSet < 1000) {
				sprintf(outBuff, "Setting charge current to %u mA", currentToSet);
				sckOut();
				writeCurrent(currentToSet);

				float readedOhms = readResistor(0);
				sprintf(outBuff, "Actual resitor value: %.2f Ohms", readedOhms);
				sckOut();
				
			}
			
			break;

		} case EXTCOM_ENABLE_SENSOR: {

			// Prepare string for matching
			strIn.toLowerCase();

			// Find out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			if (wichSensor < SENSOR_COUNT) enableSensor(wichSensor);

			break;

		} case EXTCOM_DISABLE_SENSOR: {

			// prepare string for matching
			strIn.toLowerCase();

			// fin out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			if (wichSensor < SENSOR_COUNT) disableSensor(wichSensor);

			break;

		} case EXTCOM_SET_INTERVAL_SENSOR: {

			// prepare string for matching
			strIn.toLowerCase();

			// fin out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			if (wichSensor < SENSOR_COUNT) {

				// Remove title so only command is left in input string
				String titleCompare = sensors[wichSensor].title;
				titleCompare.toLowerCase();
				strIn = cleanInput(titleCompare, strIn);

				uint16_t newInterval = strIn.toInt();

				if (newInterval < minimal_sensor_reading_interval || newInterval > max_sensor_reading_interval) {
					sckOut("No valid interval received!! please try again...");
					break;
				}

				OneSensor *thisSensor = &sensors[wichSensor];
				sprintf(outBuff, "%s new read Interval: %u", thisSensor->title, newInterval);
				sckOut();
				sensors[wichSensor].interval = newInterval;
				saveConfig();
			}
			break;

			break;

		} case EXTCOM_CONTROL_SENSOR: {

			// prepare string for matching
			strIn.toLowerCase();

			// fin out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			if (wichSensor < SENSOR_COUNT) {
			
				if (strIn.length() < 1) {
					sckOut(F("No command received!! please try again..."));
					break;
				}

				if (sensors[wichSensor].controllable)  {

					// Remove title so only command is left in input string
					String titleCompare = sensors[wichSensor].title;
					titleCompare.toLowerCase();
					strIn = cleanInput(titleCompare, strIn);
					
					// Print sensor title
					sckOut(String(sensors[wichSensor].title) + ": " + strIn);

					switch (sensors[wichSensor].location) {
						case BOARD_URBAN: {
							sckOut(urban.control(wichSensor, strIn));
							break;

						} case BOARD_AUX: {
							sckOut(auxBoards.control(wichSensor, strIn));
							break;
							
						} default: {
							;
						}
					}

				} else {
					sckOut(String F("No configured command found for ") + sensors[wichSensor].title + F(" sensor!!!"));
				}
			}
			break;

		} case EXTCOM_U8G_PRINT: {

			auxBoards.print(SENSOR_GROOVE_OLED, strIn);
			break;

		} case EXTCOM_U8G_PRINT_SENSOR: {

			// prepare string for matching
			strIn.toLowerCase();

			// fin out wich sensor is
			SensorType wichSensor = getSensorFromString(strIn);

			if (wichSensor < SENSOR_COUNT) {

				getReading(wichSensor);
				ISOtime();

				OneSensor *thisSensor = &sensors[wichSensor];
				String Stitle = thisSensor->title;
				String Sreading = String(thisSensor->reading, 1);
				String Sunit = thisSensor->unit;
				auxBoards.displayReading(Stitle, Sreading, Sunit, ISOtimeBuff);

				sckOut(String F("Printing ") + Stitle);
			}

			break;

		} case EXTCOM_LIST_SENSORS: {

			SensorType thisType = SENSOR_COUNT;

			sprintf(outBuff, "\r\nEnabled\r\n----------");
			sckOut();
			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				thisType = static_cast<SensorType>(i);

				if (sensors[thisType].enabled) sckOut(String(sensors[thisType].title) + " (" + String(sensors[thisType].interval) + " sec)");
			}

			sprintf(outBuff, "\r\nDisabled\r\n----------");
			sckOut();
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				thisType = static_cast<SensorType>(i);

				if (!sensors[thisType].enabled) sckOut(sensors[thisType].title);
			}


			break;

		} case EXTCOM_PUBLISH: {
			publish();
			break;

		} case EXTCOM_ESP_GET_FREEHEAP: {

			// Get ESP free heap
			msgBuff.com = ESP_GET_FREE_HEAP_COM;
			ESPqueueMsg(false);

			// TODO get SAM free heap

			break;

		} case EXTCOM_GET_FREE_RAM: {

			sckOut(String(freeRAM()));
			break;

		} case EXTCOM_LIST_TIMERS: {

			for (uint8_t i=0; i<timerSlots; i++){
				if (timers[i].action > 0) {
					sprintf(outBuff, "Timer action: %i, every %lu milliseconds", timers[i].action, timers[i].interval);
					if (timers[i].periodic) sprintf(outBuff, "%s running periodically", outBuff);
					sckOut();
				}
			}
			break;

		// Help
		} case EXTCOM_HELP: {
			sckOut("", PRIO_HIGH);
			for (uint8_t i=0; i<EXTCOM_COUNT-2; i+=3) {
				for (uint8_t ii=0; ii<3; ii++) {
					sckOut("- ", PRIO_HIGH, false);
					sckOut(comTitles[i+ii], PRIO_HIGH, false);
					for (uint8_t iii=0; iii<3-((String(comTitles[i+ii]).length() + 2) / 8); iii++){
						sckOut(F("\t"), PRIO_HIGH, false);
					}
				}
				sckOut("", PRIO_HIGH);
			}
			sckOut("", PRIO_HIGH);
			break;

		// Just linebreak
		} case EXTCOM_COUNT: {
			break;

		} default: {
			sckOut(F("Unrecognized command, try help!"), PRIO_HIGH);
			break;
		}
	}

	// If the esp web console is waiting for an answer...
	if (espConsole) {
		msgBuff.com = ESP_CONSOLE_PUBLISH;
		ESPqueueMsg(false, true);
		espConsole = false;
	}
}
SensorType SckBase::getSensorFromString(String strIn) {
	SensorType wichSensor = SENSOR_COUNT;
	uint8_t maxWordsFound = 0;

	// Get sensor type
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType thisSensor = static_cast<SensorType>(i);

		// Makes comparison lower case and not strict
		String titleCompare = sensors[thisSensor].title;
		titleCompare.toLowerCase();
		strIn.toLowerCase();

		// How many words match in Sensor title
		uint8_t matchedWords = countMatchedWords(titleCompare, strIn);

		if (matchedWords > maxWordsFound) {
			maxWordsFound = matchedWords;
			wichSensor = thisSensor;
		}

	}
	if (wichSensor == SENSOR_COUNT) sckOut(F("Can't find that sensor!!!"));
	return wichSensor;
}

/* Text outputs
 *
 *	@params strOut 		Text output
 	@params priority	Level of priority of the message: 
 							PRIO_LOW - only showed in OUT_VERBOSE
 							PRIO_MED (default) - showed in VERBOSE and OUT_NORMAL
							PRIO_HIGH - showed in all modes (even in OUT_SILENT)
	@params newLine		Print a carriage return after output?
 */
void SckBase::sckOut(String strOut, PrioLevels priority, bool newLine) {

	strOut.toCharArray(outBuff, strOut.length()+1);
	sckOut(priority, newLine);
}
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine) {

	strncpy(outBuff, strOut, 240);
	sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine) {

	if (espConsole) {
		if (outputLevel + priority > 1) {
			strncpy(msgBuff.param, outBuff, 240);
			msgBuff.com = ESP_CONSOLE_COM;
			ESPqueueMsg(true, false);
		}
	}

	if (onUSB) {
		if (outputLevel + priority > 1) {
			SerialUSB.print(outBuff);
			if (newLine) SerialUSB.println();
		}
	}
	strncpy(outBuff, "", 128);
}
void SckBase::prompt() {

	sckOut("SCK > ", PRIO_MED, false);
}



// 	-------------
// 	|	Time    |
// 	-------------
//
bool SckBase::setTime(String epoch) {
	// validate time here!!!
	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		sckOut("RTC updated!!!");
		onTime = true;
		userLastAction = rtc.getEpoch();		// Restart las action to avoid comparations with wrong time
		ISOtime();
		sckOut(ISOtimeBuff);
		prompt();
		return true;
	}
	else sckOut("RTC update failed!!");
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

// 	---------------
// 	|	Sensors   |
// 	---------------
//
void SckBase::enableSensor(SensorType wichSensor) {

	// Get lastReadingTime from the last updated sensor (this puts de sensor behind the rest of the enabled sensors in terms of time)
	uint32_t last = 0;
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		SensorType indexSensor = static_cast<SensorType>(i);
		if (sensors[indexSensor].enabled && sensors[indexSensor].lastReadingTime > last) last = sensors[indexSensor].lastReadingTime;
	}

	sprintf(outBuff, "Enabling %s", sensors[wichSensor].title);
	sckOut();

	// Enable sensor 
	sensors[wichSensor].enabled = true;

	// Set the next reading to be after the rest of the sensors
	sensors[wichSensor].lastReadingTime = last;

	// Save Config
	saveConfig();

	// For Sdcard headers
	headersChanged = rtc.getEpoch();
}
void SckBase::disableSensor(SensorType wichSensor) {

	if (wichSensor < SENSOR_COUNT) {

		OneSensor *thisSensor = &sensors[wichSensor];
		sckOut(String F("Disabling ") + thisSensor->title);
		sensors[wichSensor].enabled = false;
		saveConfig();

		headersChanged = rtc.getEpoch();
	}

	// If we are disabling MICS, turn off heater
	if (wichSensor == SENSOR_CO || wichSensor == SENSOR_NO2) urban.gasOff(wichSensor);
}
void SckBase::updateSensors() {

	// Don't take readings if RTC is not updated
	if (!onTime) {
		sckOut("RTC is not updated!!!");
		errorMode();
		return;
	}

	// In network mode don't take readings if token or wifi is not configured
	if (config.mode == MODE_NET && (!tokenSet || !wifiSet)) {
		sckOut("Wifi or token configuration missing!!!");
		errorMode();
		return;
	}

	if (config.mode == MODE_SD && !sdPresent()) {
		errorMode();
		return;
	}

	// Only update sensor readings if we are in network or sdcard modes
	if (config.mode == MODE_NET || config.mode == MODE_SD) {

		uint32_t nextReadingTime = 0;

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			if (sensors[wichSensor].enabled) {
				
				uint32_t elapsed = rtc.getEpoch() - sensors[wichSensor].lastReadingTime;
				if (elapsed >= sensors[wichSensor].interval) {
					// Time to read sensor!!
					led.brightnessFactor = 1;
					if (getReading(wichSensor)) {
						RAMstore(wichSensor);
						sprintf(outBuff, " (RAM) + %s: %.2f %s", sensors[wichSensor].title, sensors[wichSensor].reading, sensors[wichSensor].unit);
						sckOut(PRIO_HIGH);
					}
					nextReadingTime = sensors[wichSensor].interval;
				} else {
					nextReadingTime = sensors[wichSensor].interval - elapsed;
				}
			}
		}

		// Call publish if we reached the interval
		uint32_t elapsed = rtc.getEpoch() - lastPublishTime;
		uint32_t nextPublishTime = config.publishInterval;

		if (elapsed >= config.publishInterval) {
			// Time to publish!!
			publish();
		} else {
			nextPublishTime = config.publishInterval - elapsed;
		}

		if (nextReadingTime != 0 && nextReadingTime < nextPublishTime) closestAction = nextReadingTime;
		else closestAction = nextPublishTime;
	}
}
bool SckBase::getReading(SensorType wichSensor) {

	// reading is not yet valid...
	sensors[wichSensor].valid = false;

	// Store the time we started the measurement
	uint32_t startedTime = rtc.getEpoch();

	switch (sensors[wichSensor].location) {
		
		case BOARD_BASE: {
			// If we are reading sensors from this board
			float tempReading = 0;

			switch (wichSensor) {
				case SENSOR_BATTERY: tempReading = getBatteryPercent(); break;
				// case SENSOR_VOLTIN: tempReading = getUSBvoltage(); break;
				case SENSOR_VOLTIN: tempReading = getVoltage(USB_CHAN); break;
				default: ;
			}
			sensors[wichSensor].reading = tempReading;
			sensors[wichSensor].valid = true;
			break;
		} case BOARD_URBAN: {
			sensors[wichSensor].reading = urban.getReading(wichSensor);
			if (!urban.ESR) sensors[wichSensor].valid = true;
			break;
		} case BOARD_AUX: {

			// Exception for groove oled (instead of read i will write...)
			if (wichSensor == SENSOR_GROOVE_OLED) {

				SensorType displaySensor = static_cast<SensorType>(sensorDisplayIndex);

				while (!sensors[displaySensor].enabled) {
					sensorDisplayIndex ++;
					if (sensorDisplayIndex == SENSOR_COUNT) sensorDisplayIndex = 0;
					displaySensor = static_cast<SensorType>(sensorDisplayIndex);
				}

				sensorDisplayIndex ++;
				if (sensorDisplayIndex == SENSOR_COUNT) sensorDisplayIndex = 0;

				if (displaySensor == SENSOR_GROOVE_OLED) break;

				ISOtime();
				OneSensor *thisSensor = &sensors[displaySensor];
				String Stitle = thisSensor->title;
				String Sreading = String(thisSensor->reading, 1);
				String Sunit = thisSensor->unit;
				auxBoards.displayReading(Stitle, Sreading, Sunit, ISOtimeBuff);

				sensors[wichSensor].lastReadingTime = startedTime;
				break;
			}

			// Check if the sensor is busy (and ping the sensor to continue working)
			sensors[wichSensor].busy = auxBoards.getBusyState(wichSensor);

			if (!sensors[wichSensor].busy){
				sensors[wichSensor].reading = auxBoards.getReading(wichSensor);
				sensors[wichSensor].valid = true;
			}
			
			break;
		} default: {
			;
		}
	}

	// Store last reading time
	if (sensors[wichSensor].valid) {
		sensors[wichSensor].lastReadingTime = startedTime;
		globalLastReading = startedTime;
	}

	return sensors[wichSensor].valid;
}
bool SckBase::RAMstore(SensorType wichSensor) {
	
	// If RAM space reserved for readings is full return false
	if (RAMreadingsIndex >= ram_max_readings) return false;

	// Check if this reading belongs to an already created group (in the same time frame)
	if (RAMgroupIndex >= 0 && abs(sensors[wichSensor].lastReadingTime - RAMgroups[RAMgroupIndex].time) <= maxDiffBetweenReadings) {

		// This reading in in the same time frame as the last one
		// Add to the count of this group
		RAMgroups[RAMgroupIndex].numberOfReadings++;

	} else {

		// Create new time group
		RAMgroupIndex++;
		RAMgroups[RAMgroupIndex].time = sensors[wichSensor].lastReadingTime;
		RAMgroups[RAMgroupIndex].readingStartIndex = RAMreadingsIndex + 1;
		RAMgroups[RAMgroupIndex].numberOfReadings = 1;

		// Add to the sd index (parallel count for storing sdcard readings in network mode)
		sdIndex ++;
	}

	// Store reading

	// Prepare data to be stored
	SingleSensorReading toStore;
	toStore.sensor = wichSensor;
	toStore.reading = sensors[wichSensor].reading;

	// Add one index
	RAMreadingsIndex++;
	RAMreadings[RAMreadingsIndex] = toStore;

	return true;
}
bool SckBase::RAMgetGroup(int groupIndex) {

	groupBuffer.time = RAMgroups[groupIndex].time;
	groupBuffer.numberOfReadings = RAMgroups[groupIndex].numberOfReadings;

	for (uint8_t i=0; i<RAMgroups[groupIndex].numberOfReadings; i++) {

		uint32_t thisIndex = RAMgroups[groupIndex].readingStartIndex + i;

		groupBuffer.data[i].sensor = RAMreadings[thisIndex].sensor;
		groupBuffer.data[i].reading = RAMreadings[thisIndex].reading;
	}

	return true;
}
void SckBase::publish() {

	// Check if there are some readings to publish
	if (RAMgroupIndex < 0) {
		sckOut("Can't publish... no readings yet!!!", PRIO_LOW);
		return;
	}

	if (config.mode == MODE_NET) {

		// If there is a publish running
		if (publishRuning) {
			
			// Give up on this publish try and reset to try to clear errors
			if (rtc.getEpoch() - publishStarted > publish_timeout) {
	 			sckOut("Publish is taking too much time...\n Saving to SD card and resetting!!!");
	 			publishToSD();
				softReset();
			}

			// Check for Wifi
			if (!onWifi()) {
				sckOut("Waiting for wifi for publishing...", PRIO_LOW);
				return;
			}

		} else {

			// If ESP is OFF turn it on
			if (digitalRead(POWER_WIFI)) ESPcontrol(ESP_ON);

			publishStarted = rtc.getEpoch();
			publishRuning = true;
			ESPpublish();

		}
		
	} else if (config.mode == MODE_SD) {

		publishStarted = rtc.getEpoch();
		publishRuning = true;
		publishToSD();

	}
}
bool SckBase::publishToSD() {

	if (sdIndex < 0) {
		publishRuning = false;
		return false;
	}

	sckOut("Publishing to SDcard...");

	if (openPublishFile()) {

		// Write one line for time-group
		for (uint16_t i=0; i<=sdIndex; ++i) {

			// Get data from RAM
			RAMgetGroup(i);

			// rewrite headers if there is a change on enabled sensors
			if (headersChanged != 0 && groupBuffer.time > headersChanged) {
				publishFile.close();
				openPublishFile(true);
				headersChanged = 0;
			}

	 		// Write timeStamp
	 		char tmpTime[20];
	 		epoch2iso(groupBuffer.time, tmpTime);
			publishFile.print(tmpTime);
			publishFile.print(",");

			// We have to write sensors in the same order as header
			for (uint8_t sensorIndex=0; sensorIndex<SENSOR_COUNT; sensorIndex++) {
				
				SensorType wichSensor = static_cast<SensorType>(sensorIndex);

				if (sensors[wichSensor].enabled) {

					// Write data sensor by sensor
					for (uint8_t ii=0; ii<groupBuffer.numberOfReadings; ii++) {

						// Write sensor reading
						if (groupBuffer.data[ii].sensor == wichSensor) {
							publishFile.print(groupBuffer.data[ii].reading);
							break;
						}
					}

					publishFile.print(",");
				}
			}

			// Write new line char
			publishFile.println("");

			// Console output
			sprintf(outBuff, "%s: %i sensor readings saved to SD.", tmpTime, groupBuffer.numberOfReadings);
			sckOut();

		}
		// Close file
		publishFile.close();
		
		// Restart the sd card index
		sdIndex = -1;

		// If we are on SD_MODE mode also restart the ram index (because ESPpublish wont restart it)
		if (config.mode == MODE_SD) {
			RAMgroupIndex = -1;
			RAMreadingsIndex = -1;

			// Remember at wich time we publish
			lastPublishTime = rtc.getEpoch();
		}

		publishRuning = false;
		return true;
	}

	publishRuning = false;
	sckOut(F("Cant' open publish file!!!"));
	return false;
}
bool SckBase::ESPpublish()  {
	
	sckOut("Publishing to platform...");

	// Get last readings group from RAM
	RAMgetGroup(RAMgroupIndex);

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonSensors = jsonBuffer.createObject();
	
	// Epoch time of the grouped readings
	jsonSensors["t"] = groupBuffer.time;

	for (uint8_t sensorIndex=0; sensorIndex<SENSOR_COUNT; sensorIndex++) {

		SensorType wichSensor = static_cast<SensorType>(sensorIndex);

		if (sensors[wichSensor].enabled && sensors[wichSensor].id > 0) {

			// Write data sensor by sensor
			for (uint8_t ii=0; ii<groupBuffer.numberOfReadings; ii++) {

				// Write sensor reading
				if (groupBuffer.data[ii].sensor == wichSensor) {
					jsonSensors[String(wichSensor)] = String(groupBuffer.data[ii].reading, 2);
					break;
				}
			}
		}
	}

	char tmpTime[20];
	epoch2iso(groupBuffer.time, tmpTime);
	sprintf(outBuff, "%s: %i sensor readings on the go to platform...", tmpTime, groupBuffer.numberOfReadings);
	sckOut();

	jsonSensors.printTo(msgBuff.param, 240);
	msgBuff.com = ESP_MQTT_PUBLISH_COM;
	ESPqueueMsg(true, true);

	return true;
}
bool SckBase::sdLogADC(){

	bool writeHead = false;
	char debugFileName[] = "debug.csv";

	sckOut("Started debug power!!");

	// Create a file for writing
	if (sdPresent()) {

		// Check if we are using a new file
		if (!sd.exists(debugFileName)) writeHead = true;

		// Open file
		debugLogFile = sd.open(debugFileName, FILE_WRITE);

		if (debugLogFile) sckOut("Debug file opened!!");

		// Write header
		if (writeHead) debugLogFile.println(F("chann0,chann1,charger,battvolt,battPercent,onUSB,charging"));

		debugLogFile.print(getVoltage(CHG_CHAN));
		debugLogFile.print(",");
		debugLogFile.print(getVoltage(ISET_CHAN));
		debugLogFile.print(",");
		debugLogFile.print(getVoltage(USB_CHAN));
		debugLogFile.print(",");
		debugLogFile.print(getVoltage(BATT_CHAN));
		debugLogFile.print(",");
		debugLogFile.print(getBatteryPercent());
		debugLogFile.print(",");
		debugLogFile.print(onUSB);
		debugLogFile.print(",");
		debugLogFile.print(charging);
		
		debugLogFile.close();
		sckOut("debug file written!!!");

		return true;
	}

	return false;
}


// 	--------------
// 	|	Button   |
// 	--------------
//
void SckBase::buttonEvent() {

	userLastAction = rtc.getEpoch();

	if (!digitalRead(PIN_BUTTON)) {

		urban.ESR = true;

		butLastEvent = millis();

		timerSet(ACTION_LONG_PRESS, longPressInterval);
		timerSet(ACTION_VERY_LONG_PRESS, veryLongPressInterval);

		buttonDown();

	} else {

		butLastEvent = millis();
		
		timerClear(ACTION_LONG_PRESS);
		timerClear(ACTION_VERY_LONG_PRESS);

		buttonUp();
	}

	if (onUSB && !USBDeviceAttached) {
		USBDevice.init();
		USBDevice.attach();
		USBDeviceAttached = true;
		prompt();
	}
}
void SckBase::buttonDown() {

	sckOut(F("buttonDown"), PRIO_LOW);

	switch (config.mode) {
		case MODE_FLASH: {
			softReset();
			break;

		} case MODE_SETUP: {
			changeMode(config.persistentMode);
			break;

		} case MODE_OFF: {

			wakeUp();
			break;

		} default: {
			changeMode(MODE_SETUP);
		}
	}
}
void SckBase::buttonUp() {

	sckOut(F("Button up"), PRIO_LOW);

	if (config.mode == MODE_OFF) {
		sckOut("Sleeping!!");
		timerSet(ACTION_SLEEP, 500);
	} else urban.ESR = false;
}
void SckBase::longPress() {

	// Make sure we havent released button without noticed it
	if (!digitalRead(PIN_BUTTON)) {
	
		sckOut(String F("Button long press: ") + String(millis() - butLastEvent), PRIO_MED);
		changeMode(MODE_OFF);
	
	} else buttonEvent();
}
void SckBase::veryLongPress() {

	// Make sure we havent released button without noticed it
	if (!digitalRead(PIN_BUTTON)) {
	
		sckOut(String F("Button very long press: ") + String(millis() - butLastEvent), PRIO_MED);
		factoryReset();

	} else buttonEvent();
}

// 	-----------------
// 	|	 SD card	|
// 	-----------------
//
bool SckBase::sdPresent() {

	if (sdcardAlreadySearched) return false;

	ESPcontrol(ESP_OFF);
	digitalWrite(CH_PD, LOW);		// Do this here to avoid wifi led current drain but provide sd card access

	// Make sure files are closed to avoid corruption
	closeFiles();

	if (sd.cardBegin(CS_SDCARD, SPI_HALF_SPEED)) {
		sckOut(F("Sdcard ready!!"), PRIO_LOW);
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		if (config.mode == MODE_SD) errorMode();
		sdcardAlreadySearched = true;
		return false;
	}
}
bool SckBase::openPublishFile(bool writeHeader) {

	char charFileName[publishFileName.length()];

	if (sdPresent()) {

		if (sd.begin(CS_SDCARD, SPI_HALF_SPEED)) {

			for (uint8_t fi=1; fi<99; fi++) {

				publishFileName.toCharArray(charFileName, publishFileName.length() + 1);

				// If file doesn't exist we need to write header
				if (!sd.exists(charFileName)) writeHeader = true;

				// Open file
				publishFile = sd.open(charFileName, FILE_WRITE);

				if (publishFile) {
					// Check if file is not to big
					if (publishFile.size() < FileSizeLimit) {

						// Write headers
						if (writeHeader) {
							
							sckOut("Writing headers to sdcard file.");

							// TimeStamp
							publishFile.print("Time,");

					 		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
								SensorType wichSensor = static_cast<SensorType>(i);

								if (sensors[wichSensor].enabled) {
									// Sensor titles
									publishFile.print(sensors[wichSensor].title);
									
									// If there are units cofigured
									if (String(sensors[wichSensor].unit).length() > 0) {
										publishFile.print("-");
										publishFile.print(sensors[wichSensor].unit);	
									}
									if (wichSensor < SENSOR_COUNT) publishFile.print(",");	
								}
							}
							publishFile.println("");
						}
						sckOut(String F("Using ") + publishFileName + F(" to store posts."));
						return true;
					}
					publishFile.close();
				}

				// If file dont open or too big try next
				publishFileName = String F("POST") + leadingZeros(String(fi), 3) + F(".CSV");
				
			}
		}
	}
	return false;
}
bool SckBase::openLogFile() {

	char charLogFileName[logFileName.length()];
	char charOldLogFileName[oldLogFileName.length()];

	if (sdPresent()) {
		sd.begin(CS_SDCARD);

		// Open file
		logFile = sd.open(charLogFileName, FILE_WRITE);
			
		if (logFile) {
			// If file is already on size limit
			if (logFile.size() > FileSizeLimit) {
				logFile.close();
				// If old file already exists, remove it. (We are just keeping 2 log files)
				if (sd.exists(charOldLogFileName)) sd.remove(charOldLogFileName);
				// Move file to OLD file
				sd.rename(charLogFileName, charOldLogFileName);
				// Open new file
				logFile = sd.open(charLogFileName, FILE_WRITE);
			}
			sckOut(String F("Using ") + charLogFileName + F(" to store logs."), PRIO_LOW);
			return true;
		}
	}
	return false;
}
bool SckBase::openConfigFile(bool onlyRead) {

	if (sdPresent()) {

		sd.begin(CS_SDCARD);

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
void SckBase::closeFiles() {

	if (publishFile) publishFile.close();
	if (debugLogFile) debugLogFile.close();
	if (logFile) logFile.close();
}
void SckBase::factoryReset() {

	clearWifi();
	clearToken();
	saveConfig(true);
	softReset();
}


// 	-------------------------
// 	|	Power Management    |
// 	-------------------------
//
float SckBase::getBatteryPercent() {

	uint8_t percent = 0;
	uint16_t voltage = (uint16_t)(getVoltage(BATT_CHAN) * 1000);

	for(uint8_t i = 0; i < 100; i++) {
    	if(voltage < batTable[i]) {
			percent = i;
      		break;
    	}
  	}

	// When USB is connected and the battery is not charging
	// We dont have a way to know if battery is missing or fully charged so we say 100% for both cases
	if (onUSB && !charging) percent = 100;

	// If voltage is over the largest value on table
	if (voltage >= batTable[99]) percent = 100;

	return (float)percent;
}
float SckBase::getVoltage(ADC_voltage wichVoltage) {

	byte add = 0;
	byte dir[4] = {2,4,6,8};

	switch(wichVoltage) {
		case BATT_CHAN: add = 3; break;
		case USB_CHAN: 	add = 2; break;
		case CHG_CHAN: 	add = 0; break;
		case ISET_CHAN: add = 1; break;
	}

	byte ask = B11000000 + add;

	uint32_t result = 0;

	uint8_t numberOfSamples = 5;

	// Average 5 samples
	for (uint8_t i=0; i<numberOfSamples; i++) {
		writeI2C(ADC_DIR, 0, ask);
		writeI2C(ADC_DIR, 0, ask);

		result += (readI2C(ADC_DIR, dir[add])<<4) + (readI2C(ADC_DIR, dir[add] + 1)>>4);
	}

	float resultInVoltage = 2 * (result / numberOfSamples) * VCC / RESOLUTION_ANALOG / 1000;

	return resultInVoltage;
}
void SckBase::writeCurrent(int current) {
    int resistor = (4000000/current)-96-3300;
    writeResistor(0, resistor);
}
void SckBase::updatePower() {

	if (millis() % 500 != 0) return;

	if (getVoltage(USB_CHAN) > 3.0){

		// USB is JUST connected
		if (!onUSB) {
			sckOut("USB connected!");
			USBDevice.init();
			USBDevice.attach();
			USBDeviceAttached = true;
		}
		onUSB = true;

		// Check if we are chanrging the battery
		if (getVoltage(CHG_CHAN) < 3.0) { 
			if (!charging) sckOut("Charging battery!");
			charging = true;
		} else charging = false;					// In this case battery should be full or not conected

		// Led feedback
		led.lowBatt = false;
		if (!charging) {
			led.charging = false;
			led.finishedCharging = true;
		} else {
			led.charging = true; 
			led.finishedCharging = false;
		}

	} else {

		// USB is not connected
		if (onUSB) {
			sckOut("USB disconnected!");
			USBDevice.detach();
			USBDeviceAttached = false;
		}
		onUSB = false;

		// There is no way of charging without USB
		charging = false;

		// Led feedback
		led.charging = false;

		// Make sure we are not wating power on ESP if it is not necessary
		if (config.mode != MODE_SETUP && !publishRuning && !timerExists(ACTION_RECOVER_ERROR)) ESPcontrol(ESP_OFF);

		//Turn off Serial leds
		digitalWrite(SERIAL_TX_LED, HIGH);
		digitalWrite(SERIAL_RX_LED, HIGH);

		uint8_t tmpBattPercent = (uint8_t)getBatteryPercent();

		if (tmpBattPercent < lowBattLimit) led.lowBatt = true;
		if (tmpBattPercent < lowBattEmergencySleep) {
			
			// If battery is extremely low go to sleep to keep RTC time and wakeup every minute to check if we are connected

			while (getVoltage(USB_CHAN) < 3.0) {

				// Fast triple red led flash
				for (uint8_t i=0; i < 2; ++i) {
					led.setRGBColor(led.lowRedRGB);
					delay(40);
					led.off();
					delay(40);
				}

				sleepTime = 60000;	// 60,000 ms = 1 minute
				goToSleep();
				led.off();
			}

			wakeUp();

		// Enter sleep mode
		} else if (	(closestAction > minSleepPeriod) && 						// Still some time before next action
					(rtc.getEpoch() - userLastAction > 20) && 					// At least 10 seconds after the las user action (button)
					(config.mode == MODE_SD || config.mode == MODE_NET) && 		// Only in network an sdcard modes
					!publishRuning) {											// If we are not publishing

			uint32_t NOW = rtc.getEpoch();
			uint32_t wakeupTime = NOW + closestAction - 1;					// Wakeup 1 second before next action
			if (wakeupTime - NOW > 120) wakeupTime = NOW + 120;				// Wake up at least every 2 minutes to check if something is pending

			// Sleep but wake up for a micro flash every 10 seconds
			while ((NOW < wakeupTime) && (NOW - userLastAction > 20)) {

				sleepTime = minSleepPeriod * 1000;
				goToSleep();

				if (config.mode == MODE_NET) led.setRGBColor(led.blueRGB);
				else if (config.mode == MODE_SD) led.setRGBColor(led.pinkRGB);
				delay(10);

				NOW = rtc.getEpoch();
			}

			closestAction = 0;
			wakeUp();
		}
	}
}
void SckBase::goToSleep() {

	if (sleepTime > 0) sprintf(outBuff, "Sleeping for %lu seconds", (sleepTime) / 1000);
	else sprintf(outBuff, "Sleeping forever!!! (until a button click)");
	sckOut();

	if (config.mode != MODE_OFF) changeMode(MODE_OFF);

	// Needed for avoid ESP current leak
	sdPresent();
	SPI.end();

	// Turn off ESP
	ESPcontrol(ESP_OFF);

	// ESP control pins savings
	digitalWrite(CH_PD, LOW);
	digitalWrite(GPIO0, LOW);
	digitalWrite(CS_ESP, LOW);
	digitalWrite(0, LOW);
	digitalWrite(1, LOW);

	// Put ADC to sleep
	byte payload = B00000000;						
	for (int i=0; i<4; ++i)	{
		Wire.beginTransmission(0x48);
		Wire.write(payload + i);
		Wire.endTransmission();
		delay(4);
	}
	
	// MICS heaters saving

	uint32_t nextReadingCO = sensors[SENSOR_CO].lastReadingTime + sensors[SENSOR_CO].interval;
	if (sensors[SENSOR_CO].enabled && (nextReadingCO - rtc.getEpoch() < urban.CO_PREHEATING_TIME)) {		// If next reading is in less than preheating_time turn it on
		urban.gasOn(SENSOR_CO);
	} else {
		urban.gasOff(SENSOR_CO);
	}

	uint32_t nextReadingNO2 = sensors[SENSOR_NO2].lastReadingTime + sensors[SENSOR_NO2].interval;
	if (sensors[SENSOR_NO2].enabled && (nextReadingNO2 - rtc.getEpoch() < urban.NO2_PREHEATING_TIME)) {		// If next reading is in less than 10 minutes turn it on
		urban.gasOn(SENSOR_NO2);
	} else {
		urban.gasOff(SENSOR_NO2);
	}

	// Power Suply in low power mode
	digitalWrite(PS, LOW);

	// Disconnect USB
	USBDevice.detach();
	USBDeviceAttached = false;

	uint32_t localSleepTime = sleepTime;
	sleepTime = 0;

	if (localSleepTime > 0) LowPower.deepSleep(localSleepTime);
	else LowPower.deepSleep();
}
void SckBase::wakeUp() {
	sckOut("Waked up!!!");
	changeMode(config.persistentMode);
}
void SckBase::softReset() {

	// Close files before resseting to avoid corruption
	closeFiles();

 	NVIC_SystemReset();
}


//	---------------------
// 	|	 Urban Board 	|
// 	---------------------
//
bool SckBase::urbanBoardDetected() {
	
	// Test if digital POT responds 
	float originalValue = readResistor(6);
	writeResistor(6, 1200.0);
	float compareValue = readResistor(6);
	writeResistor(6, originalValue);
	if (abs(compareValue - 1176.47) < 5) {
		return true;
	}
	return false;
}


// 	-------------
// 	|	 Led	|
// 	-------------
//
void Led::setup() {
	off();
	dir = true;
	colorIndex = 0;
	pulseMode = PULSE_STATIC;
}
void Led::update(SCKmodes newMode, uint8_t newPulseMode) {

	switch (newPulseMode) {
		case 0: {
			pulseMode = PULSE_SOFT;
			break;
		} case 1: {
			pulseMode = PULSE_HARD_SLOW;
			break;
		} case 2: {
			pulseMode = PULSE_HARD_FAST;
			break;
		}
	}

	switch (newMode) {
		case MODE_SETUP: {
			currentPulse = pulseRed;
			break;
		} case MODE_NET: {
			currentPulse = pulseBlue;
			break;
		} case MODE_SD: {
			currentPulse = pulsePink;
			break;
		} case MODE_FLASH: {
			ledRGBcolor = lightBlueRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_BRIDGE: {
			ledRGBcolor = lightGreenRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_OFF: {
			ledRGBcolor = offRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_SHELL: {
			ledRGBcolor = altBlueRGB;
			pulseMode = PULSE_STATIC;
		} default: {
			;
		}
	}

	if (pulseMode != PULSE_STATIC) configureTimer5(refreshPeriod);
	else disableTimer5();

	tick();
}
void Led::reading() {
	ledRGBcolor = whiteRGB;
	pulseMode = PULSE_STATIC;
	timerReading = millis(); //substituir esto por una libreria de timers
}
void Led::wifiOK() {
	ledRGBcolor = greenRGB;
	pulseMode = PULSE_STATIC;
}
void Led::configOK() {
	ledRGBcolor = greenRGB;
	pulseMode = PULSE_STATIC;
}
void Led::bridge() {
	ledRGBcolor = whiteRGB;
	pulseMode = PULSE_STATIC;
}
void Led::tick() {

	if(pulseMode == PULSE_SOFT) {
		
		// Soft pulse
		ledRGBcolor = *(currentPulse + colorIndex);
		if (dir) {
			colorIndex = colorIndex + 1;
			if (colorIndex > 24) {
				colorIndex = 24;
				dir = false;
			}
		} else {
			colorIndex = colorIndex - 1;
			if (colorIndex < 0) {
				colorIndex = 0;
				dir = true;
			}
		}

		if (charging) {
			if (colorIndex >= 0 && colorIndex <= 2) {
				ledRGBcolor = orangeRGB;
				brightnessFactor = 1;
			}
		} else if (finishedCharging) {
			if (colorIndex >= 0 && colorIndex <= 2) {
				ledRGBcolor = greenRGB;
				brightnessFactor = 1;
			}
		} else if (lowBatt) {
			if (colorIndex == 0 || colorIndex == 3) {
				ledRGBcolor = orangeRGB;
				brightnessFactor = 1;
			} else {
				brightnessFactor = 0;
			}
		}

	} else if (pulseMode == PULSE_HARD_SLOW) {

		if (millis() - hardTimer > slowHard) {
			hardTimer = millis();

			if (inErrorColor) ledRGBcolor = currentPulse[24];
			else ledRGBcolor = yellowRGB;

			inErrorColor = !inErrorColor;
		}

	} else if (pulseMode == PULSE_HARD_FAST) {

		if (millis() - hardTimer > fastHard) {
			hardTimer = millis();

			if (inErrorColor) ledRGBcolor = currentPulse[24];
			// else ledRGBcolor = yellowRGB;
			else ledRGBcolor = offRGB;

			inErrorColor = !inErrorColor;
		}

	}

	// Apply brightnessFactor
	ledRGBcolor.r = ledRGBcolor.r * brightnessFactor;
	ledRGBcolor.g = ledRGBcolor.g * brightnessFactor;
	ledRGBcolor.b = ledRGBcolor.b * brightnessFactor;

	setRGBColor(ledRGBcolor);
}
void Led::setRGBColor(RGBcolor myColor) {

	if (myColor.r == 0) {
		pinMode(PIN_LED_RED, OUTPUT);
		digitalWrite(PIN_LED_RED, HIGH);
	} else analogWrite(PIN_LED_RED, 255 - myColor.r);
	
	if (myColor.g == 0) {
		pinMode(PIN_LED_GREEN, OUTPUT);
		digitalWrite(PIN_LED_GREEN, HIGH);
	} else analogWrite(PIN_LED_GREEN, 255 - myColor.g);

	if (myColor.b == 0) {
		pinMode(PIN_LED_BLUE, OUTPUT);
		digitalWrite(PIN_LED_BLUE, HIGH);
	} else analogWrite(PIN_LED_BLUE, 255 - myColor.b);
}
void Led::setHSIColor(float h, float s, float i) {
	uint8_t r, g, b;

	h = fmod(h,360);
	h = 3.14159*h/(float)180;
	s = s>0?(s<1?s:1):0;
	i = i>0?(i<1?i:1):0;

	if(h < 2.09439) {
		r = 255*i/3*(1+s*cos(h)/cos(1.047196667-h));
		g = 255*i/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		b = 255*i/3*(1-s);
	} else if(h < 4.188787) {
		h = h - 2.09439;
		g = 255*i/3*(1+s*cos(h)/cos(1.047196667-h));
		b = 255*i/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		r = 255*i/3*(1-s);
	} else {
		h = h - 4.188787;
		b = 255*i/3*(1+s*cos(h)/cos(1.047196667-h));
		r = 255*i/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		g = 255*i/3*(1-s);
	}

	setRGBColor({r,g,b});
}
void Led::off() {
	disableTimer5();
	
	ledRGBcolor = offRGB;
	pulseMode = PULSE_STATIC;

	pinMode(PIN_LED_RED, OUTPUT);
	pinMode(PIN_LED_GREEN, OUTPUT);
	pinMode(PIN_LED_BLUE, OUTPUT);
	
	digitalWrite(PIN_LED_RED, HIGH);
	digitalWrite(PIN_LED_GREEN, HIGH);
	digitalWrite(PIN_LED_BLUE, HIGH);
}


// 	-----------------
// 	|	 Timers 	|
// 	-----------------
//
bool SckBase::timerRun() {

	for (uint8_t i=0; i<timerSlots; i++) {
		if (timers[i].action != ACTION_NULL) {
			if (millis() - timers[i].started > timers[i].interval) {
				
				// Check for action to execute
				switch(timers[i].action) {
					case ACTION_CLEAR_ESP_BOOTING:{
						sckOut(F("ESP ready!!!"));
						timerSet(ACTION_GET_ESP_STATUS, statusPoolingInterval, true);
						break;

					} case ACTION_ESP_ON: {
						ESPcontrol(ESP_ON);
						break;

					} case ACTION_ESP_REBOOT: {
						ESPcontrol(ESP_REBOOT);
						break;

					} case ACTION_GET_ESP_STATUS: {
						if (!readLightEnabled) {
							if (!digitalRead(POWER_WIFI)) getStatus();
						}
						break;
					
					} case ACTION_LONG_PRESS: {
						longPress();
						break;

					} case ACTION_VERY_LONG_PRESS: {
						veryLongPress();
						break;

					} case ACTION_RESET: {

						softReset();
						break;

					} case ACTION_UPDATE_SENSORS: {

						updateSensors();
						break;

					} case ACTION_DEBUG_LOG: {

						sdLogADC();
						break;

					} case ACTION_GOTO_SETUP: {

						changeMode(MODE_SETUP);
						break;

					} case ACTION_RECOVER_ERROR: {

						if (config.persistentMode == MODE_SD) {

							if (onTime && sdPresent()) {
								timerClear(ACTION_RECOVER_ERROR);
								changeMode(MODE_SD);
							}
						
						} else if (config.persistentMode == MODE_NET) {

							if (digitalRead(POWER_WIFI)) ESPcontrol(ESP_ON);

							if (onTime && onWifi() && tokenSet) {
								timerClear(ACTION_RECOVER_ERROR);
								timerClear(ACTION_START_AP_MODE);
								changeMode(MODE_NET);
							}
						}

						break;

					} case ACTION_START_AP_MODE: {

						msgBuff.com = ESP_START_AP_COM;
						ESPqueueMsg(false, false);
						break;

					} case ACTION_SAVE_SD_CONFIG: {
						
						// How much time for next publish...
						uint32_t timeToNextPublish = rtc.getEpoch() - lastPublishTime;

						// If there is no message on queue and publish time is at least 5 seconds away
						if (BUS_queueCount == 0 && timeToNextPublish > 5 && !triggerHello) {
							
							// Save sd config (esp off)
							saveSDconfig();

							// Clear timer
							timerClear(ACTION_SAVE_SD_CONFIG);
						}
						break;

					} case ACTION_MQTT_SUBSCRIBE: {

						if (onWifi()) mqttConfig(true);
						else timerSet(ACTION_MQTT_SUBSCRIBE, 1000);
						break;

					} case ACTION_RETRY_READ_SENSOR: {

						sckIn(String("read ") + String(sensors[retrySensor].title));
						break;

					} case ACTION_SLEEP :{

						goToSleep();
						break;

					} default: {
						;
					}
				}

				// Clear Timer
				if (!timers[i].periodic) {
					timers[i].action = ACTION_NULL;
					timers[i].interval = 0;
					timers[i].started = 0;
				} else {

					// Restart timer for periodic tasks
					timers[i].started = millis();
				}

				return true;
			}
		}
	}

	return false;
}
void SckBase::timerSet(TimerAction action, uint32_t interval, bool isPeriodic) {

	bool slotsFree = false;

	for (uint8_t i=0; i<timerSlots; i++) {
		if (timers[i].action == ACTION_NULL) {
			timers[i].action = action;
			timers[i].interval = interval;
			timers[i].started = millis();
			timers[i].periodic = isPeriodic;
			slotsFree = true;
			break;
		}
	}

	if (!slotsFree) {
		sckOut(F("We need more Timer slots!!!"), PRIO_HIGH);
		for (uint8_t i=0; i<timerSlots; i++) sckOut(String(timers[i].action));
	}
}
bool SckBase::timerClear(TimerAction action) {

	for (uint8_t i=0; i<timerSlots; i++) {
		if (timers[i].action == action) {
			timers[i].action = ACTION_NULL;
			timers[i].interval = 0;
			timers[i].started = 0;
			timers[i].periodic = false;
			return true;
		}
	}
	return false;
}
void SckBase::timerClearTasks(bool clearAll) {

	for (uint8_t i=0; i<timerSlots; i++) {

		if ((timers[i].action != ACTION_LONG_PRESS &&
			timers[i].action != ACTION_VERY_LONG_PRESS && 
			timers[i].action != ACTION_GET_ESP_STATUS && 
			timers[i].action != ACTION_RECOVER_ERROR &&
			timers[i].action != ACTION_SAVE_SD_CONFIG) || clearAll) {

				timers[i].action = ACTION_NULL;
				timers[i].interval = 0;
				timers[i].started = 0;
				timers[i].periodic = false;
		}
	}
}
bool SckBase::timerExists(TimerAction action) {
	for (uint8_t i=0; i<timerSlots; i++) {
		if (timers[i].action == action) {
			return true;
		}
	}
	return false;
}


// 	--------------------
// 	|	 POT control   |
// 	--------------------
//
void SckBase::writeResistor(byte resistor, float value ) {
   byte POT = POT1;
   byte ADDR = resistor;
   int data=0x00;
   if (value>100000) value = 100000;
   data = (int)(value/ohmsPerStep);
   if ((resistor==2)||(resistor==3))
     {
       POT = POT2;
       ADDR = resistor - 2;
     }
   else if ((resistor==4)||(resistor==5))
     {
       POT = POT3;
       ADDR = resistor - 4;
     }
   else if ((resistor==6)||(resistor==7))
     {
       POT = POT4;
       ADDR = resistor - 6;
     }
   writeI2C(POT, ADDR, data);
}
float SckBase::readResistor(byte resistor) {
   byte POT = POT1;
   byte ADDR = resistor;
   if ((resistor==2)||(resistor==3))
     {
       POT = POT2;
       ADDR = resistor - 2;
     }
   else if ((resistor==4)||(resistor==5))
     {
       POT = POT3;
       ADDR = resistor - 4;
     }
   else if ((resistor==6)||(resistor==7))
     {
       POT = POT4;
       ADDR = resistor - 6;
     }
   return readI2C(POT, ADDR)*ohmsPerStep;
}
void SckBase::writeI2C(byte deviceaddress, byte address, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
  delay(4);
}
byte SckBase::readI2C(int deviceaddress, byte address) {
  Wire.beginTransmission(deviceaddress);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available() != 1) return 0x00;
  byte data = Wire.read();
  return data;
}  


// 	-------------------------
// 	|	 Utility functions 	|
// 	-------------------------
//
String leadingZeros(String original, int decimalNumber) {
	for (uint8_t i=0; i < (decimalNumber - original.length()); ++i)	{
		original = "0" + original;
	}
	return original;
}
uint8_t countMatchedWords(String baseString, String input) {
	
	uint8_t foundedCount = 0;
	String word;
	
	while (input.length() > 0) {

		// Get next word
		if (input.indexOf(" ") > -1) word = input.substring(0, input.indexOf(" "));
		// Or there is only one left
		else word = input;

		// If we found one
		if (baseString.indexOf(word) > -1) foundedCount += 1;
		// If next word is not part of the title we asume the rest of the input is a command or something else
		else break;

		// remove what we tested
		input.replace(word, "");
		input.trim();
	}

	return foundedCount;
}
String cleanInput(String toRemove, String original) {

	String word;
	String checking = original;
	bool finished = false;

	while (!finished) {
		// word to check
		if (checking.indexOf(" ") > -1)  {
			word = checking.substring(0, checking.indexOf(" ")+1);
		} else {
			word = checking;
			finished = true;
		}

		String wordNoSpaces = word;
		wordNoSpaces.trim();

		if (toRemove.indexOf(wordNoSpaces) > -1) original.replace(word, "");
		else finished = true;
			
		checking.replace(word, "");
	}
	return original;
}
extern "C" char *sbrk(int i);
size_t freeRAM(void) {
	char stack_dummy = 0;
	return(&stack_dummy - sbrk(0));
}
