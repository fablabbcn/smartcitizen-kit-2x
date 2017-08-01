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

	// Serial Ports Configuration
	Serial1.begin(baudrate);
	SerialUSB.begin(baudrate);

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
	pinMode(IO0, OUTPUT);	// PA7 -- CO Sensor Heather
	pinMode(IO1, OUTPUT);	// PA6 -- NO2 Sensor Heater
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
	attachInterrupt(PIN_BUTTON, ISR_button, CHANGE);

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

			sprintf(outBuff, "Detecting: %s", sensors[wichSensor].title);
			sckOut();
			
			if (auxBoards.begin(wichSensor)) {

				if (!sensors[wichSensor].enabled) {
					sprintf(outBuff, "Found %s!!!", sensors[wichSensor].title);
					sckOut();
					enableSensor(wichSensor);
				} else {
					sprintf(outBuff, "Found %s, already enabled!!!", sensors[wichSensor].title);
					sckOut();
				} 

			} else {
				if (sensors[wichSensor].enabled) {
					sprintf(outBuff, "Cant find %s!!!", sensors[wichSensor].title);
					sckOut();
					disableSensor(wichSensor);
				}
			}
		}
	}

	// Check if USB connected and charging status
	updatePower();
	timerSet(ACTION_UPDATE_POWER, 500, true);
};
void SckBase::update() {

	// Flash and bridge modes
	if (config.mode == MODE_FLASH || config.mode == MODE_BRIDGE){
		if (SerialUSB.available()) {
			char buff = SerialUSB.read();
			serialBuff += buff;
			if (serialBuff.length() > 4) serialBuff.remove(0);
			if (serialBuff.startsWith("Bye")) softReset();
			Serial1.write(buff);
		}
		if (Serial1.available()) SerialUSB.write(Serial1.read());
	} else {

		// update ESP communications
		if (!digitalRead(POWER_WIFI)) ESPbusUpdate();

		// Update timers
		timerRun();

		// Check Serial ports inputs
		inputUpdate();

		//----------------------------------------
		// 	MODE_SETUP
		//----------------------------------------
		if (config.mode == MODE_SETUP) {
			// TODO led feedback on CRC OK, wifi OK, ping OK, and MQTT OK
			if (readLightEnabled) {
				lightResults = readLight.read();
				if (lightResults.ok) {
					
					if (lightResults.lines[0].endsWith(F("wifi")) || lightResults.lines[0].endsWith(F("auth"))) {
						if (lightResults.lines[1].length() > 0) {
							lightResults.lines[1].toCharArray(credentials.ssid, 64);
							lightResults.lines[2].toCharArray(credentials.password, 64);
							credentials.time = rtc.getEpoch();
							sendNetwork();
						}
					}
					if (lightResults.lines[0].endsWith(F("auth"))) {
						if (lightResults.lines[3].length() > 0) {
							lightResults.lines[3].toCharArray(token, 7);
							sendToken();
						}
						if (lightResults.lines[4].toInt() > 0 && lightResults.lines[4].toInt() < ONE_DAY_IN_SECONDS) {
							config.publishInterval = lightResults.lines[4].toInt();
							sckOut(String F("New reading interval: ") + String(config.publishInterval));
						}
					}
					if (lightResults.lines[0].endsWith(F("time"))) {
						setTime(lightResults.lines[1]);
					}
					led.configOK();
				 	readLight.reset();
				 	readLightEnabled = false;

				 	//MQTT Hellow for Onboarding process
				 	if (onWifi) {
				 		msgBuff.com = ESP_MQTT_HELLOW_COM;
						ESPqueueMsg(false, false);
					} else {
						// If we are not yet connected queue the MQTT hello
						triggerHello = true;
					}
				}
			}
		} 
	}
}

void SckBase::sendNetwork() {

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["ssid"] = credentials.ssid;
	jsonNet["pass"] = credentials.password;
	jsonNet.printTo(msgBuff.param, 240);
	msgBuff.com = ESP_SET_WIFI_COM;
	sckOut(String F("Sending wifi settings to ESP: ") + String(msgBuff.param), PRIO_LOW);
	ESPqueueMsg(true, false);
}

void SckBase::clearNetworks() {
	
	sckOut(F("Clearing networks..."));
	msgBuff.com = ESP_CLEAR_WIFI_COM;
	ESPqueueMsg(false, true);
}

void SckBase::sendToken() {
	strncpy(msgBuff.param, token, 64);
	msgBuff.com = ESP_SET_TOKEN_COM;
	ESPqueueMsg(true, true);
}

void SckBase::clearToken() {

	sckOut(F("Clearing token..."));
	msgBuff.com = ESP_CLEAR_TOKEN_COM;
	ESPqueueMsg(false, true);
}

void SckBase::changeMode(SCKmodes newMode) {

	// Start with a new clear state
	// --------------------------------------

	// Clear ALL timers
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

			// Start ESP and ap mode
			if (digitalRead(POWER_WIFI)) timerSet(ACTION_ESP_ON, 200);
			
			// Restart lightread for receiving new data
			readLightEnabled = true;
			readLight.reset();
			lightResults.commited = false;
			break;

		} case MODE_NET: {

			// If we dont have wifi turn off esp (it will be turned on by next publish try)
			// TODO if we are not on battery esp will be always on
			ESPcontrol(ESP_OFF);

			sckOut("Entering Network mode!");

			timerSet(ACTION_UPDATE_SENSORS, 1000, true);
			sckOut(String F("Publishing every ") + String(config.publishInterval) + F(" seconds"));
			break;

		} case MODE_SD: {

			ESPcontrol(ESP_OFF);

			sckOut("Entering SD card mode!");

			timerSet(ACTION_UPDATE_SENSORS, 1000, true);
			sckOut(String F("Publishing every ") + String(config.publishInterval) + " seconds");
			break;

		} case MODE_BRIDGE: {
			ESPcontrol(ESP_ON);
			changeOutputLevel(OUT_SILENT);
			break;

		} case MODE_FLASH: {
			changeOutputLevel(OUT_SILENT);
			ESPcontrol(ESP_FLASH);
			break;
		} default: {
			;
		}
	}

	// Keep previous mode
	if (config.mode == MODE_NET || config.mode == MODE_SD) prevMode = config.mode;

	// Set new mode
	config.mode = newMode;

	// Save new mode to eeprom to recover it after reset.
	if (config.mode == MODE_OFF || config.mode == MODE_SD || config.mode == MODE_NET) saveConfig();

	// Update led
	led.update(newMode, 0);

	// After reset it will go to sleep in a clean state
	if (newMode == MODE_OFF) goToSleep();
}

void SckBase::errorMode() {
	
	// Clear timer for updating sensors 
	timerClear(ACTION_UPDATE_SENSORS);

	// Try to recover error every n milliseconds
	if (!timerExists(ACTION_RECOVER_ERROR)) timerSet(ACTION_RECOVER_ERROR, 500, true);

	// Give led feedback about the error
	led.update(config.mode, 2);

	// Change to setup mode in n seconds
	if (!timerExists(ACTION_GOTO_SETUP)) timerSet(ACTION_GOTO_SETUP, 5000);

	// Start ESP to try time Sync
	// if (digitalRead(POWER_WIFI)) timerSet(ACTION_ESP_ON, 200);
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
				sckOut("Turning off ESP...");
				timerClear(ACTION_GET_ESP_STATUS);
				BUS_queueCount = 0;
				onWifi = false;
				espSerialDebug = false;
				digitalWrite(POWER_WIFI, HIGH);		// Turn off ESP
				digitalWrite(GPIO0, LOW);
				espTotalOnTime += millis() - espLastOn;
				sckOut(String F("ESP was on for ") + String(millis() - espLastOn, 0) + F(" milliseconds."));

				espStatus.wifi = ESP_NULL;
				espStatus.net = ESP_NULL;
				espStatus.mqtt = ESP_NULL;
				espStatus.time = ESP_NULL;
				espStatus.ap = ESP_NULL;
				espStatus.web = ESP_NULL;
				espStatus.conf = ESP_NULL;
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

	// If there is something the ESP want to tell to us
	if (BUS_in.receiveData()) ESPprocessMsg();

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
			if (!mesgReceived) sckOut("*", PRIO_MED, false);
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
}

void SckBase::ESPqueueMsg(bool sendParam, bool waitAnswer) {

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

			int comparison = strcmp(credentials.ssid, jsonNet["s"]);
			if (comparison == 0) {
				String tssid = jsonNet["s"];
				String tpass = jsonNet["p"];
				sckOut(String F("Added network: ") + tssid + F(" - ") + tpass);
				prompt();
			} else {
				sckOut(F("Failed to add network!!!"));
			}
			break;

		} case ESP_CLEAR_WIFI_COM: {

			sckOut(F("Wifi networks deleted!!!"));
			break;

		} case ESP_GET_WIFI_COM: {
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			uint8_t number = jsonNet["n"];
			String tssid = jsonNet["s"];
			String tpass = jsonNet["p"];
			if (tssid.equals("none")) {
				sckOut(F("No network configured!!"));
			} else {
				sckOut(String(number+1) + F(": ") + tssid + F(" - ") + tpass);	
			}
			prompt();
			break;

		} case ESP_GET_BEST_WIFI_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			uint8_t number = jsonNet["n"];
			String tssid = jsonNet["s"];
			String tpass = jsonNet["p"];
			sckOut(String(number+1) + F(": ") + tssid + F(" - ") + tpass);
			prompt();
			break;

		} case ESP_GET_IP_COM: {
			sckOut(String(msgIn.param));
			break;

		} case ESP_SET_TOKEN_COM: {
			int comparison = strcmp(token, msgIn.param);
			if (comparison == 0) {
				sckOut(String F("New token: ") + String(token));
				prompt();
			} else {
				sckOut(F("Failed to set token!!!"));
			}
			break;

		} case ESP_GET_TOKEN_COM: {
			strncpy(token, msgIn.param, 8);
			sckOut(String(token));
			prompt();
			break;

		} case ESP_GET_CONF_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonConf = jsonBuffer.parseObject(msgIn.param);
			
			config.publishInterval = jsonConf["ri"];
			// TODO agregar mode change
			// config.persistentMode = config.mode;

			saveConfig();
			
			sckOut(F("Configuration updated:"));
			sckOut(String F("Reading interval: ") + String(config.publishInterval));
			prompt();
			break;

		} case ESP_GET_APCOUNT_COM: {

			break;

		} case ESP_GET_VERSION_COM: {

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonVer = jsonBuffer.parseObject(msgIn.param);
			String ESPv = jsonVer["ver"];
			ESPversion = ESPv;
			String ESPbd = jsonVer["date"];
			ESPbuildDate = ESPbd;
			sckOut(String F("ESP version:\t\t") + ESPversion + F(" (") + ESPbuildDate + F(")"));
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
			sckOut(F("ESP sensor readings updated!!!"));
			break;

		} case ESP_MQTT_CLEAR_STATUS :{
			sckOut(F("ESP MQTT status cleared!!"), PRIO_LOW);
			break;

		} case ESP_MQTT_HELLOW_COM: {
			sckOut(F("ESP trying MQTT hellow..."));
			break;

		} case ESP_GET_FREE_HEAP_COM:{

			sckOut(String F("ESP free heap: ") + String(msgIn.param));
			break;

		} case ESP_WEB_CONFIG_SUCCESS: {

			sckOut(F("Configuration changed via WebServer!!!"));
			// led.configOK();
		 	readLightEnabled = false;
		 	readLight.reset();

		 	//MQTT Hellow for Onboarding process
			triggerHello = true;
			break;

		}
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

	espStatus.wifi 	= msgIn.param[0];
	espStatus.net	= msgIn.param[1];
	espStatus.mqtt	= msgIn.param[2];
	espStatus.time	= msgIn.param[3];
	espStatus.ap	= msgIn.param[4];
	espStatus.web	= msgIn.param[5];
	espStatus.conf	= msgIn.param[6];

	// Wifi status has changed
	if (espStatus.wifi != prevEspStatus.wifi) {
		switch (espStatus.wifi) {
			case ESP_WIFI_CONNECTED_EVENT: {
				sckOut(F("Conected to wifi!!"));

				// Feedback
				if (config.mode != MODE_SETUP) led.update(config.mode, 0);

				onWifi = true;

				// Send MQTT Hello
				if (triggerHello) {
					sckOut(F("Sending MQTT Hello..."));
					msgBuff.com = ESP_MQTT_HELLOW_COM;
					ESPqueueMsg(false, true);
					triggerHello = false;
				}

				// Forced Time sync
				if (!rtc.isConfigured() || rtc.getYear() < 17) {
					sckOut(F("OUT OF TIME, Asking time to ESP..."));
					msgBuff.com = ESP_GET_TIME_COM;
					ESPqueueMsg(false, true);
				}

				if (ESPpublishPending) {
				// If there is a publish operation waiting...
				 	publish();
				}

				break;

			} case ESP_WIFI_ERROR_EVENT: {
				sckOut(F("Wifi ERROR: undefined!!"));
				onWifi = false;
				break;

			} case ESP_WIFI_ERROR_PASS_EVENT: {
				sckOut(F("Wifi ERROR: wrong password!!"));
				onWifi = false;
				break;

			} case ESP_WIFI_ERROR_AP_EVENT: {
				sckOut(F("Wifi ERROR: can't find access point!!"));
				onWifi = false;
				break;

			} 
		} 

		// If there was ANY wifi error...
		if (!onWifi) {

			// If we are not expecting setup mode configuration turn ESP off to avoid battery drain
			if(config.mode != MODE_SETUP && !timerExists(ACTION_GOTO_SETUP)) ESPcontrol(ESP_OFF);

			// If we NEED network give feedback about error
			// if (config.mode == MODE_NET) led.update(config.mode, 2);

			// If there is a pending publish
			// if (ESPpublishPending) {
			// 	ESPcontrol(ESP_OFF);
			// 	sckOut(F("ERROR: publish failed, saving to SDcard only..."));
			// 	publishToSD();
			// 	ESPpublishPending = false;
			// }
		}
	}

	// Net status has changed
	if (espStatus.net != prevEspStatus.net) {

	}

	// Mqtt status has changed
	if (espStatus.mqtt != prevEspStatus.mqtt) {

		switch (espStatus.mqtt) {
			case ESP_MQTT_PUBLISH_OK_EVENT: {
				sckOut(F("MQTT publish OK!!"));

				// Start dimming the led...
				// if (!onUSB) led.dim = true;

				// ESPcontrol(ESP_OFF);
				// ESPpublishPending = false;
				// publishToSD();
				break;

			} case ESP_MQTT_HELLO_OK_EVENT: {
				sckOut(F("MQTT Hello OK!!"));

				// Go to network mode
				if(config.mode != MODE_NET && triggerHello) changeMode(MODE_NET);
				break;

			} case ESP_MQTT_ERROR_EVENT: {
				if (config.mode == MODE_NET) led.update(config.mode, 2);
				sckOut(F("ERROR: MQTT failed!!"));
				// ESPcontrol(ESP_OFF);
				// publishToSD();
				break;
			}
		}

		if (espStatus.mqtt != ESP_NULL) {
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

			}
		}
	}

	// Mode status has changed
	if (espStatus.ap != prevEspStatus.ap) {
		switch (espStatus.ap) {
			case ESP_AP_ON_EVENT: {
				sckOut(F("Started APmode"));
				break;

			} case ESP_AP_OFF_EVENT: {
				sckOut(F("Stoped APmode"));
				break;
			}
		}
	}

	// WebServer status has changed
	if (espStatus.web != prevEspStatus.web) {
		switch (espStatus.web) {
			case ESP_WEB_ON_EVENT: {
				sckOut(F("Web server started"));
				break;

			} case ESP_WEB_OFF_EVENT: {
				sckOut(F("Web server stoped"));
				break;

			}
		}
	}

	// Conf status has changed
	if (espStatus.conf != prevEspStatus.conf) {
		if (espStatus.conf == ESP_CONF_CHANGED_EVENT) {

			sckOut(F("Configuration on ESP has changed!!!"), PRIO_LOW);
			msgBuff.com = ESP_GET_CONF_COM;
			ESPqueueMsg(false, false);
			
		}
	}

	prevEspStatus = espStatus;
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

		} case EXTCOM_ESP_SERIAL_DEBUG_ON: {
			if (!digitalRead(POWER_WIFI)) {
				msgBuff.com = ESP_SERIAL_DEBUG_ON;
				espSerialDebug = true;
				ESPqueueMsg(false);
			} else {
				sckOut(F("Please start ESP first!"));
			}
			break;

		} case EXTCOM_ESP_SERIAL_DEBUG_OFF: {
			if (!digitalRead(POWER_WIFI)) {
				msgBuff.com = ESP_SERIAL_DEBUG_OFF;
				espSerialDebug = false;
				ESPqueueMsg(false);
			} else {
				sckOut(F("Please start ESP first!"));	
			}
			break;

		} case EXTCOM_ESP_MQTT_HELLO: {

			msgBuff.com = ESP_MQTT_HELLOW_COM;
			ESPqueueMsg(false, true);
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

			String newSsid = strIn.substring(first + 1, second);
			String newPass = strIn.substring(third + 1, fourth);

			newSsid.toCharArray(credentials.ssid, 64);
			newPass.toCharArray(credentials.password, 64);
			sendNetwork();

			break;

		} case EXTCOM_GET_WIFI: {
			msgBuff.com = ESP_GET_WIFI_COM;
			ESPqueueMsg(false, false);
			break;

		} case EXTCOM_GET_BEST_WIFI: {
			msgBuff.com = ESP_GET_BEST_WIFI_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_CLEAR_WIFI: {
			clearNetworks();
			break;

		} case EXTCOM_GET_IP: {
			msgBuff.com = ESP_GET_IP_COM;
			ESPqueueMsg(false, false);
			break;

		} case EXTCOM_SET_TOKEN: {
			if (strIn.length() != 6) {
				sckOut(F("Token should have 6 characters!!!"), PRIO_HIGH);
			} else {
				strIn.toCharArray(token, 64);
				sendToken();
			}
			break;
		
		} case EXTCOM_GET_TOKEN: {
			msgBuff.com = ESP_GET_TOKEN_COM;
			ESPqueueMsg(false, true);
			break;

		} case EXTCOM_CLEAR_TOKEN: {
			clearToken();
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

				sckOut(String F("Saving factory defaults, reseting in 5 seconds ..."), PRIO_HIGH);
				saveConfig(true);
				timerSet(ACTION_RESET, 5000);

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
						config.persistentMode = requestedMode;
						changeMode(requestedMode);
						break;
					}
				}
				if (requestedMode == MODE_COUNT) sckOut(F("Unrecognized mode, please try again!"));

			}
			break;

		} case EXTCOM_GET_CONFIG: {
			sckOut(String F("Current mode: ") + modeTitles[config.persistentMode] + F(" mode"));
			// sckOut(String F("Persistent mode: ") + modeTitles[config.persistentMode] + F(" mode"));
			sckOut(String F("Publish interval: ") + config.publishInterval);

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

		} case EXTCOM_READLIGHT_TOGGLE_DEBUG:{

			readLight.debugFlag = !readLight.debugFlag;
			sckOut(F("Readlight debug flag: "), PRIO_MED, false);
			if (readLight.debugFlag) sckOut(F("true"));
			else sckOut(F("false"));
			break;

		// Time configuration
		} case EXTCOM_SET_TIME: {
			setTime(strIn);
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
					sckOut("Failed getting reading!!!");
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

			sprintf(outBuff,   "CHG:  %u mV\r\nISET:  %u mV\r\nVUSB:  %.0f mV\r\nBatt volt:  %i mV\r\nBatt:  %.0f %%\r\nOn USB:  %i\r\nCharging:  %i",
								getCHG(),
								getISET(),
								getVUSB(),
								getBatteryVoltage(),
								getBatteryPercent(),
								onUSB,
								charging
					);
			sckOut();
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

			sckOut(F("\nEnabled"));
			sckOut(F("----------"));
			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				thisType = static_cast<SensorType>(i);

				if (sensors[thisType].enabled) sckOut(String(sensors[thisType].title) + " (" + String(sensors[thisType].interval) + " sec)");
			}

			sckOut(F("\nDisabled"));
			sckOut(F("----------"));
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

	if (onUSB) {
		if (outputLevel + priority > 1) {
			SerialUSB.print(strOut);
			if (newLine) SerialUSB.println();
		}
	}

	// TODO FIX THIS algo no le gusta y no inicia
	// if (priority == PRIO_HIGH || priority == PRIO_MED) {
		// if (openLogFile()) {
			// logFile.print(strOut);
			// if (newLine) logFile.println();
			// logFile.close();
		// }
	// }
}

void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine) {

	if (onUSB) {
		if (outputLevel + priority > 1) {
			SerialUSB.print(strOut);
			if (newLine) SerialUSB.println();
		}
	}
}

void SckBase::sckOut(PrioLevels priority, bool newLine) {
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



/* 	-------------
 	|	Timer   |
 	-------------
*/
bool SckBase::setTime(String epoch) {
	// validate time here!!!
	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		sckOut("RTC updated!!!");
		onTime = true;
		ISOtime();
		sckOut(ISOtimeBuff);
		prompt();
		if (config.mode != MODE_SETUP) changeMode(config.mode);
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

/* 	---------------
 	|	Sensors   |
 	---------------
*/
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

	// TODO force publish before storage is full...

	// Don't take readings if RTC is not updated
	if (!onTime) {
		errorMode();
		return;
	}

	// Only update sensor readings if we are in network or sdcard modes
	if (config.mode == MODE_NET || config.mode == MODE_SD) {

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			if (sensors[wichSensor].enabled) {
				if (rtc.getEpoch() - sensors[wichSensor].lastReadingTime >= sensors[wichSensor].interval) {
					led.dim = false;
					led.brightnessFactor = 1;
					sprintf(outBuff, "%s: ", sensors[wichSensor].title);
					sckOut(PRIO_HIGH, false);
					getReading(wichSensor);
					RAMstore(wichSensor);
					sprintf(outBuff, "%.2f %s", sensors[wichSensor].reading, sensors[wichSensor].unit);
					sckOut(PRIO_HIGH);
				}
			}
		}

		if (rtc.getEpoch() - lastPublishTime > config.publishInterval) publish();

		// if (!onUSB) led.dim = true;
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
				case SENSOR_VOLTIN: tempReading = getVUSB(); break;
				default: ;
			}
			sensors[wichSensor].reading = tempReading;
			sensors[wichSensor].valid = true;
			break;
		} case BOARD_URBAN: {
			sensors[wichSensor].reading = urban.getReading(wichSensor);
			sensors[wichSensor].valid = true;
			break;
		} case BOARD_AUX: {
			sensors[wichSensor].reading = auxBoards.getReading(wichSensor);
			sensors[wichSensor].valid = true;
			break;
		} default: {
			;
		}
	}

	// Store last reading time
	if (sensors[wichSensor].valid) sensors[wichSensor].lastReadingTime = startedTime;

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

	if (RAMreadingsIndex < 0 || groupIndex > RAMgroupIndex) return false;

	groupBuffer.time = RAMgroups[groupIndex].time;
	groupBuffer.numberOfReadings = RAMgroups[groupIndex].numberOfReadings;

	for (uint8_t i=0; i<RAMgroups[groupIndex].numberOfReadings; i++) {

		uint32_t thisIndex = RAMgroups[groupIndex].readingStartIndex + i;

		groupBuffer.data[i].sensor = RAMreadings[thisIndex].sensor;
		groupBuffer.data[i].reading = RAMreadings[thisIndex].reading;
	}

	return true;
}

bool SckBase::publish() {

	if (config.mode == MODE_NET && ESPpublishPending) return false;
	if (RAMreadingsIndex < 0) return false;

	sckOut("Starting publish...");

	// Turn on led
	led.brightnessFactor = 1;
	led.dim = false;

	if (config.mode == MODE_NET) {

		// Start Wifi
		ESPcontrol(ESP_ON);
		if (onWifi) {
			if (ESPpublish()) {
				publishToSD();
				return true;
			}
		} else {
			ESPpublishPending = true;
		}
		
	} else if (config.mode == MODE_SD) {

		if (publishToSD()) {
			return true;
		}
	}

	return false;
}

bool SckBase::publishToSD() {

	sckOut("Publishing to SDcard...");

	if (openPublishFile()) {

		// Write one line for time-group
		for (uint16_t i=0; i<=RAMgroupIndex; ++i) {

			// Get data from RAM
			RAMgetGroup(i);

			// rewrite headers if there is a change on enabled sensors
			if(headersChanged != 0 && groupBuffer.time > headersChanged) {
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
		}
		// Close file
		publishFile.close();
		lastPublishTime = rtc.getEpoch();
		// if (!onUSB) led.dim = true;
		
		// Clear RAM
		RAMgroupIndex = -1;
		RAMreadingsIndex = -1;

		sckOut(F("Readings saved to SD!!"));

		return true;
	}

	sckOut(F("ERROR: Cant' open publish file!!!"));
	lastPublishTime = rtc.getEpoch();
	return false;
}

bool SckBase::ESPpublish()  {
	
	sckOut("Publishing to platform...");

	// for (uint16_t i=0; i<=RAMgroupIndex; ++i) {


	// }

	return true;
}

// void SckBase::ESPpublish() {

// 	// Prepare json for sending
// 	StaticJsonBuffer<240> jsonBuffer;
// 	JsonObject& jsonSensors = jsonBuffer.createObject();

// 	// TODO Falta el time que ya no es sensor!!!!!
// 	jsonSensors["t"] = double_with_n_digits(sensors[wichSensor].reading, 2);

// 	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

// 		SensorType wichSensor = static_cast<SensorType>(i);
		
// 		// Only send enabled sensors
// 		if (sensors[wichSensor].enabled) {

// 			jsonSensors[String(i)] = double_with_n_digits(sensors[wichSensor].reading, 2);
// 		}
// 	}

// 	jsonSensors.printTo(msgBuff.param, 240);

// 	msgBuff.com = ESP_MQTT_PUBLISH_COM;
// 	sckOut(String F("Sending readings to ESP..."));
// 	sckOut((msgBuff.param), PRIO_LOW);

// 	ESPqueueMsg(true, true);
// }

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

		debugLogFile.print(getCHG());
		debugLogFile.print(",");
		debugLogFile.print(getISET());
		debugLogFile.print(",");
		debugLogFile.print(getVUSB());
		debugLogFile.print(",");
		debugLogFile.print(getBatteryVoltage());
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

void SckBase::saveConfig(bool factory) {

	Configuration toSaveConfig;

	if (!factory) {
		
		toSaveConfig.valid = true;
		toSaveConfig.mode = config.mode;
		toSaveConfig.persistentMode = config.persistentMode;
		toSaveConfig.publishInterval = config.publishInterval;

		// Save per sensor config
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			toSaveConfig.sensor[wichSensor].enabled = sensors[wichSensor].enabled;
			toSaveConfig.sensor[wichSensor].interval = sensors[wichSensor].interval;
		}

	} else {

		// Default values
		toSaveConfig.valid = true;
		toSaveConfig.mode = default_mode;
		toSaveConfig.persistentMode = MODE_SD;
		toSaveConfig.publishInterval = default_publish_interval;

		// Default sensor values
		AllSensors tempSensors;

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			toSaveConfig.sensor[wichSensor].enabled = tempSensors[wichSensor].enabled;
			toSaveConfig.sensor[wichSensor].interval = default_sensor_reading_interval;
		}

	}

	// Save to eeprom
	eepromConfig.write(toSaveConfig);

	// Remove old sdcard config
	sd.remove(configFileName);

	// Save to sdcard
	if (openConfigFile()) {

		char lineBuff[128];

		configFile.println("# -------------------\r\n# General configuration\r\n# -------------------");
		// configFile.println("\r\n# mode:sdcard, network, setup");
		// sprintf(lineBuff, "mode:%s", modeTitles[toSaveConfig.mode]);
		// configFile.println(lineBuff);
		configFile.println("\r\n# mode:sdcard or network");
		sprintf(lineBuff, "mode:%s", modeTitles[toSaveConfig.persistentMode]);
		configFile.println(lineBuff);
		configFile.println("\r\n# publishInterval:period in seconds");
		sprintf(lineBuff, "publishInterval:%lu", toSaveConfig.publishInterval);
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
	}
}

void SckBase::loadConfig() {

	Configuration savedConf = eepromConfig.read();

	if (savedConf.valid) {
		
		config.publishInterval = savedConf.publishInterval;
		config.persistentMode = savedConf.persistentMode;
		config.mode = savedConf.mode;

		// Load per sensor config
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			sensors[wichSensor].enabled = savedConf.sensor[wichSensor].enabled;
			sensors[wichSensor].interval = savedConf.sensor[wichSensor].interval;
		}

		// Load sdcard config
		loadSDconfig();

	} else {

		// If there is no sdcard valid config turn to factory defaults
		if (!loadSDconfig()) {
			sckOut("Error loading config.txt!!!");
			saveConfig(true);
			loadConfig();
		} else {
			saveConfig();
		}
	}
	changeMode(config.persistentMode);
}

bool SckBase::loadSDconfig() {

	// Open file only for reading
	if (openConfigFile(true)) {

		sckOut("Loading config file from sdcard!");

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
			

			// Ignore comments
			if (!lineBuff.startsWith("#") && lineBuff.length() > 0) {

				// Get mode configuration
				if (lineBuff.startsWith("mode:")) {
					lineBuff.replace("mode:", "");
					for (uint8_t i=0; i<MODE_COUNT; i++) {
						SCKmodes wichMode = static_cast<SCKmodes>(i);
						if (lineBuff.equals(modeTitles[wichMode])) {
							if (wichMode != config.persistentMode) {
								sprintf(outBuff, "config.txt mode: %s", modeTitles[wichMode]);
								sckOut();
								manualConfigDetected = true;
								config.persistentMode = wichMode;
							}
						}
					}

				// Get persistent mode configuration
				// } else if (lineBuff.startsWith("persistentMode:")) {
				// 	lineBuff.replace("persistentMode:", "");
				// 	if (lineBuff.equals(modeTitles[MODE_SD])) {
				// 		if (config.persistentMode != MODE_SD) {
				// 			sckOut("config.txt persistent mode: sdcard");
				// 			manualConfigDetected = true;
				// 			config.persistentMode = MODE_SD;	
				// 		}
				// 	} else if (lineBuff.equals(modeTitles[MODE_NET])) {
				// 		if (config.persistentMode != MODE_NET) {
				// 			sckOut("config.txt persistent mode: network");
				// 			manualConfigDetected = true;
				// 			config.persistentMode = MODE_NET;
				// 		}
				// 	}
				
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
						} else if (newInterval > minimal_sensor_reading_interval && newInterval < max_sensor_reading_interval) {
							
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

	} else return false;

	return true;
}

/* 	--------------
 	|	Button   |
 	--------------
*/
void SckBase::buttonEvent() {

	// if (millis() - butLastEvent < 30) return;

	if (!digitalRead(PIN_BUTTON)) {

		butIsDown = true;
		led.dim = false;
		butLastEvent = millis();

		timerSet(ACTION_LONG_PRESS, longPressInterval);
		timerSet(ACTION_VERY_LONG_PRESS, veryLongPressInterval);

		buttonDown();

	} else {

		butIsDown = false;
		// if (config.mode == MODE_NET || config.mode == MODE_SD && !onUSB) led.dim = true;
		butLastEvent = millis();		
		
		timerClear(ACTION_LONG_PRESS);
		timerClear(ACTION_VERY_LONG_PRESS);

		buttonUp();
	}
}

void SckBase::buttonDown() {

	sckOut(F("buttonDown"), PRIO_LOW);

	switch (config.mode) {
		case MODE_OFF: {
			wakeUp();
			break;

		} case MODE_FLASH: {
			softReset();
			break;

		} case MODE_SETUP: {
			changeMode(prevMode);
			break;
		} default: {
			changeMode(MODE_SETUP);
		}
	}
}

void SckBase::buttonUp() {
	
	sckOut(F("Button up"), PRIO_LOW);
}

void SckBase::veryLongPress() {
	// Make sure we havent released button without noticed it
	if (!digitalRead(PIN_BUTTON)) {
		sckOut(String F("Button very long press: ") + String(millis() - butLastEvent), PRIO_MED);
		// Factory reset
		factoryReset();
	} else buttonEvent();
}

void SckBase::longPress() {
	// Make sure we havent released button without noticed it
	if (!digitalRead(PIN_BUTTON)) {
		sckOut(String F("Button long press: ") + String(millis() - butLastEvent), PRIO_MED);
		changeMode(MODE_OFF);
	} else buttonEvent();
}

void SckBase::softReset() {

	// Close files before resseting to avoid corruption
	closeFiles();

 	NVIC_SystemReset();
}

/* 	-----------------
 	|	 SD card	|
 	-----------------
*/
bool SckBase::sdPresent() {

	ESPcontrol(ESP_OFF);
	digitalWrite(CH_PD, LOW);		// Do this here to avoid wifi led current drain but provide sd card access

	// Make sure files are closed to avoid corruption
	closeFiles();

	if (sd.cardBegin(CS_SDCARD, SPI_HALF_SPEED)) {
		sckOut(F("Sdcard ready!!"));
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		if (config.mode == MODE_SD) {
			errorMode();
		}
		return false;
	}
}

bool SckBase::openPublishFile(bool writeHeader) {

	char charFileName[publishFileName.length()];

	sckOut(F("Opening publish file..."));

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
	sckOut(F("Error opening file in SD card!!!"));
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

void SckBase::goToSleep(bool wakeToCheck) {

	// Wake up in veryLongPressInterval - longPressInterval and if button still down execute reset factory
	// if (wakeToCheck){
	// 	sckOut(F("Waking in a moment to check for factory reset..."));
	// if (ISOtime().equals("0")) {
	// 	rtc.setTime(0, 0, 0);
 //  		rtc.setDate(1, 1, 17);
	// }

	// 	// rtc.setAlarmSeconds((veryLongPressInterval - longPressInterval ) / 1000);
	// rtc.setAlarmSeconds(10);
	// rtc.enableAlarm(rtc.MATCH_SS);
	// rtc.attachInterrupt(ISR_alarm);
	// }


	// rtc.setAlarmSeconds(5000);
	// para implementar periodos de sleep:
	// habra que hacer una seleccion del match (y de la funcion correspondiente segun el periodo solicitado)
	// MATCH_OFF          = RTC_MODE2_MASK_SEL_OFF_Val,          // Never
 //    MATCH_SS           = RTC_MODE2_MASK_SEL_SS_Val,           // Every Minute
 //    MATCH_MMSS         = RTC_MODE2_MASK_SEL_MMSS_Val,         // Every Hour
 //    MATCH_HHMMSS       = RTC_MODE2_MASK_SEL_HHMMSS_Val,       // Every Day
 //    MATCH_DHHMMSS      = RTC_MODE2_MASK_SEL_DDHHMMSS_Val,     // Every Month
	// MATCH_MMDDHHMMSS = RTC_MODE2_MASK_SEL_MMDDHHMMSS_Val, // Every Year
  	// rtc.enableAlarm(rtc.MATCH_SS);

  	// Timer interrupt for wake up
  	// rtc.attachInterrupt(wakeUp);

	// SYSCTRL->VREG.bit.RUNSTDBY = 1;
 //  	SYSCTRL->DFLLCTRL.bit.RUNSTDBY = 1;

	sckOut(F("Going to sleep..."));

	closeFiles();

	ESPcontrol(ESP_OFF);

	USB->DEVICE.CTRLA.bit.SWRST = 1;
  	while (USB->DEVICE.SYNCBUSY.bit.SWRST | (USB->DEVICE.CTRLA.bit.SWRST == 1));

	// USBDevice.detach();

	// Turn off Serial leds
	digitalWrite(SERIAL_TX_LED, HIGH);
	digitalWrite(SERIAL_RX_LED, HIGH);

	// rtc.standbyMode();
}

void SckBase::wakeUp() {

	// USBDevice.init();
	// USBDevice.attach();
	sckOut(F("Waked up!!!"));
	changeMode(MODE_SETUP);
}

void SckBase::checkFactoryReset() {
	if (!digitalRead(PIN_BUTTON)) {
		// wakeUp();
		factoryReset();
	} else {
		goToSleep();
	}
}

void SckBase::factoryReset() {

	msgBuff.com = ESP_LED_OFF;
	ESPqueueMsg(true);

	strncpy(credentials.ssid, "ssid", 64);
	strncpy(credentials.password, "password", 64);
	credentials.time = 0;
	clearNetworks();

	clearToken();

	saveConfig(true);

	// Set a periodic timer for reset when ESP comunication (clear wifi and token) is complete
	timerSet(ACTION_RESET, 1000);
}


/* 	-------------
 	|	 Power management (por ahora es cun copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
 	-------------
*/
uint16_t SckBase::getBatteryVoltage() {

	uint8_t readingNumber = 5;
	uint32_t batVoltage = 0;

	for(uint8_t i=0; i<readingNumber; i++) batVoltage += (2*(readADC(3))*VCC/RESOLUTION_ANALOG);

	return batVoltage / readingNumber;
}

float SckBase::getBatteryPercent() {

	uint8_t percent = 0;
	uint16_t voltage = (uint16_t)getBatteryVoltage();

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

float SckBase::getVUSB() {
  	float chargerVoltage = 2*(readADC(2))*VCC/RESOLUTION_ANALOG;
	return chargerVoltage;
}

uint16_t SckBase::getCHG() {
  uint16_t temp = 2*(readADC(0))*VCC/RESOLUTION_ANALOG;
  return temp;
}

uint16_t SckBase::getISET() {
  uint16_t temp = 2*(readADC(1))*VCC/RESOLUTION_ANALOG;
  return temp;
}

uint16_t SckBase::readADC(byte channel) {
  byte dir[4] = {2,4,6,8};
  byte temp = B11000000 + channel;
  writeI2C(ADC_DIR, 0, temp);
  writeI2C(ADC_DIR, 0, temp);
  uint16_t data = (readI2C(ADC_DIR, dir[channel])<<4) + (readI2C(ADC_DIR, dir[channel] + 1)>>4);
  return data;
}

void SckBase::writeCurrent(int current) {
    int resistor = (4000000/current)-96-3300;
    writeResistor(0, resistor);
}

void SckBase::updatePower() {
	
	if (getVUSB() > 3000){
		// USB is connected
		if (!onUSB) sckOut("USB connected!");
		onUSB = true;

		// USBDevice.init();
		// USBDevice.attach();
		// SerialUSB.begin(baudrate);

		// Led feedback always ON
		led.dim = false;

		if (getCHG() < 3000) { 
			if (!charging) sckOut("Charging battery!");
			charging = true;
		} else charging = false;					// In this case battery should be full

	} else {
		// USB is not connected
		if (onUSB) sckOut("USB disconnected!");
		onUSB = false;

		// Minimal led Feedback
		if (config.mode == MODE_SD || config.mode == MODE_NET) led.dim = true;
		else led.dim = false;

		// USBDevice.init();
		// USBDevice.detach();
		// SerialUSB.end();

		//Turn off Serial leds
		digitalWrite(SERIAL_TX_LED, HIGH);
		digitalWrite(SERIAL_RX_LED, HIGH);

		// There is no way of charging without USB
		charging = false;
	}

	// LED feedback status

	// Low battery led warning
	if (getBatteryPercent() < lowBattLimit && !charging) led.lowBatt = true;
	else led.lowBatt = false;

	// Charging battery led feedback
	if (charging && config.mode != MODE_SETUP) led.charging = true;
	else led.charging = false;

	// Finished charging led feedback
	if (!charging && onUSB && config.mode != MODE_SETUP) led.finishedCharging = true;
	else led.finishedCharging = false;
}

/* 	---------------------
 	|	 Urban Board 	|
 	---------------------
*/
 /*
  * Detect urban board by changing Audio amplifier resistor value (I2C) and check if it responds OK.
  */
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


/* 	-------------
 	|	 Led	|
 	-------------
*/
void Led::setup() {
	off();
	dir = true;
	colorIndex = 0;
	pulseMode = PULSE_STATIC;
}

/* Call this every time there is an event that changes SCK mode
 *
 */ 
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

		
		if (dim) {

			uint8_t waitLoops = 8;
			
			// Count loops after dim count started
			if (colorIndex == 0) {
			
				dimLoops++;

				// If enough time has passed turn brightness down
				if (dimLoops == waitLoops) {
					brightnessFactor = 0;
					heartBeat = 0;
				}
			}

			// Only led spikes after some time when on battery
			if (dimLoops > waitLoops) {
				if (colorIndex == 24) {
					ledRGBcolor = *(currentPulse + 18);
			 		brightnessFactor = 1;
				} else {
					brightnessFactor = 0;
				}
			}

		} else {
			// Restart dim loops counter
			dimLoops = 0;
			brightnessFactor = 1;
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

		// No dim in this type of pulse (this is for errors)

		if (millis() - hardTimer > slowHard) {
			hardTimer = millis();

			if (inErrorColor) ledRGBcolor = currentPulse[24];
			else ledRGBcolor = yellowRGB;

			inErrorColor = !inErrorColor;
		}

	} else if (pulseMode == PULSE_HARD_FAST) {

		// No dim in this type of pulse (this is for errors)

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

/* Change Led color based on RGB values
 *
 */
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
};

/* Change Led color based on HSI values (Hue, Saturation, Intensity)
 *
 */
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
};

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


/* 	-----------------
 	|	 Timers 	|
 	-----------------
*/
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
						if (!digitalRead(POWER_WIFI)) getStatus();
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

					} case ACTION_UPDATE_POWER:{

						updatePower();
						break;

					} case ACTION_DEBUG_LOG: {

						updatePower();
						sdLogADC();
						break;

					} case ACTION_GOTO_SETUP: {

						changeMode(MODE_SETUP);
						break;

					} case ACTION_RECOVER_ERROR: {

						if (config.persistentMode == MODE_SD) {

							if (onTime && sdPresent()) {
								changeMode(MODE_SD);
								timerClear(ACTION_RECOVER_ERROR);
							}
						
						} else if (config.persistentMode == MODE_NET) {

							//if (onWifi && tokenConfigured) changeMode(MODE_NET);
							timerClear(ACTION_RECOVER_ERROR);
						}

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

	if (!slotsFree) sckOut(F("We need more Timer slots!!!"), PRIO_HIGH);
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

void SckBase::timerClearTasks() {

	for (uint8_t i=0; i<timerSlots; i++) {

		if (timers[i].action != ACTION_LONG_PRESS && 
			timers[i].action != ACTION_VERY_LONG_PRESS && 
			timers[i].action != ACTION_GET_ESP_STATUS && 
			timers[i].action != ACTION_UPDATE_POWER &&
			timers[i].action != ACTION_RECOVER_ERROR) {

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


/* 	-------------
 	|	 POT control (por ahora es copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
 	-------------
*/
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

byte SckBase::readI2C(int deviceaddress, byte address ) {
  byte  data = 0x0000;
  Wire.beginTransmission(deviceaddress);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  unsigned long time = millis();
  while (!Wire.available()) if ((millis() - time)>500) return 0x00;
  data = Wire.read(); 
  return data;
}  


/* 	-------------------------
 	|	 Utility functions 	|
 	-------------------------
*/
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
size_t freeRAM(void)
{
char stack_dummy = 0;
return(&stack_dummy - sbrk(0));
}
