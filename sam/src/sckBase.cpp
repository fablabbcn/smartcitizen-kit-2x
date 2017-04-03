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


// Epprom flash emulation to store persistent variables
FlashStorage(eppromMode, EppromMode);
FlashStorage(eppromConf, EppromConf);

// SAM <<>> ESP communication
EasyTransfer BUS_in, BUS_out;

// Urban board
SckUrban urban;

// Auxiliary I2C devices
AuxBoards auxBoards;

// Sleepy dog
WatchdogSAMD wdt;

// Sdcard
SdFat sd;
File publishFile;
File logFile;
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
	ESPcontrol(ESP_ON);				// This keeps ESP wifi leds totally off
	delay(20);
	ESPcontrol(ESP_OFF);
	timerClear(ACTION_CLEAR_ESP_BOOTING);
	ESPbooting = false;

	// Version
	String buildDate = __DATE__;
	buildDate.replace(' ', '-');
	version += buildDate + '-' + String(__TIME__) + "-ALPHA";

	// MODE TITLES
	modeTitles[MODE_AP]			= 	"ap";
	modeTitles[MODE_NET] 		=	"network";
	modeTitles[MODE_SD] 		= 	"sdcard";
	modeTitles[MODE_SHELL] 		= 	"shell";
	modeTitles[MODE_FLASH] 		= 	"esp flash";
	modeTitles[MODE_BRIDGE] 	= 	"esp bridge";
	modeTitles[MODE_ERROR] 		= 	"error";
	modeTitles[MODE_FIRST_BOOT] = 	"first boot";
	modeTitles[MODE_OFF]		=	"off";

	// EXTERNAL COMMAND TITLES
	// Esp Commands
	comTitles[EXTCOM_ESP_REBOOT]			= 	"esp reboot";
	comTitles[EXTCOM_ESP_OFF]				= 	"esp off";
	comTitles[EXTCOM_ESP_ON]				= 	"esp on";
	comTitles[EXTCOM_ESP_START_AP]			=	"esp start ap";
	comTitles[EXTCOM_ESP_STOP_AP]			=	"esp stop ap";
	comTitles[EXTCOM_ESP_START_WEB]			=	"esp start web";
	comTitles[EXTCOM_ESP_STOP_WEB]			= 	"esp stop web";
	comTitles[EXTCOM_ESP_SLEEP]				= 	"esp sleep";
	comTitles[EXTCOM_ESP_WAKEUP]			=	"esp wakeup";
	comTitles[EXTCOM_GET_APLIST]			=	"get aplist";
	comTitles[EXTCOM_ESP_SERIAL_DEBUG_ON]	=	"esp debug on";
	comTitles[EXTCOM_ESP_SERIAL_DEBUG_OFF]	=	"esp debug off";
	comTitles[EXTCOM_ESP_LED_ON]			= 	"esp led on";
	comTitles[EXTCOM_ESP_LED_OFF]			= 	"esp led off";
	comTitles[EXTCOM_ESP_MQTT_HELLO]		=	"mqtt hello";

	// Configuration commands
	comTitles[EXTCOM_SET_WIFI]			= 	"set wifi";
	comTitles[EXTCOM_GET_WIFI]			= 	"get wifi";
	comTitles[EXTCOM_GET_BEST_WIFI]		=	"get best wifi";
	comTitles[EXTCOM_CLEAR_WIFI]		=	"clear wifi";
	comTitles[EXTCOM_GET_IP]			=	"get ip";
	comTitles[EXTCOM_SET_TOKEN]			= 	"set token";
	comTitles[EXTCOM_GET_TOKEN]			= 	"get token";
	comTitles[EXTCOM_CLEAR_TOKEN]		=	"clear token";
	comTitles[EXTCOM_GET_VERSION]		= 	"get version";
	comTitles[EXTCOM_SYNC_CONFIG]		= 	"sync config";
	comTitles[EXTCOM_DOWNLOAD_CONFIG]	= 	"download config";
	comTitles[EXTCOM_SET_CONFIG]		= 	"set config";		// @params: readint (read interval)
	comTitles[EXTCOM_GET_CONFIG]		= 	"get config";

	// Mode commands
	comTitles[EXTCOM_RESET]			= 	"reset";
	comTitles[EXTCOM_RESET_CAUSE]	= 	"rcause";
	comTitles[EXTCOM_GET_MODE]		= 	"get mode";
	comTitles[EXTCOM_SET_MODE]		= 	"set mode";			// @params: net, shell, sdcard, bridge, flash, sleep, off

	// Other configuration
	comTitles[EXTCOM_SET_OUTLEVEL]			= 	"set outlevel";
	comTitles[EXTCOM_GET_OUTLEVEL]			= 	"get outlevel";
	comTitles[EXTCOM_SET_LED]				= 	"set led";				// @params: off, (to implement: red, blue, green, etc)
	comTitles[EXTCOM_GET_URBAN_PRESENT]		= 	"urban present";
	comTitles[EXTCOM_READLIGHT_ON]			=	"set readlight on";
	comTitles[EXTCOM_READLIGHT_OFF]			=	"set readlight off";
	comTitles[EXTCOM_READLIGHT_RESET]		=	"set readlight reset";

	// Time configuration
	comTitles[EXTCOM_GET_TIME]			= 	"get time";			// @params: iso (default), epoch
	comTitles[EXTCOM_SET_TIME]			= 	"set time";			// @params: epoch time
	comTitles[EXTCOM_SYNC_TIME]			= 	"sync time";

	// SD card
	comTitles[EXTCOM_SD_PRESENT]	=	"sd present";

	// Sensors
	comTitles[EXTCOM_GET_SENSOR]		=	"read";			// @params sensor Title
	comTitles[EXTCOM_PUBLISH]			= 	"publish";
	comTitles[EXTCOM_LIST_SENSORS]		=	"list sensors";
	comTitles[EXTCOM_ENABLE_SENSOR]		=	"enable";		// @ params wichSensor
	comTitles[EXTCOM_DISABLE_SENSOR]	=	"disable";		// @params wichSensor

	// Sey Alpha POT's (TODO remove from here and find a more modular solution)
	comTitles[EXTCOM_ALPHADELTA_POT]	=	"set alpha";				// @ params: wichpot (AE1, WE1, AE2...), value (0-100,000)

	comTitles[EXTCOM_GET_CHAN0]			=	"get chann0";
	comTitles[EXTCOM_GET_CHAN1]			=	"get chann1";
	comTitles[EXTCOM_GET_CHARGER]		=	"get charger";
	comTitles[EXTCOM_GET_BATTVOLT]		=	"get battvolt";
	
	// Other
	comTitles[EXTCOM_GET_FREEHEAP]	=	"get heap";
	comTitles[EXTCOM_HELP]			= 	"help";


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

	// Auxiliary boards
	auxBoards.setup();

	// Output level
	outputLevel = OUT_NORMAL;

	analogReadResolution(12);				// Set Analog resolution to MAX
	analogWriteResolution(8);

	// Wich mode was I before turned off?
	EppromMode savedMode = eppromMode.read();

	// Return to last saved mode or Setup Mode as fallback
	if (savedMode.valid) {
		if (savedMode.mode == MODE_OFF) goToSleep();
		else changeMode(savedMode.mode);
	} else changeMode(MODE_AP);

	// Load last saved interval
	EppromConf savedConf = eppromConf.read();
	if (savedConf.valid) {
		configuration.readInterval = savedConf.readInterval;
	}
	if (configuration.readInterval == 0) configuration.readInterval = 15;		// Sanity check default

	if (!urbanPresent) changeMode(MODE_ERROR);

	// Sensors Setup
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		SensorType wichSensor = static_cast<SensorType>(i);

		// Only enable base board sensors
		if (sensors[wichSensor].location == BOARD_BASE) {
			sensors[wichSensor].enabled 	= true;
			sensors[wichSensor].interval 	= configuration.readInterval;	
		// Enable urban board sensors if it has been found
		} else if (urbanPresent && sensors[wichSensor].location == BOARD_URBAN) {
			sensors[wichSensor].enabled 	= true;
			sensors[wichSensor].interval 	= configuration.readInterval;			
		}
	}

	// Temporary disabled
	sensors[SENSOR_NETWORKS].enabled 	= false;
	sensors[SENSOR_CO].enabled 			= false;				// Disabled for now
	sensors[SENSOR_NO2].enabled			= false;				// Disabled for now
	sensors[SENSOR_VOLTIN].enabled 		= false;				// Disabled for now


	// Check if USB connected and enable-disable Serial communication
	USBConnected();

	// For debugging purposes only (comment for production)
	// timerSet(ACTION_DEBUG_LOG, 5000, true);

};

void SckBase::update() {

	// Flash and bridge modes
	if (mode == MODE_FLASH || mode == MODE_BRIDGE){
		if (SerialUSB.available()) Serial1.write(SerialUSB.read());
		if (Serial1.available()) SerialUSB.write(Serial1.read());
	} else {

		// update ESP communications
		if (ESPon) ESPbusUpdate();

		// Update timers
		timerRun();

		// Check Serial ports inputs
		inputUpdate();

		
		//----------------------------------------
		// 	MODE_AP
		//----------------------------------------
		if (mode == MODE_AP) {
			if (!onWifi) {

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
								setReadInterval(lightResults.lines[4].toInt());
								sckOut(String F("New reading interval: ") + String(configuration.readInterval));
							}
						}
						if (lightResults.lines[0].endsWith(F("time"))) {
							setTime(lightResults.lines[1]);
						}
						led.configOK();
					 	readLight.reset();
					 	readLightEnabled = false;

					 	//MQTT Hellow for Onboarding process
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

	// Close any (potencially) open file
	closeFiles();
	
	// Clear ALL timers
	timerClearTasks();

	// Led is steady until we found something to give feedback of
	uint8_t pulseMode = 0;

	// Stop searching for light signals (only do it on setup mode)
	readLightEnabled = false;


	// Configure things depending on new mode
	// --------------------------------------
	// Restore previous output level
	if (mode == MODE_BRIDGE) changeOutputLevel(prevOutputLevel);

	// Actions for each mode
	switch(newMode) {
		case MODE_AP: {

			sckOut(F("Entering Setup mode!"));

			// Start ESP and ap mode
			timerSet(ACTION_ESP_ON, 50);
			
			// Restart lightread for receiving new data
			readLightEnabled = true;
			readLight.reset();
			lightResults.commited = false;
			break;

		} case MODE_NET: {

			// Restart Watchdog
			restartWatchdog();

			// If we dont have wifi turn off esp (it will be turned on by next publish try)
			if (!onWifi) ESPcontrol(ESP_OFF);

			sckOut(F("Entering Network mode!"));

			// first publish in 3 seconds
			timerSet(ACTION_PUBLISH, 3000);

			// Set timer for periodically publishing
			timerSet(ACTION_PUBLISH, configuration.readInterval*1000, true);

			sckOut(String F("Publishing every ") + String(configuration.readInterval) + F(" seconds"));
			break;

		} case MODE_SD: {

			// Restart Watchdog
			restartWatchdog();

			// ESPcontrol(ESP_OFF);

			sckOut(F("Entering SD card mode!"));

			// publish in 3 seconds
			timerSet(ACTION_PUBLISH, 3000);

			// Set timer for periodically publishing
			timerSet(ACTION_PUBLISH, configuration.readInterval*1000, true);

			sckOut(String F("Publishing every ") + String((int)configuration.readInterval) + " seconds");
			break;

		} case MODE_BRIDGE: {
			ESPcontrol(ESP_ON);
			changeOutputLevel(OUT_SILENT);
			break;

		} case MODE_FLASH: {
			changeOutputLevel(OUT_SILENT);
			ESPcontrol(ESP_FLASH);
			break;
		}
	}

	// Save previous mode
	prevMode = mode;

	// Set new mode
	mode = newMode;

	// Save new mode to epprom to recover it after reset.
	if (newMode == MODE_SD || newMode == MODE_NET || newMode == MODE_OFF) {
		EppromMode toSaveMode;
		toSaveMode.valid = true;
		toSaveMode.mode = newMode;
		eppromMode.write(toSaveMode);
	}

	// Update led
	led.update(newMode, pulseMode);

	// This must be at the end so the rest get executed before goig to sleep
	// After reset it will go to sleep in a clean state
	if (newMode == MODE_OFF) goToSleep();
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
			onWifi = false;
			if ((ESPon || ESPbooting) && mode != MODE_BRIDGE) {
				sckOut(F("Turning off ESP..."));
				timerClear(ACTION_GET_ESP_STATUS);
				ESPon = false;
				ESPbooting = false;
				espSerialDebug = false;
				digitalWrite(CH_PD, LOW);
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
			sckOut(F("Putting ESP in flash mode...\r\nRemember to reboot ESP after flashing (esp reboot)!"));
			if (ESPon) ESPcontrol(ESP_OFF);
			SerialUSB.begin(ESP_FLASH_SPEED);
			Serial1.begin(ESP_FLASH_SPEED);
			delay(500);
			digitalWrite(CH_PD, HIGH);
			digitalWrite(GPIO0, LOW);			// LOW for flash mode
			digitalWrite(POWER_WIFI, LOW); 		// Turn on ESP
			break;

		case ESP_ON:
			if (!ESPon && !ESPbooting) {
				sdPresent();
				sckOut(F("Turning on ESP..."));
				SPI.end();						// Important for SCK-1.5.3 so the sdfat lib releases the SPI bus and the ESP can access his flash
				delay(10);
				digitalWrite(CH_PD, HIGH);
				digitalWrite(GPIO0, HIGH);		// HIGH for normal mode
				digitalWrite(POWER_WIFI, LOW); 	// Turn on ESP
				espLastOn = millis();
				delay(10);
				ESPbooting = true;
				timerSet(ACTION_CLEAR_ESP_BOOTING, 500);
			}
			break;

		case ESP_REBOOT:
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
	if (BUS_queueIndex >= 0) {

		// Send first message in queue
		msgOut.com = BUS_queue[0].com;
		strncpy(msgOut.param, BUS_queue[0].param, 240);
		msgOut.waitAnswer = BUS_queue[0].waitAnswer;
		BUS_out.sendData();

		// If we need to wait for confirmation
		bool waitAnswer = BUS_queue[0].waitAnswer;
		bool mesgReceived = false;
		if (waitAnswer) {
			float millisNow = millis();
			while (millis() - millisNow < answerTimeout) {
				if (BUS_in.receiveData()) {
					if (msgIn.com == msgOut.com) mesgReceived = true;
					ESPprocessMsg();
					if (mesgReceived) break;
				}
			}
		}
		// Asume message sent and received
		if (!waitAnswer || mesgReceived) {
			// Remove first message from queue and move the rest
			for (uint8_t i=1; i<=BUS_queueIndex; i++){
				BUS_queue[i-1].com = BUS_queue[i].com;
				strncpy(BUS_queue[i-1].param, BUS_queue[i].param, 240);
				BUS_queue[i-1].waitAnswer = BUS_queue[i].waitAnswer;
			}
		
			// Update queue index
			BUS_queueIndex--;
		}
	}
}

void SckBase::ESPqueueMsg(bool sendParam, bool waitAnswer) {

	if (!ESPon) ESPcontrol(ESP_ON);

	BUS_queueIndex++;

	// Put command message buffer in queue
	BUS_queue[BUS_queueIndex].com = msgBuff.com;

	// Do we need to wait answer for this message??
	if (waitAnswer) BUS_queue[BUS_queueIndex].waitAnswer = true;
	else BUS_queue[BUS_queueIndex].waitAnswer = false;

	// If no need to send params
	if (!sendParam) strncpy(BUS_queue[BUS_queueIndex].param, "", 240);
	else strncpy(BUS_queue[BUS_queueIndex].param, msgBuff.param, 240);	
}

void SckBase::ESPprocessMsg() {

	sckOut(F("Processing message from ESP..."), PRIO_LOW);
	sckOut(String F("Command: ") + String(msgIn.com), PRIO_LOW);
	sckOut(String F("Parameters: ") + String(msgIn.param), PRIO_LOW);

	switch(msgIn.com) {
		case ESP_GET_STATUS_COM: {
			processStatus();
			break;

		} case ESP_BOOTED_AND_READY: {
			ESPbooting = false;
			ESPon = true;
			sckOut(F("ESP ready!!!"), PRIO_LOW);
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
			
			// configuration.readInterval = jsonConf["ri"];
			setReadInterval(jsonConf["ri"]);
			
			saveConf();
			sckOut(F("Configuration updated:"));
			sckOut(String F("Reading interval: ") + String(configuration.readInterval));
			prompt();
			break;

		} case ESP_GET_APCOUNT_COM: {
			OneSensor *thisSensor = &sensors[SENSOR_NETWORKS];
			thisSensor->lastReadingTime = rtc.getEpoch();
			thisSensor->reading = atof(msgIn.param);
			thisSensor->valid = true;
			sckOut(thisSensor->title + ": " + String(thisSensor->reading) + " " + thisSensor->unit);
			prompt();
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
			led.configOK();
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
				if (mode != MODE_AP) led.update(mode, 0);

				onWifi = true;

				// If we dont have time...
				if (!onTime) {
					msgBuff.com = ESP_GET_TIME_COM;
					ESPqueueMsg(false, true);
				}

				if (triggerHello) {
					sckOut(F("Sending MQTT Hello..."));
					msgBuff.com = ESP_MQTT_HELLOW_COM;
					ESPqueueMsg(false, false);
					triggerHello = false;
				} else if (ESPpublishPending) {
					// If there is a publish operation waiting...
					ESPpublish();
					ESPpublishPending = false;
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
			// If we are not expecting apmode configuration
			if(mode != MODE_AP) ESPcontrol(ESP_OFF);

			// If we NEED network give feedback about error
			if (mode == MODE_NET) led.update(mode, 2);

			// If there is a pending publish
			if (ESPpublishPending) {
				sckOut(F("ERROR: publish failed, saving to SDcard only..."));
				bool platformPublishedOK = false;
				publishToSD(platformPublishedOK);
				ESPpublishPending = false;
			}
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

				// Published OK so restarting the watchdog.
				restartWatchdog();

				bool platformPublishedOK = true;
				publishToSD(platformPublishedOK);
				break;

			} case ESP_MQTT_HELLO_OK_EVENT: {
				sckOut(F("MQTT Hello OK!!"));

				// Go to network mode
				changeMode(MODE_NET);
				break;

			} case ESP_MQTT_ERROR_EVENT: {
				sckOut(F("ERROR: MQTT failed!!"));
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
				if (!onTime) led.update(mode, 1);
				break;

			} case ESP_TIME_UPDATED_EVENT: {
				
				// Time sync
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
			if (ESPon) {
				msgBuff.com = ESP_SERIAL_DEBUG_ON;
				espSerialDebug = true;
				ESPqueueMsg(false);
			} else {
				sckOut(F("Please start ESP first!"));
			}
			break;

		} case EXTCOM_ESP_SERIAL_DEBUG_OFF: {
			if (ESPon) {
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
			sckOut(version);
			break;

		} case EXTCOM_SYNC_CONFIG: {
			sckOut(F("To be implemented!!!"), PRIO_HIGH);
			break;

		} case EXTCOM_DOWNLOAD_CONFIG: {
			sckOut(F("To be implemented!!!"), PRIO_HIGH);
			break;

		} case EXTCOM_SET_CONFIG: {
			// TODO poner los params en el help			
			if (strIn.startsWith("readint")) {
				strIn.replace("readint", "");
				strIn.trim();
				uint32_t intTinterval = strIn.toInt();

				if (intTinterval > 0 && intTinterval < 86400) {
					// configuration.readInterval = intTinterval;
					setReadInterval(intTinterval);
					saveConf();
					sckOut(String F("Change reading interval to: ") + String(configuration.readInterval), PRIO_HIGH);
				}
			}
			break;

		} case EXTCOM_GET_CONFIG: {
			sckOut(String F("Reading interval: ") + configuration.readInterval);
			prompt();
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

		} case EXTCOM_GET_MODE: {
			sckOut(String F("Current mode is ") + modeTitles[mode] + F(" mode"));
			break;

		} case EXTCOM_SET_MODE: {

			SCKmodes requestedMode = MODE_COUNT;

			for (uint8_t i=0; i < MODE_COUNT; ++i) {
				if (strIn.startsWith(modeTitles[i])) {
					requestedMode = static_cast<SCKmodes>(i);
					changeMode(requestedMode);
					break;
				}
			}
			if (requestedMode == MODE_COUNT) sckOut(F("Unrecognized mode, please try again!"));
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
			}
			break;

		} case EXTCOM_GET_URBAN_PRESENT: {
			if (urbanBoardDetected()) sckOut(F("Urban board detected!!"), PRIO_HIGH);
			else sckOut(F("Urban board not found!!"), PRIO_HIGH);
			break;

		} case EXTCOM_READLIGHT_ON: {

			sckOut(F("Turning on readlight..."));
			readLightEnabled = true;
			break;

		} case EXTCOM_READLIGHT_OFF: {

			sckOut(F("Turning on readlight..."));
			readLightEnabled = false;
			break;

		} case EXTCOM_READLIGHT_RESET: {

			readLight.reset();
			readLightEnabled = true;
			break;

		// Time configuration
		} case EXTCOM_SET_TIME: {
			setTime(strIn);
			break;

		} case EXTCOM_GET_TIME: {
			if (ISOtime().equals("0")) sckOut(F("Time NOT synced since last reset!!!"), PRIO_HIGH);
			if (strIn.equals("epoch")) {
				sckOut(String(rtc.getEpoch()));
			} else {
				sckOut(ISOtime());
			}
			break;
		} case EXTCOM_SYNC_TIME: {
			if (ESPon) {
				msgBuff.com = ESP_GET_TIME_COM;
				ESPqueueMsg(false);
			} else {
				sckOut(F("First start ESP and wait for network connection!!"));
			}
			break;

		// SD card
		} case EXTCOM_SD_PRESENT: {
			if (sdPresent()) sckOut(F("Sdcard ready!!!"));
			break;
	
		} case EXTCOM_GET_SENSOR: {

			SensorType thisType = SENSOR_COUNT;

			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				thisType = static_cast<SensorType>(i);
				
				// Makes comparison lower case and not strict (sensor title only have to contain command)
				String titleCompare = sensors[thisType].title;
				titleCompare.toLowerCase();
				strIn.toLowerCase();
				
				if (titleCompare.indexOf(strIn) > -1) break;
				thisType = SENSOR_COUNT;
			}

			// Failed to found your sensor
			if (thisType == SENSOR_COUNT) {
				sckOut(F("Can't find that sensor!!!"));
			} else {

				
				// Get reading
				if (getReading(thisType)) {
					
					OneSensor *thisSensor = &sensors[thisType];
					sckOut(thisSensor->title + ": " + String(thisSensor->reading) + " " + thisSensor->unit);
				
				} else {
					
					// Exception for Networks readings because it is async... it will output when its finished.
					if (thisType == SENSOR_NETWORKS) break;

					sckOut(F("Failed getting reading!!!"));
				}
			}
			break;

		} case EXTCOM_GET_CHAN0: {
			sckOut(String F("chan0: ") + getChann0() + F(" V"), PRIO_HIGH);
			break;

		} case EXTCOM_GET_CHAN1: {
			sckOut(String F("chan1: ") + getChann1() + F(" V"), PRIO_HIGH);
			break;

		} case EXTCOM_GET_CHARGER: {
			sckOut(String F("charger: ") + getCharger() + F(" V"), PRIO_HIGH);
			break;

		}case EXTCOM_GET_BATTVOLT: {
			sckOut(String F("charger: ") + getBatteryVoltage() + F(" V"), PRIO_HIGH);
			break;

		} case EXTCOM_ENABLE_SENSOR: {

			SensorType thisType = SENSOR_COUNT;

			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				thisType = static_cast<SensorType>(i);
				
				// Makes comparison lower case and not strict (sensor title only have to contain command)
				String titleCompare = sensors[thisType].title;
				titleCompare.toLowerCase();
				strIn.toLowerCase();
				
				if (titleCompare.indexOf(strIn) > -1) break;
				thisType = SENSOR_COUNT;
			}

			// Failed to found your sensor
			if (thisType == SENSOR_COUNT) {
				sckOut(F("Can't find that sensor!!!"));
			} else {
				
				OneSensor *thisSensor = &sensors[thisType];
				sckOut(String F("Enabling ") + thisSensor->title);
				sensors[thisType].enabled = true;

				headersChanged = true;
			}

			break;

		} case EXTCOM_DISABLE_SENSOR: {

			SensorType thisType = SENSOR_COUNT;

			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				thisType = static_cast<SensorType>(i);
				
				// Makes comparison lower case and not strict (sensor title only have to contain command)
				String titleCompare = sensors[thisType].title;
				titleCompare.toLowerCase();
				strIn.toLowerCase();
				
				if (titleCompare.indexOf(strIn) > -1) break;
				thisType = SENSOR_COUNT;
			}

			// Failed to found your sensor
			if (thisType == SENSOR_COUNT) {
				sckOut(F("Can't find that sensor!!!"));
			} else {
				
				OneSensor *thisSensor = &sensors[thisType];
				sckOut(String F("Disabling ") + thisSensor->title);
				sensors[thisType].enabled = false;

				headersChanged = true;
			}


			break;

		} case EXTCOM_LIST_SENSORS: {

			SensorType thisType = SENSOR_COUNT;

			sckOut(F("\nEnabled"));
			sckOut(F("----------"));
			// Get sensor type
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				thisType = static_cast<SensorType>(i);

				if (sensors[thisType].enabled) sckOut(sensors[thisType].title);
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

		} case EXTCOM_GET_FREEHEAP: {

			// Get ESP free heap
			msgBuff.com = ESP_GET_FREE_HEAP_COM;
			ESPqueueMsg(false);

			// TODO get SAM free

			break;

		// Help
		} case EXTCOM_HELP: {
			sckOut("", PRIO_HIGH);
			for (uint8_t i=0; i<EXTCOM_COUNT-2; i+=3) {
				for (uint8_t ii=0; ii<3; ii++) {
					sckOut("- ", PRIO_HIGH, false);
					sckOut(comTitles[i+ii], PRIO_HIGH, false);
					for (uint8_t iii=0; iii<3-((comTitles[i+ii].length() + 2) / 8); iii++){
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
		sckOut(F("RTC updated!!!"));
		onTime = true;
		sckOut(ISOtime());
		prompt();
		if (mode != MODE_AP) changeMode(mode);
		return true;
	}
	else sckOut(F("RTC update failed!!"));
	return false;
}

String SckBase::ISOtime() {
	// Return string.format("%04d-%02d-%02dT%02d:%02d:%02dZ", tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
	if (onTime) {
		String isoTime = "20" + 
		leadingZeros(String(rtc.getYear()), 2) + "-" + 
		leadingZeros(String(rtc.getMonth()), 2) + "-" + 
		leadingZeros(String(rtc.getDay()), 2) + "T" +  
		leadingZeros(String(rtc.getHours()), 2) + ":" + 
		leadingZeros(String(rtc.getMinutes()), 2) + ":" + 
		leadingZeros(String(rtc.getSeconds()), 2) + "Z";
		return isoTime;
	} else {
		return "0";
	}
}

String SckBase::epoch2iso(uint32_t toConvert) {

	time_t tc = toConvert;
    struct tm* tmp = gmtime(&tc);

	String isoTime = "20" + 
	leadingZeros(String(tmp->tm_year - 100), 2) + "-" +
	leadingZeros(String(tmp->tm_mon + 1), 2) + "-" + 
	leadingZeros(String(tmp->tm_mday), 2) + "T" +
	leadingZeros(String(tmp->tm_hour), 2) + ":" + 
	leadingZeros(String(tmp->tm_min), 2) + ":" + 
	leadingZeros(String(tmp->tm_sec), 2) + "Z";
	
	return isoTime;
}

/* 	---------------
 	|	Sensors   |
 	---------------
*/
void SckBase::publish() {

	if (mode == MODE_NET) ESPcontrol(ESP_ON);

	sckOut("Reading sensors...");
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType wichSensor = static_cast<SensorType>(i);
		if (sensors[wichSensor].enabled) {
			// Restart the valid flag
			getReading(wichSensor);
		} else {
			sensors[wichSensor].reading = 0;
		}
	}

	timerSet(ACTION_READING_FINISHED, 100, true);
}

bool SckBase::getReading(SensorType wichSensor) {

	// reading is not yet valid...
	sensors[wichSensor].valid = false;

	switch (sensors[wichSensor].location) {
		case BOARD_BASE: {
			// If we are reading sensors from this board
			float tempReading = 0;

			switch (wichSensor) {
				case SENSOR_BATTERY: tempReading = getBatteryPercent(); break;
				case SENSOR_TIME: tempReading = rtc.getEpoch(); break;
				case SENSOR_VOLTIN: tempReading = getCharger(); break;
				case SENSOR_NETWORKS: {
					if (mode == MODE_NET) {		// Dont turn on ESP on sdcard mode
						msgBuff.com = ESP_GET_APCOUNT_COM;
						ESPqueueMsg(false, false);
						return false;	// This case is a exception because we have to wait for ESP to answer network scan
					} else {
						sensors[wichSensor].reading = 0;
					}
				}
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
		}
		sckOut(String(sensors[wichSensor].reading));
	}

	// Store last reading time
	if (sensors[wichSensor].valid) sensors[wichSensor].lastReadingTime = rtc.getEpoch();

	return sensors[wichSensor].valid;
}

bool SckBase::readingFinished() {
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType wichSensor = static_cast<SensorType>(i);
		
		// If sensor IS enabled check if reading is finished
		if (sensors[wichSensor].enabled) {
			if (!sensors[wichSensor].valid) return false;
			if (rtc.getEpoch() - sensors[wichSensor].lastReadingTime > READING_MAX_TIME) return false;
		}
	}
	return true;
}

void SckBase::sensorPublish() {

	bool platformPublishedOK = false;

	if (mode == MODE_NET) {
		// First publish on the platform
		ESPpublish();

	} else {
		publishToSD(platformPublishedOK);
	}
}

void SckBase::ESPpublish() {

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonSensors = jsonBuffer.createObject();

	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType wichSensor = static_cast<SensorType>(i);
		
		// Only send enabled sensors
		if (sensors[wichSensor].enabled) {

			// exception for time (send ISO time)
			if (wichSensor == SENSOR_TIME) { 
				jsonSensors[String(i)] = sensors[wichSensor].lastReadingTime;
			} else {
				jsonSensors[String(i)] = double_with_n_digits(sensors[wichSensor].reading, 2);
			}
		}
	}

	jsonSensors.printTo(msgBuff.param, 240);

	msgBuff.com = ESP_MQTT_PUBLISH_COM;
	sckOut(String F("Sending readings to ESP..."));
	sckOut((msgBuff.param), PRIO_LOW);

	ESPqueueMsg(true, true);
}

bool SckBase::publishToSD(bool platformPublishedOK) {

	if (openPublishFile()) {
		
		// Write down the platform publish status of this reading
		if (platformPublishedOK) publishFile.print("1,");
		else publishFile.print("0,");

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			
			SensorType wichSensor = static_cast<SensorType>(i);

			// Only print data if sensors is enabled, otherwise print only the ,
			if (sensors[wichSensor].enabled) {

				// Time need a special treatment
				if (wichSensor == SENSOR_TIME) {
					publishFile.print(epoch2iso(sensors[wichSensor].lastReadingTime));
				}
				// This sensors needs rounding
				else if (wichSensor == SENSOR_NETWORKS || wichSensor == SENSOR_BATTERY)
					publishFile.print((int)sensors[wichSensor].reading);

				// This are published as float
				else
					publishFile.print(sensors[wichSensor].reading);
			} 

			if (wichSensor < SENSOR_COUNT) publishFile.print(",");

		}
		publishFile.println("");
		publishFile.close();

		// Only asume that everything is working if we are not expecting network publish
		if (mode == MODE_SD) {
			restartWatchdog();
		}
		sckOut(F("Readings saved to SD!!"));
		return true;
	} else {
		if (mode == MODE_SD) {
			sckOut(F("ERROR: Cant' open publish file!!!"));
			led.update(mode, 2);
		}
	}
	return false;
}

bool SckBase::sdLogADC(){

	bool writeHead = false;
	char debugFileName[] = "debug.csv";

	// Create a file for writing
	if (sdPresent()) {

		// Check if we are using a new file
		if (!sd.exists(debugFileName)) writeHead = true;

		// Open file
		debugLogFile = sd.open(debugFileName, FILE_WRITE);

		// Write header
		if (writeHead) debugLogFile.println(F("chann0,chann1,charger,battvolt"));

		debugLogFile.print(getChann0());
		debugLogFile.print(",");
		debugLogFile.print(getChann1());
		debugLogFile.print(",");
		debugLogFile.print(getCharger());
		debugLogFile.print(",");
		debugLogFile.println(getBatteryVoltage());

		debugLogFile.close();

	}
}

void SckBase::saveConf() {

	EppromConf toSaveConf;
	toSaveConf.valid = true;
	toSaveConf.readInterval = configuration.readInterval;
	eppromConf.write(toSaveConf);
}

void SckBase::setReadInterval(uint32_t newReadInterval) {

	configuration.readInterval = newReadInterval;

	// Restart timers
	changeMode(mode);
}

/* 	--------------
 	|	Button   |
 	--------------
*/
void SckBase::buttonEvent() {

	if (!digitalRead(PIN_BUTTON)) {

		butIsDown = true;
		butLastEvent = millis();

		timerSet(ACTION_LONG_PRESS, longPressInterval);
		timerSet(ACTION_VERY_LONG_PRESS, veryLongPressInterval);

		buttonDown();

	} else {

		butIsDown = false;
		butLastEvent = millis();		
		
		timerClear(ACTION_LONG_PRESS);
		timerClear(ACTION_VERY_LONG_PRESS);

		buttonUp();
	}
}

void SckBase::buttonDown() {

	sckOut(F("buttonDown"), PRIO_MED);

	switch (mode) {
		case MODE_OFF: {
			wakeUp();
			break;

		} case MODE_FLASH: {
			softReset();
			break;

		} case MODE_BRIDGE: {
			changeMode(prevMode);
			break;

		} case MODE_NET: {
			changeMode(MODE_SD);
			break;

		} case MODE_SD: {
			changeMode(MODE_AP);
			break;

		} case MODE_AP: {
			changeMode(MODE_NET);
			break;
		} default: {
			changeMode(MODE_AP);
		}
	}
}

void SckBase::buttonUp() {
	
	sckOut(F("Button up"), PRIO_MED);
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
 	wdt.enable(10);
}

/* 	-----------------
 	|	 SD card	|
 	-----------------
*/
bool SckBase::sdPresent() {

	ESPcontrol(ESP_OFF);

	if (sd.cardBegin(CS_SDCARD, SPI_HALF_SPEED)) {
		sckOut(F("Sdcard ready!!"), PRIO_LOW);
		if (mode == MODE_SD) {
			if (onTime) led.update(mode, 0);
			else {
				led.update(mode, 1);
				sckOut(F("Error: RTC out of time!!!"));
			}
		}
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		if (mode == MODE_SD) {
			led.update(mode, 2);
		}
		return false;
	}
}

bool SckBase::openPublishFile() {

	char charFileName[publishFileName.length()];

	bool writeHeader = false;

	if (sdPresent()) {

		sd.begin(CS_SDCARD);
		
		uint8_t fi = 1;
		while (fi < 128) {

			publishFileName.toCharArray(charFileName, publishFileName.length() + 1);

			// If file doesn't exist we need to write header or headers have changed
			if (!sd.exists(charFileName)) writeHeader = true;

			headersChanged = false;

			// Open file
			publishFile = sd.open(charFileName, FILE_WRITE);
			
			if (publishFile) {
				// Check if file is not to big
				if (publishFile.size() < FileSizeLimit) {
					if (writeHeader) {
						// Write headers;
						// Published OK or not
						publishFile.print(F("Published,"));

				 		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
							SensorType wichSensor = static_cast<SensorType>(i);
							publishFile.print(sensors[wichSensor].title);
							
							// If there are units cofigured
							if (sensors[wichSensor].unit.length() > 0) {
								publishFile.print("-");
								publishFile.print(sensors[wichSensor].unit);	
							}
							if (wichSensor < SENSOR_COUNT) publishFile.print(",");
						}
						publishFile.println("");
					}
					sckOut(String F("Using ") + publishFileName + F(" to store posts."), PRIO_LOW);
					return true;
				} else {
					publishFileName = String F("POST") + leadingZeros(String(fi), 3) + F(".CSV");
					publishFile.close();
				}
			}
		}
	}
	sckOut(F("Error opening file in SD card!!!"));
	return false;
}

bool SckBase::openLogFile() {

	// Turn off ESP since it don't play well with SDcard right now 
	ESPcontrol(ESP_OFF);

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

	ESPcontrol(ESP_OFF);

	USB->DEVICE.CTRLA.bit.SWRST = 1;
  	while (USB->DEVICE.SYNCBUSY.bit.SWRST | (USB->DEVICE.CTRLA.bit.SWRST == 1));

	USBDevice.detach();

	// Turn off Serial leds
	digitalWrite(SERIAL_TX_LED, HIGH);
	digitalWrite(SERIAL_RX_LED, HIGH);

	// rtc.standbyMode();
}

void SckBase::wakeUp() {

	USBDevice.init();
	USBDevice.attach();
	sckOut(F("Waked up!!!"));
	if (prevMode != MODE_OFF) changeMode(prevMode);
	else changeMode(MODE_AP);
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

	// Save mode for next reboot
	EppromMode toSaveMode;
	toSaveMode.valid = true;
	toSaveMode.mode = MODE_AP;
	eppromMode.write(toSaveMode);

	// Reset configuration
	configuration.readInterval = 60;
	saveConf();

	// Set a periodic timer for reset when ESP comunication (clear wifi and token) is complete
	timerSet(ACTION_FACTORY_RESET, 1000);
}


/* 	-------------
 	|	 Power management (por ahora es cun copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
 	-------------
*/
float SckBase::getBatteryVoltage() {
	float batVoltage = (int)(2*(readADC(3))*VCC/RESOLUTION_ANALOG);
	return batVoltage;
}

float SckBase::getBatteryPercent() {

	float percent = 0;
	float voltage = getBatteryVoltage();

	for(uint16_t i = 0; i < 100; i++) {
    	if(voltage < batTable[i]) {
      		percent = i * 10;
      		break;
    	}
  	}
  	if(percent < 10 && percent > 0) percent = 10;

  	return percent/10;
}

float SckBase::getCharger() {
  	float chargerVoltage = 2*(readADC(2))*VCC/RESOLUTION_ANALOG;
	return chargerVoltage;
}

uint16_t SckBase::getChann0() {
  uint16_t temp = 2*(readADC(0))*VCC/RESOLUTION_ANALOG;
  return temp;
}

uint16_t SckBase::getChann1() {
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

void SckBase::writeCurrent(int current)
  {
    int resistor = (4000000/current)-96-3300;
    writeResistor(0, resistor);
  }

bool SckBase::USBConnected() {

	if (getCharger() > 4000){
		// USB is connected

		// If this is the first time we detect the connection
		if (!onUSB) {

			// Turn on readlight debug output
			readLight.debugFlag = true;

			USBDevice.init();
			USBDevice.attach();
		}

		onUSB = true;

	} else {
		// USB is not connected

		// If this is the first time we detect the disconnection
		if (onUSB) {

			// Turn off readlight debug output
			readLight.debugFlag = false;

			USBDevice.detach();

			// Turn off Serial leds
			digitalWrite(SERIAL_TX_LED, HIGH);
			digitalWrite(SERIAL_RX_LED, HIGH);
		}

		onUSB = false;
	}

	return onUSB;
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
		case MODE_AP: {
			currentPulse = pulseRed;
			break;
		} case MODE_NET: {
			currentPulse = pulseBlue;
			break;
		} case MODE_SD: {
			currentPulse = pulsePink;
			break;
		} case MODE_SHELL: {
			ledRGBcolor = orangeRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_ERROR: {
			ledRGBcolor = redRGB;
			break;
		} case MODE_FLASH: {
			ledRGBcolor = lightBlueRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_BRIDGE: {
			ledRGBcolor = lightGreenRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_FIRST_BOOT: {
			ledRGBcolor = yellowRGB;
			pulseMode = PULSE_STATIC;
			break;
		} case MODE_OFF: {
			ledRGBcolor = offRGB;
			pulseMode = PULSE_STATIC;
			break;
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
	} else if (pulseMode == PULSE_HARD_SLOW) {

		if (millis() - hardTimer > slowHard) {
			hardTimer = millis();

			// TODO Optimize this!!!
			// if (ledRGBcolor.r == offRGB.r && ledRGBcolor.g == offRGB.g && ledRGBcolor.b == offRGB.b) ledRGBcolor = currentPulse[24];
			if (ledRGBcolor.r == yellowRGB.r && ledRGBcolor.g == yellowRGB.g && ledRGBcolor.b == yellowRGB.b) ledRGBcolor = currentPulse[24];
			// else ledRGBcolor = offRGB; 
			else ledRGBcolor = yellowRGB; 
		}

	} else if (pulseMode == PULSE_HARD_FAST) {

		if (millis() - hardTimer > fastHard) {
			hardTimer = millis();
			// if (ledRGBcolor.r == offRGB.r && ledRGBcolor.g == offRGB.g && ledRGBcolor.b == offRGB.b) ledRGBcolor = currentPulse[24];
			if (ledRGBcolor.r == yellowRGB.r && ledRGBcolor.g == yellowRGB.g && ledRGBcolor.b == yellowRGB.b) ledRGBcolor = currentPulse[24];
			// else ledRGBcolor = offRGB; 
			else ledRGBcolor = yellowRGB; 
		}

	}

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
						ESPbooting = false;
						ESPon = true;
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
						if (!readLightEnabled) getStatus();
						break;
					
					} case ACTION_LONG_PRESS: {
						longPress();
						break;

					} case ACTION_VERY_LONG_PRESS: {
						veryLongPress();
						break;

					} case ACTION_FACTORY_RESET: {

						EppromMode toSaveMode;
						toSaveMode.valid = true;
						toSaveMode.mode = MODE_AP;
						eppromMode.write(toSaveMode);
						softReset();
						break;

					} case ACTION_READING_FINISHED: {
						if (readingFinished()) {
							timerClear(ACTION_READING_FINISHED);
							sckOut("");
							sckOut("--------------------------");
							for (uint8_t i=0; i<SENSOR_COUNT; i++) {

								SensorType wichSensor = static_cast<SensorType>(i);

								if (sensors[wichSensor].enabled) {
									if (wichSensor == SENSOR_TIME) 
										sckOut(sensors[wichSensor].title + ": " + epoch2iso(sensors[wichSensor].lastReadingTime) + " " + sensors[wichSensor].unit, PRIO_HIGH);
									else if (wichSensor == SENSOR_NETWORKS) 
										sckOut(sensors[wichSensor].title + ": " + String(sensors[wichSensor].reading) + " " + sensors[wichSensor].unit, PRIO_HIGH);
									else 
										sckOut(sensors[wichSensor].title + ": " + String(sensors[wichSensor].reading) + " " + sensors[wichSensor].unit, PRIO_HIGH);
								}
							}
							sckOut("--------------------------");
							if (mode == MODE_NET) {
								if (onWifi) ESPpublish();
								else ESPpublishPending = true;

							} else if (mode == MODE_SD) publishToSD();
						}
						break;

					} case ACTION_PUBLISH: {

						// If we are not waiting for previous publish triggger a new one
						if (!timerExists(ACTION_READING_FINISHED)) publish();
						break;

					} case ACTION_DEBUG_LOG: {

						sdLogADC();
						break;

					} case ACTION_WATCHDOG_RESET: {

						// Only reset kit in Setup and net modes
						if (mode == MODE_NET || mode == MODE_AP) softReset();
						break;
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

		if (timers[i].action != ACTION_LONG_PRESS && timers[i].action != ACTION_VERY_LONG_PRESS && timers[i].action != ACTION_GET_ESP_STATUS) {
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

void SckBase::restartWatchdog() {

	sckOut(F("Restarting Watchdog..."));

	// Clear the previous set Watchdog
	timerClear(ACTION_WATCHDOG_RESET);

	// Reset kit if nobody calls this function in 5 reading intervals
	timerSet(ACTION_WATCHDOG_RESET, configuration.readInterval * 1000 * MAX_PUBLISH_FAILS_ALLOWED);
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




/*

 00000000    0000     00000       0000
    00     00    00   00  00    00    00
    00    00      00  00   00  00      00
    00    00      00  00   00  00      00
    00     00    00   00  00    00    00
    00       0000     00000       0000
NOTAS
-----BASE BOARD-------
Hay que documentar el cambio de MQTT_MAX_PACKET_SIZE en pubSubClient.h a 1024 por que si no se hace los paquetes largos simplemente son desechados sin aviso

BUGS
*** MODECHANGE ORGANIZATION
-- Hacer una funcion que borre TODOS los timers
-- Organizar y unificar los cambios cuando pasas de un modo a otro
-- HAY que revisar bien el flow de errores en NET y SD MODE 

*** SDCARD ISSUES
-- En sdcard mode no inicia el serial
-- sin SDcard no init
-- Si quito la sdcard y la reinserto el esp ya no inicia
-- a veces despues de hacer ciclos de botones el esp ya no prende

*** BUTTON ISSUES
-- a veces el boton se dispara dos veces

*** ONBOARDING TODO
-- hacer un mqtt hellow siempre que se de una config exitosa, no solo desde el readlight, tambien el webserver
-- Agregar al webserver las instrucciones para el onboarding despues de un setting exitoso
-- Hacer pruebas de readlight y webserver


-- poner un minimo de intervalo entre posts
-- revisar bien el wait for answer, necesitamos una confirmacion (por lo menos un ack) sobre algunos comandos con el esp (set token set wifi... etc)
-- verificar si clear token causa un reset under certain conditions


BUGS LEVES
-- get best wifi via serial dont return anything
-- cuando hago un publish o get readings aparece el numero de nets antes del output (en la consola)
-- a veces el webserver necesita un reload para mostrar la lista de apps
-- al ejecutar un ESP_OFF el led del esp se queda con algo de corriente (parece ser por iniciar la sdcard) 
-- aunque esten deshabilitados los MICS siguen usando el preheater, habria que ejecutar micsOn y micsOff cuando mics.enabled cambie.
-- a veces cuando el esp no se puede conectar el kit se queda en un loop, hay que checar si postea en sd. creo que no apaga el ESP y eso previene el guardado en sd. me imagino que no hay feedback por parte del esp... podria poner un timeout. Todo tiene que ver con el usb!! hay que ver el detach
-- poner un limite de publish no exitosos para reiniciar el sck
-- si saco la sdcard y la reinserto cuando el esp esta prendido, el esp ya no vuelve a despertar.


Short things todo
-- hacer una funcion process wifi que reciba un string y lo procese (ya no porcesar por separado)
-- lo mismo para cualwuier input que exista en mas lugares que en la consola


** STATE MACHINE DESIGN (MODES)
  --it's important to get a very simple design
  --Make a list of triggers that changes the mode
  [x] -- Revisar bien el flow de eventos y excepciones para cada modo
  [x] -- Poner los cambios de modo con el boton
  [x] -- handle wifi errors without entering in setup mode (at least not instantly to avoid confusion on temporal network errors)
  [x] -- In APmode when succsesfull setted kit (by any means: light, webbrowser, etc) the light will go to green, if you want to set another thing you have to manually cycle button to start again the ap-setup mode
  [ ] -- Unify nomenclatures and call always "setup mode" instead of apmode. Reserve apmode for esp wifi-ap mode.
  [x] -- Double check net mode flow
  [x] -- WatchDog -- poner un timeout de 5*readInterval en que si no hay succsesfull net publish en net mode o sdpublish en sd mode reiniciar el kit

** INPUT-OUTPUT
  [x] -- sckIn receives a struct with a string and the origin (USB, ESP, LIGHT, etc), process the command, and outputs via sckOut
  [x] -- sckOut recibe un string con el String y lo redirecciona a las outputs disponibles (USB,ESP, SDCARD), dependiendo del nivel de output y de la prioridad del mensaje
  [x] -- Create an array to store command titles saving all the strings in flash
  [x] -- FANCY in the case of interactive inputs, get a prompt!! ej. >, echo of what youre typing and a welcome message with some info.
  [x] -- Avoid hangs with serial port on sleep
  [x] -- detect if usb serial is connected, and avoid serial communications if there is no usb connected
  [ ] -- Test what happens about serial communications if you just connect a charger and not usb data connection
  [ ] -- agregar al help la posibilidad de descripciones de commandos
  [x] -- Crear un comando externo "List sensors"
  [ ] -- Trabajar el help...

** ESP COMMUNICATION
  [x] -- Cuando se manda algo al ESP hay que prenderlo
  [x] -- Enviar una notificacion de boot end cuando arranca el esp
  [x] -- Cuando se requiere al ESP y esta apagado hacer un retry una vez que ya este encendido
  [x] -- Eliminar el waitforConnectresult y substituirlo por algo non blocking (en la funcion tryconnect del ESP)
  [x] -- Hacer que los commandos SAM <> ESP sean numericos
  [x] -- Migrar todo el codigo a arduino
  [x] -- Esp control  (ESP_OFF, ESP_ON, ESP_REBOOT, etc)
  [x] -- Implementar una manera de salir de ESPbridging mode
  [x] -- Send Serial commands
  [x] -- Get status (rediseñar el sistema de mensajes)
  [x] -- envio de credenciales al ESP (easyTransfer)
  [x] -- envio de token a esp (easytransfer)
  [x] -- get wifi from ESP (easytrasfer)
  [x] -- get token from ESP (easyTranfer)
  [x] -- get APscan from ESP 
  [x] -- get time from esp,
  [x] -- Implementar un timer en base a lastimeSync para ajustar el reloj a traves de la red cuando tenemos wifi.
  [x] -- Cambiar el espbooting con un timeout depsues de iniciar el esp
  [x] -- Limpiar de nuevo el readlight!!! y encontrar el problema que hace que dejen de fluir los datos despues de calibrado
  [x] -- Arreglar el bug del readlight y Serial1
  [x] -- Hacer una cola de mensajes (msgout) y poner un manejador de cola con retrys y acks
  [x] -- Reimplementar el esp publish
  [ ] -- Si no tengo un token marco error con el led
  [ ] -- Asociar el Token al wifi con el que se dio de alta, si se da de alta un wifi sin token tomar el ultimo usado, lo mismo a la inversa.
  [ ] -- Grabar en un archivo separado las lecturas que no se publicaron exitosamente para hacer mas facil el revisar cuales faltan y al publicarlas escribirlas en el archivo normal.
  [ ] -- cuando no hay sdcard (o siempre?) mantener en ram las publicaciones fallidas y publicarlas en la primera conexion exitosa.

** LED
  [x] -- Automatic change depending on mode
  [x] -- Mejorar los fades y colores
  [x] -- Integrate HSI and RGB colors in a tansparent way
  [x] -- Smooth soft pulse with interrupt timers
  [ ] -- White for publishing??
  [x] -- Hard pulse for ERROR modes
  [x] -- Solve led extra white bug (el bug estaba en wiring_digital.c en la funcion de pinmode, en la ultima version de arduino esta corregida (pero no en platformio))
  [x] -- Feedback on light setup

** RTC - TIME
  [x] -- Cuando !onTime dar feedback con el led
  [x] -- Resolver la integración de las alarmas de la libreria rtc (implemente mi propio timer)
  [x] -- recibir el time por luz
  [x] -- recibir el time por USB
  [x] -- recibir el time por webServer
  [ ] -- Get time from smartcitizen HTTP 
  [ ] -- Integrate Timelibrary to SAM code

** BUTTON
  [x] -- Robust interrupt for button
  [x] -- Detect Down, Up, and shortPress
  [x] -- Detect long and verylong

** READING-LIGHT-SOUND
  [x] -- Debug the new nonbloking version
  [ ] -- Probar en todas las pantallas posibles y en moviles
  [x] -- Hacer commit a github de la non blocking
  [x] -- Integrate library
  [x] -- Send esp command on succsesfull received credentials
  [x] -- Change led with checksum OK.
  [x] -- Only use it if Urban board is present.
  [x] -- Led feedback on stages: CRC, WIFI, PING, MQTT (solo poner verde si el CRC va bien) los errores de red se deben mostrar cuando cambies a Network mode
  [ ] -- Usar la implementación de lightRead para poner tambien la trasmision por sonido

** CONFIGURATION
  [x] -- Make hardcoded Default configuration
  [x] -- Save configuration on eeprom
  [ ] -- Get configuration from sdcard
  [x] -- get configuration from ESP
  [x] -- Hacer configurable el interval de readings y agregarlo al web
  [x] -- Interval readings por lightread
  [ ] -- Dar acceso a la configuracion por sensor (enabled e interval)
  [ ] -- save on epprom la configuracion de enabled o disabled para cada sensor
  [ ] -- Cuando estas en setup mode dar acceso a configurar por MQTT

** POWER MANAGMENT
  [x] -- Implementar el sleep para el SAM
  [ ] -- Status report on USB connected
  [ ] -- Status report on Battery connected
  [ ] -- Status report on charging (y en que MODE)
  [ ] -- buscar la manera de estar seguro que el sam esta dormido cuando no hay serialUSB connected
  [ ] -- Manejar los chargemodes
  [ ] -- Poner feedback en el led del nivel de bateria y carga
  [ ] -- Hacer pruebas de consumo
  [ ] -- Emergency mode with interrupt to wake up
  [ ] -- How on battery influences modes
  [ ] -- Diseñar bien la estrategia acerca del ESP y MICS
  [x] -- Integrar la tabla lookup para la bateria
  [ ] -- Implementar lookup table DINAMICA en la bateria
  [ ] -- logear en sdcard los cuatro canales del adc para entender como se comportan

** SDCARD
  [x] -- Encontrar la combinación entre ESP y SD para que el SPi no se cuelgue
  [x] -- Integrate SD library
  [x] -- Detect if sdcard is present
  [x] -- Guardar datos en sdcard siempre que se ejecute un publish y ponerles un flag de "publicacion exitosa"
  [ ] -- Modo de publicar todos los posts que no tengan el flag de publicacion exitosa
  [x] -- Feedback de error al entrar en modo sdcard si no esta presente
  [ ] -- debug events in sdcard
  [ ] -- Configuración de wifi a traves de sdcard, siempre tener un archivo de texto con las redes, que se puedan modificar, borrar, etc.
  [ ] -- Configuracion de token por sdcard, con un timestamp para solo actualizarlo si es mas nuevo que el que tenomos actualmente. hay que asociarlo a las credenciales wifi
  [ ] -- Flasheo del firmware por sdcard
  [ ] -- Configuracion de intervalo por sdcard
  [ ] -- Configuracion avanzada por sdcard
  [ ] -- Custom html en la sdcard para apmode


-----URBAN BOARD------

** DETECTION AND DIAGNOSIS

  [x] -- Detect if urban present

** SENSOR TEMPLATE

** SOUND
  [x] -- Basic average (dBc)

** TEMP AND HUMIDITY

** LIGHT
  [x] -- Implementar el get light
  [ ] -- Implementar el get UV

** GASES
  [ ] -- Implementar el get CO
  [ ] -- Implementar el get No2
  [ ] -- Hacer pruebas de consumo y plantear modos de uso

  ** ACCELEROMETER
  [ ] -- Crear una funcion get Acc
  [ ] -- Integrar el ACC a los sensores... y dar acceso en tiempo real?


-------Auxiliary devices----------
  [ ] -- Pensar el diseño para integrar librerias de diferentes dispositivos:
  			*** HAY QUE SIMPLIFICARLO PARA QUE SE PUEDAN AGREGAR GENERICAMENTE LOS SENSORES ***
  			hacer una lista de auxSensors, que contenga objetos para cada uno de los sensores habilitados. Para poder iterar sobre ella aun sin saber cuantos o cuales existan.
  			La idea sería tener compilado el soporte de los diferentes dispositivos y en la configuración habilitarlos dando el nombre del device y su dirección i2c.
  			Para esto necesito un wrapper en la libreria sckAux que sepa que sensores se habilitarian y que funciones para begin (que reporta error en caso de no encontrar el device) y read, por ejemplo si habilitas el sht31 debe poner sht31humedad y sht31temperatura.
  			En teoria en la libreria aux debo esconder todo lo necesario para leer un sensor (manejo de pots por ejemplo) y solo exponer las funciones begin y read.
  [x] -- Usar como primer ejemplo de lo anterior el SHT31
  [ ] -- Implementar en la configuracion una manera de prender los auxiliares
  [ ] -- Feedback de error en caso de que no se pueda iniciar un auxiliary device
  [ ] -- implement auxiliary device web page for enabling devicas via light
  [ ] -- the same but via web server
  [ ] -- Create a file in the sdcard with the list of supported devices where you can enable them.
  [ ] -- enable auxiliary devices via MQTT
  [x] -- Crear el dispositivo Auxiliar AlphasenseDelta con todos los sensores internos y demas. 
  [ ] -- Implementar el manejo del EPPROM de AlphaDelta
  [ ] -- Buscar la manera de tener un GPS como auxiliar
  [ ] -- Crear dispositivos para los diferentes devices de seeed studio

--------ESP8266-------
** Wifi
  [x] -- Recibir las credenciales x easyTransfer y guardarlas en flash
  [x] -- Responder al SAM con la red que se guardo como confirmacion
  [x] -- Si hay otra red con el mismo SSID sobreescribirla con la nueva HAY que encontrar una manera mas sencilla!!!!!
  [x] -- Poner un maximo de redes, e ir borrando la ultima cuando se agreguen nuevas
  [x] -- Implementar la funcion selectBestNetwork(), 
  [x] -- Enviar todas las redes configuradas al SAM
  [x] -- Borrar todas las redes
  [x] -- Al iniciar intenta conectarse y si falla pasar a modo ap
  			primero intentamos conectarnos a la ultima red de la lista, 
  			si no funciona hacemos un barrido y despues de conectarnos exitosamente a una, 
  			la movemos al final de la lista para evitar escaneos las proximas veces
  [x] -- AP mode
  [x] -- Json con la lista de ap escaneados entregado por el webserver
  [x] -- Json con la lista de redes configuradas, el token
  [x] -- Web server que reciba configuracion por GET
  [x] -- Pagina para el web server con ssid, pass y token
  [x] -- Agregar la lista de accesspoints a la pagina web
  [x] -- mDNS para webserver en station mode
  [x] -- Organizar el codigo del NTP
  [x] -- Portal captivo que redireccione al servidor del ESP
  [x] -- Conectar el lightread
  [x] -- Termninar y revisar el lightread non-bloking
  [x] -- Limpiar el lightREad non-blocking (ya funciona bien)
  [x] -- arreglar el bug que impide que funcione easytransfer cuando lightread enabled
  			parece que el problema es cualquier cosa que tarde mas de N ms, interrumpe el easytransfer y se pierden los paquetes
  			-una posibilidad seria forzar el receiveData con un timerinterrupt, esp arreglaria el problema independientemente de quien lo genere.

  [x] -- Poner una verificacion con loop hasta que sea exitoso para enviar los resultados del lightread, (wifi y token)
  [ ] -- Support open wifi networks
  [ ] -- Support different tokens for each wifi network (if no token is given assign the last one used)
  [x] -- Resolver la recepcion de datos de los sensores por easy transfer
  [ ] -- Hellow MQTT
  [ ] -- Get version from SAM and publish it (ver con guillem la recepcion por parte de la plataforma en /settings?)
  [ ] -- publicacion por mqtt con confirmacion al SAM de publicacion correcta.
  [ ] -- Hacer ping a la plataforma, después de conectarnos a wifi para saber si tenemos acceso a internet
  [ ] -- Integrar los intervals de posts para cada sensor en el json que se entrega en webShow
  [x] -- Agregar la opcion de set intervals por GET (en pagina set)
  [x] -- En la pagina web ponerle al token (optional), agrandar las letras y el boton, poner el logo (checar el diseño del onboarding).
  [ ] -- Implementar sync time por http con smartcitizen.me como fallback a ntp
  [ ] -- Cuando estamos en error poner la explicacion del error en el wab server de setupmode

** POSTS
  [x] -- MQTT
  [ ] -- Http (fallback)
  [ ] -- SSL

** NET DEBUG OUTPUT
  [ ] -- Net console (netcat)
  [ ] -- Errors via mqtt (hay que diseñar la clasificacion de errores)

** OTA Firmware upgrade
  [ ] -- OTA firmware upgrade del ESP (solo vale la pena si se puede desde la plataforma)
  [ ] -- NET Bridge para flashear el SAM con bossa???
  [ ] -- Flasheo del SAM desde flash??

** Power management
  [x] -- Ver que cambios de hardware son necesarios para despertar por interrupt externa, vale la pena? (por ahora con ESP_ON y ESP_OFF funciona bien) nohace falta cambiar nada, ya esta implementado (EXTCOM_ESP_WAKEUP);

*/
