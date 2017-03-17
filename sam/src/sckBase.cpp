#include "sckBase.h"
#include "sckUrban.h"


/* 	-------------------------
 	|	 Hardware Timers 	|
 	-------------------------
*/
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

/* 	---------------------------------------------------------
	|	Persistent Variables (eeprom emulation on flash)	|
	---------------------------------------------------------
*/
struct EppromMode {
	bool valid;
	SCKmodes mode;
};

struct EppromConf {
	bool valid;
	uint32_t readInterval;
};

FlashStorage(eppromMode, EppromMode);
FlashStorage(eppromConf, EppromConf);

// SAM <<>> ESP communication
EasyTransfer BUS_in, BUS_out;

/* 	----------------------------------
 	|	SmartCitizen Kit Baseboard   |
 	----------------------------------
*/
// Urban board
SckUrban urban;

// Sleepy dog
WatchdogSAMD wdt;

SdFat sd;
File publishFile;
File logFile;

// Auxiliary I2C devices
AlphaDelta alphaDelta;


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
	comTitles[EXTCOM_ESP_SERIAL_DEBUG_ON]	=	"esp serial debug on";
	comTitles[EXTCOM_ESP_SERIAL_DEBUG_OFF]	=	"esp serial debug off";
	comTitles[EXTCOM_ESP_LED_ON]			= 	"esp led on";
	comTitles[EXTCOM_ESP_LED_OFF]			= 	"esp led off";

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
	comTitles[EXTCOM_SET_TIME]		= 	"set time";			// @params: epoch time
	comTitles[EXTCOM_SYNC_TIME]		= 	"sync time";

	// SD card
	comTitles[EXTCOM_SD_PRESENT]	=	"sd present";


	// Sensor readings
	comTitles[EXTCOM_GET_TIME]			= 	"get time";			// @params: iso (default), epoch
	comTitles[EXTCOM_GET_APCOUNT]		=	"get apcount";
	comTitles[EXTCOM_GET_NOISE]			=	"get noise";
	comTitles[EXTCOM_GET_HUMIDITY]		=	"get humidity";
	comTitles[EXTCOM_GET_TEMPERATURE]	=	"get temperature";
	comTitles[EXTCOM_GET_BATTERY]		=	"get battery";
	comTitles[EXTCOM_GET_LIGHT]			=	"get light";
	comTitles[EXTCOM_GET_CO]			=	"get co";
	comTitles[EXTCOM_GET_NO2]			=	"get no2";
	comTitles[EXTCOM_GET_VOLTIN]		=	"get voltin";
	comTitles[EXTCOM_GET_ALPHADELTA]	=	"get alpha";
	comTitles[EXTCOM_ALPHADELTA_POT]	=	"set alpha";				// @ params: wichpot (AE1, WE1, AE2...), value (0-100,000)

	comTitles[EXTCOM_GET_CHAN0]			=	"get chann0";
	comTitles[EXTCOM_GET_CHAN1]			=	"get chann1";
	
	comTitles[EXTCOM_PUBLISH]		= 	"publish";

	// Other
	comTitles[EXTCOM_GET_APLIST]	= 	"get aplist";
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

	// Output level
	outputLevel = OUT_NORMAL;

	analogReadResolution(12);				// Set Analog resolution to MAX
	analogWriteResolution(8);

	// Setup sensors
	// TODO read config for setting intervals and enable sensors
	sensors.enabled[SENSOR_TIME] 			= true;
	sensors.interval[SENSOR_TIME] 			= configuration.readInterval;

	sensors.enabled[SENSOR_NETWORKS] 		= true;
	sensors.interval[SENSOR_NETWORKS] 		= configuration.readInterval;

	sensors.enabled[SENSOR_NOISE] 			= true;
	sensors.interval[SENSOR_NOISE] 			= configuration.readInterval;

	sensors.enabled[SENSOR_HUMIDITY] 		= true;
	sensors.interval[SENSOR_HUMIDITY] 		= configuration.readInterval;

	sensors.enabled[SENSOR_TEMPERATURE] 	= true;
	sensors.interval[SENSOR_TEMPERATURE] 	= configuration.readInterval;

	sensors.enabled[SENSOR_BATTERY] 		= true;
	sensors.interval[SENSOR_BATTERY] 		= configuration.readInterval;

	sensors.enabled[SENSOR_LIGHT] 			= true;
	sensors.interval[SENSOR_LIGHT] 			= configuration.readInterval;

	sensors.enabled[SENSOR_CO] 				= false;				// Disabled for now
	sensors.interval[SENSOR_CO] 			= 0;				// Disabled for now
	
	sensors.enabled[SENSOR_NO2] 			= false;				// Disabled for now
	sensors.interval[SENSOR_NO2] 			= 0;				// Disabled for now

	sensors.enabled[SENSOR_VOLTIN] 			= false;				// Disabled for now
	sensors.interval[SENSOR_VOLTIN] 		= 0;				// Disabled for now

	// -------------------------------------
	// Auxiliary Devices
	// -------------------------------------
	// AlphasenseDelta
	if (alphaDelta.begin()) {
		sensors.enabled[SENSOR_ALPHADELTA_AE1]			= true;
		sensors.interval[SENSOR_ALPHADELTA_AE1]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_WE1]			= true;
		sensors.interval[SENSOR_ALPHADELTA_WE1]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_AE2]			= true;
		sensors.interval[SENSOR_ALPHADELTA_AE2]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_WE2]			= true;
		sensors.interval[SENSOR_ALPHADELTA_WE2]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_AE3]			= true;
		sensors.interval[SENSOR_ALPHADELTA_AE3]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_WE3]			= true;
		sensors.interval[SENSOR_ALPHADELTA_WE3]			= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_TEMPERATURE]	= true;
		sensors.interval[SENSOR_ALPHADELTA_TEMPERATURE]	= configuration.readInterval;
		sensors.enabled[SENSOR_ALPHADELTA_HUMIDITY]		= true;
		sensors.interval[SENSOR_ALPHADELTA_HUMIDITY]	= configuration.readInterval;
	}


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
	if (configuration.readInterval == 0) configuration.readInterval = 60;		// Sanity check default


	if (!urbanPresent) changeMode(MODE_ERROR);
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
								lightResults.lines[3].toCharArray(token, 64);
								sendToken();
							}
							if (lightResults.lines[4].toInt() > 0 && lightResults.lines[4].toInt() < ONE_DAY_IN_SECONDS) {
								configuration.readInterval = lightResults.lines[4].toInt();
								sckOut(String F("New reading interval: ") + String(configuration.readInterval));
							}
						}
						if (lightResults.lines[0].endsWith(F("time"))) {
							setTime(lightResults.lines[1]);
						}
					 	readLight.reset();
					}
				}
			}
			

		// //----------------------------------------
		// // 	MODE_NET TODO SOLO checar si lleva demasiado tiempo prendido el wifi y apagarlo.
		// //----------------------------------------
		} else if (mode == MODE_NET) {
			// if (millis() - lastPublishTime > configuration.readInterval * 1000 * 5) softReset();	// Something went wrong!! RESET
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
	ESPqueueMsg(true, true);
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

void SckBase::changeMode(SCKmodes newMode) {

	uint8_t pulseMode = 0;

	timerClear(ACTION_PUBLISH);

	// if (newMode != mode) {

		// Restore previous output level
		if (mode == MODE_BRIDGE) changeOutputLevel(prevOutputLevel);

		switch(newMode) {
			case MODE_AP: {
				// Start ESP and ap mode
				timerSet(ACTION_ESP_ON, 50);
				// Start monitoring light for messages
				sckOut(F("Entering AP mode!"));
				// Restart lightread for receiving new data
				// readLight.reset();
				// lightResults.commited = false;
				break;

			} case MODE_NET: {
				
				sensors.enabled[SENSOR_NETWORKS] = true;

				sckOut(F("Entering Network mode!"));
				// If not connected to wifi restart ESP without ap mode
				// TODO do checks for net mode and setup intervals
				// if (!onWifi) timerSet(ACTION_ESP_REBOOT, 50);
				// timerSet(ACTION_PUBLISH, configuration.readInterval, true);
				// sckOut(F("Entering Network mode, publishing every ")) + String(configuration.readInterval) + "seconds";
				break;

			} case MODE_SD: {
				sensors.enabled[SENSOR_NETWORKS] = false;
				ESPcontrol(ESP_OFF);
				sckOut(F("Entering SD card mode!"));
				timerSet(ACTION_CHECK_SD, 50);
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

		// sckOut(String F("Changing mode to ") + modeTitles[newMode] + F(" mode from ") + modeTitles[mode] + F(" mode"));
		prevMode = mode;
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

		if (newMode == MODE_OFF) goToSleep();		// This must be at the end so the ret get executed before goig to sleep
	// }	
}

void SckBase::changeOutputLevel(OutLevels newLevel) {
	prevOutputLevel = outputLevel;
	outputLevel = newLevel;
}

void SckBase::inputUpdate() {

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

	// ESP Serial debug
	if (espSerialDebug) {
		if (Serial1.available()) SerialUSB.write(Serial1.read());
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
				sdPresent();//-------------------
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
		msgOut.time = BUS_queue[0].time;
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
				}
			}
		}
		// Asume message sent and received
		if (!waitAnswer || mesgReceived) {
			// Remove first message from queue and move the rest
			for (uint8_t i=1; i<=BUS_queueIndex; i++){
				BUS_queue[i-1].time = BUS_queue[i].time;
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

	// Put time in message
	BUS_queue[BUS_queueIndex].time = rtc.getEpoch();

	// Do we need to wait answer for this message??
	if (waitAnswer) BUS_queue[BUS_queueIndex].waitAnswer = true;
	else BUS_queue[BUS_queueIndex].waitAnswer = false;

	// If no need to send params
	if (!sendParam) strncpy(BUS_queue[BUS_queueIndex].param, "", 240);
	else strncpy(BUS_queue[BUS_queueIndex].param, msgBuff.param, 240);	
}

void SckBase::ESPprocessMsg() {

	sckOut(F("Processing message from ESP..."), PRIO_LOW);
	sckOut(String F("Epoch time: ") + String(msgIn.time), PRIO_LOW);
	sckOut(String F("Command: ") + String(msgIn.com), PRIO_LOW);
	sckOut(String F("Parameters: ") + String(msgIn.param), PRIO_LOW);

	switch(msgIn.com) {
		case ESP_BOOTED_AND_READY: {
			ESPbooting = false;
			ESPon = true;
			sckOut(F("ESP ready!!!"), PRIO_LOW);
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
			configuration.readInterval = jsonConf["ri"];
			saveConf();
			sckOut(F("Configuration updated:"));
			sckOut(String F("Reading interval: ") + String(configuration.readInterval));
			prompt();
			break;

		} case ESP_GET_APCOUNT_COM: {
			sensors.readings[SENSOR_NETWORKS].valid = true;
			sensors.readings[SENSOR_NETWORKS].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_NETWORKS].value = atof(msgIn.param);
			sckOut(String(msgIn.param));
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

		} case ESP_GET_STATUS_COM: {
			processStatus();
			break;

		}
	}

	// Clear msg
	msgIn.time = 0;
	msgIn.com = 0;
	strncpy(msgIn.param, "", 240);
}

void SckBase::getStatus() {

	msgBuff.com = ESP_GET_STATUS_COM;
	strncpy(msgBuff.param, "", 240);
	ESPqueueMsg(false);
}

void SckBase::processStatus() {

	// Decode json
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);

	// Dump to the struct
	espStatus.wifi 	= jsonNet["wifi"];
	espStatus.net 	= jsonNet["net"];
	espStatus.mqtt 	= jsonNet["mqtt"];
	espStatus.time 	= jsonNet["time"];
	espStatus.ap 	= jsonNet["ap"];
	espStatus.web 	= jsonNet["web"];
	espStatus.conf 	= jsonNet["conf"];

	// Wifi status has changed
	if (espStatus.wifi != prevEspStatus.wifi) {
		switch (espStatus.wifi) {
			case ESP_WIFI_CONNECTED_EVENT: {
				sckOut(F("Conected to wifi!!"));
				onWifi = true;
				break;

			} case ESP_WIFI_ERROR_EVENT: {
				sckOut(F("Wifi ERROR: undefined!!"));
				onWifi = false;
				if (mode == MODE_NET) led.update(mode, 2);
				break;

			} case ESP_WIFI_ERROR_PASS_EVENT: {
				sckOut(F("Wifi ERROR: wrong password!!"));
				onWifi = false;
				if (mode == MODE_NET) led.update(mode, 2);
				break;

			} case ESP_WIFI_ERROR_AP_EVENT: {
				sckOut(F("Wifi ERROR: can't find access point!!"));
				onWifi = false;
				if (mode == MODE_NET) led.update(mode, 2);
				break;

			}
		} 

	}

	// Net status has changed
	if (espStatus.net != prevEspStatus.net) {

	}

	// Mqtt status has changed
	if (espStatus.mqtt != prevEspStatus.mqtt) {

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
				ESPqueueMsg(false, true);
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
			ESPqueueMsg(false, true);
			
		}
	}

	prevEspStatus = espStatus;
}



bool SckBase::ESPpublish() {
	return false;
}


// void SckBase::ESPpublish() {
// 	// hay que buscar una libreria json encode
// 	const String comToSend PROGMEM = "sck.publish(\"{\\\"time\\\":\\\"" + readings.time + "\\\",\\\"noise\\\":\\\"" + String(readings.noise.data, 2) + "\\\",\\\"temperature\\\":\\\"" + String(readings.temperature.data, 2) + "\\\",\\\"humidity\\\":\\\"" + String(readings.humidity.data, 2) + "\\\",\\\"battery\\\":\\\"" + String(readings.battery.data) + "\\\"}\")";
// 	lastPublishTime = millis();
// 	String answer = ESPsendCommand(comToSend);
// }


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

			if (newPass.length() < 8) {
				sckOut(F("Wifi password should have at least 8 characters!!!"), PRIO_HIGH);
			} else {

				newSsid.toCharArray(credentials.ssid, 64);
				newPass.toCharArray(credentials.password, 64);
				sendNetwork();
			}
			break;

		} case EXTCOM_GET_WIFI: {
			msgBuff.com = ESP_GET_WIFI_COM;
			ESPqueueMsg(false, true);
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
			ESPqueueMsg(false, true);
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
			strncpy(token, "null", 64);
			sendToken();
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
					configuration.readInterval = intTinterval;
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

			readLightEnabled = true;
			break;

		} case EXTCOM_READLIGHT_OFF: {

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
			sdPresent();
			break;

		// Sensor readings
		} case EXTCOM_GET_APCOUNT: {
			msgBuff.com = ESP_GET_APCOUNT_COM;
			ESPqueueMsg(false);
			break;

		} case EXTCOM_GET_NOISE: {
			sckOut(sensors.titles[SENSOR_NOISE] + ": " + urban.getNoise() + " " + sensors.units[SENSOR_NOISE], PRIO_HIGH);
			break;

		} case EXTCOM_GET_HUMIDITY: {
			sckOut(sensors.titles[SENSOR_HUMIDITY] + ": " + urban.getHumidity() + " " + sensors.units[SENSOR_HUMIDITY], PRIO_HIGH);
			break;

		} case EXTCOM_GET_TEMPERATURE: {
			sckOut(sensors.titles[SENSOR_TEMPERATURE] + ": " + urban.getTemperature() + " " + sensors.units[SENSOR_TEMPERATURE], PRIO_HIGH);
			break;

		} case EXTCOM_GET_BATTERY: {
			// sckOut(sensors.titles[SENSOR_BATTERY] + ": " + getBatteryVoltage()/1000. + " " + sensors.units[SENSOR_BATTERY], PRIO_HIGH);
			sckOut(sensors.titles[SENSOR_BATTERY] + ": " + getBatteryVoltage()/1000. + " V", PRIO_HIGH);
			break;

		} case EXTCOM_GET_LIGHT: {
			sckOut(sensors.titles[SENSOR_LIGHT] + ": " + urban.getLight() + " " + sensors.units[SENSOR_LIGHT], PRIO_HIGH);
			break;

		} case EXTCOM_GET_VOLTIN: {
			sckOut(sensors.titles[SENSOR_VOLTIN] + ": " + getCharger() + " " + sensors.units[SENSOR_VOLTIN], PRIO_HIGH);
			break;

		} case EXTCOM_GET_CO: {
			// sckOut(sensors.titles[SENSOR_CO] + ": " + getCO() + " " + sensors.units[SENSOR_CO], PRIO_HIGH);
			break;

		} case EXTCOM_GET_NO2: {
			// sckOut(sensors.titles[SENSOR_NO2] + ": " + getNO2() + " " + sensors.units[SENSOR_NO2], PRIO_HIGH);
			break;

		} case EXTCOM_GET_ALPHADELTA: {

			SensorType sensorsToShow[] = {
				SENSOR_ALPHADELTA_AE1,
				SENSOR_ALPHADELTA_WE1,
				SENSOR_ALPHADELTA_AE2,
				SENSOR_ALPHADELTA_WE2,
				SENSOR_ALPHADELTA_AE3,
				SENSOR_ALPHADELTA_WE3,
				SENSOR_ALPHADELTA_TEMPERATURE,
				SENSOR_ALPHADELTA_HUMIDITY,
			};

			for (uint8_t i=0; i<8; i++) {
				
				sckOut(String(sensors.titles[sensorsToShow[i]] + ": "), PRIO_HIGH, false);

				String readResult;
				if (sensors.enabled[sensorsToShow[i]]) {
					switch(sensorsToShow[i]) {
						case SENSOR_ALPHADELTA_TEMPERATURE: readResult = alphaDelta.getTemperature(); break;
						case SENSOR_ALPHADELTA_HUMIDITY: 	readResult = alphaDelta.getHumidity(); break;
						case SENSOR_ALPHADELTA_AE1: 		readResult = alphaDelta.getElectrode(alphaDelta.AE_1); break;
						case SENSOR_ALPHADELTA_WE1:			readResult = alphaDelta.getElectrode(alphaDelta.WE_1); break;
						case SENSOR_ALPHADELTA_AE2:			readResult = alphaDelta.getElectrode(alphaDelta.AE_2); break;
						case SENSOR_ALPHADELTA_WE2:			readResult = alphaDelta.getElectrode(alphaDelta.WE_2); break;
						case SENSOR_ALPHADELTA_AE3:			readResult = alphaDelta.getElectrode(alphaDelta.AE_3); break;
						case SENSOR_ALPHADELTA_WE3:			readResult = alphaDelta.getElectrode(alphaDelta.WE_3); break;
					}
					sckOut(readResult + " " + sensors.units[sensorsToShow[i]]);

				} else {
					sckOut(F("is disabled"));
				}
			}
			sckOut(String F("POT AE1: ") + String(alphaDelta.getPot(alphaDelta.POT_AE1)) + F(" ohms"));
			sckOut(String F("POT WE1: ") + String(alphaDelta.getPot(alphaDelta.POT_WE1)) + F(" ohms"));
			sckOut(String F("POT AE2: ") + String(alphaDelta.getPot(alphaDelta.POT_AE2)) + F(" ohms"));
			sckOut(String F("POT WE2: ") + String(alphaDelta.getPot(alphaDelta.POT_WE2)) + F(" ohms"));
			sckOut(String F("POT AE3: ") + String(alphaDelta.getPot(alphaDelta.POT_AE3)) + F(" ohms"));
			sckOut(String F("POT WE3: ") + String(alphaDelta.getPot(alphaDelta.POT_WE3)) + F(" ohms"));
			break;

		} case EXTCOM_ALPHADELTA_POT: {

			// Wich resistor?
			String strPOT = strIn.substring(0,2);
			Resistor wichPot;
			if (strIn.startsWith("AE1")) wichPot = alphaDelta.POT_AE1;
			else if (strIn.startsWith("WE1")) wichPot = alphaDelta.POT_WE1;
			else if (strIn.startsWith("AE2")) wichPot = alphaDelta.POT_AE2;
			else if (strIn.startsWith("WE2")) wichPot = alphaDelta.POT_WE2;
			else if (strIn.startsWith("AE3")) wichPot = alphaDelta.POT_AE3;
			else if (strIn.startsWith("WE3")) wichPot = alphaDelta.POT_WE3;

			// Which value;
			strIn.remove(0,4);
			strIn.trim();
			uint32_t wichValue = strIn.toInt();
			sckOut(String F("Setting ") + strPOT + F(" to ") + strIn + F(" ohms"));
			alphaDelta.setPot(wichPot, wichValue);

			break;

		} case EXTCOM_GET_CHAN0: {
			sckOut(String F("chan0: ") + getChann0() + F(" V"), PRIO_HIGH);
			break;

		} case EXTCOM_GET_CHAN1: {
			sckOut(String F("chan1: ") + getChann1() + F(" V"), PRIO_HIGH);
			break;

		// Other
		} case EXTCOM_PUBLISH: {
			sensorReadAll();
			break;

		// Help
		} case EXTCOM_HELP: {
			sckOut("", PRIO_HIGH);
			for (uint8_t i=0; i<EXTCOM_COUNT-1; i+=3) {
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

	if (outputLevel + priority > 1) {
		SerialUSB.print(strOut);
		if (newLine) SerialUSB.println();
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
		changeMode(mode);
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
void SckBase::sensorReadAll() {

	sensors.ready = false;
	sckOut("Reading sensors...");
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType index = static_cast<SensorType>(i);

		if (sensors.enabled[index]) {
			sensorRead(index);
		}
	}

	timerSet(ACTION_READING_FINISHED, 100, true);
}
void SckBase::sensorRead(SensorType toRead) {

	switch(toRead) {

		case SENSOR_TIME: {

			sensors.readings[SENSOR_TIME].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_TIME].value = 0;							// We use the last reading time as value for this sensor (no need to cast to float)
			sensors.readings[SENSOR_TIME].valid = true;
			break;

		} case SENSOR_NETWORKS: {
			msgBuff.com = ESP_GET_APCOUNT_COM;
			ESPqueueMsg(false, true);
			break;

		} case SENSOR_NOISE: {
			sensors.readings[SENSOR_NOISE].value = urban.getNoise();
			sensors.readings[SENSOR_NOISE].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_NOISE].valid = true;
			break;

		} case SENSOR_HUMIDITY: {
			sensors.readings[SENSOR_HUMIDITY].value = urban.getHumidity();
			sensors.readings[SENSOR_HUMIDITY].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_HUMIDITY].valid = true;
			break;

		} case SENSOR_TEMPERATURE: {
			sensors.readings[SENSOR_TEMPERATURE].value = urban.getTemperature();
			sensors.readings[SENSOR_TEMPERATURE].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_TEMPERATURE].valid = true;
			break;

		} case SENSOR_BATTERY: {
			sensors.readings[SENSOR_BATTERY].value = getBatteryVoltage();
			sensors.readings[SENSOR_BATTERY].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_BATTERY].valid = true;
			break;

		} case SENSOR_LIGHT: {
			sensors.readings[SENSOR_LIGHT].value = urban.getLight();
			sensors.readings[SENSOR_LIGHT].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_LIGHT].valid = true;
			break;

		} case SENSOR_CO: {

			break;

		} case SENSOR_NO2: {

			break;

		} case SENSOR_VOLTIN: {
			sensors.readings[SENSOR_VOLTIN].value = getCharger();
			sensors.readings[SENSOR_VOLTIN].lastReadingTime = rtc.getEpoch();
			sensors.readings[SENSOR_VOLTIN].valid = true;
			break;
		}
	}
}
bool SckBase::readingFinished() {
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType index = static_cast<SensorType>(i);
		
		// If sensor IS enabled check if reading is finished
		if (sensors.enabled[index] && sensors.interval[index] > 0) {
			if (!sensors.readings[index].valid) return false;
			if (rtc.getEpoch() - sensors.readings[index].lastReadingTime > READING_MAX_TIME) return false;
		}
	}
	return true;
}
void SckBase::sensorPublish() {

	bool platformPublishedOK = false;

	if (mode == MODE_NET) {
		if (ESPpublish()) platformPublishedOK = true;
	}
	publishToSD(platformPublishedOK);
}
void SckBase::saveConf() {

	EppromConf toSaveConf;
	toSaveConf.valid = true;
	toSaveConf.readInterval = configuration.readInterval;
	eppromConf.write(toSaveConf);
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
	sckOut(String F("Button very long press: ") + String(millis() - butLastEvent), PRIO_MED);
	// Factory reset
	factoryReset();
}

void SckBase::longPress() {
	sckOut(String F("Button long press: ") + String(millis() - butLastEvent), PRIO_MED);
	changeMode(MODE_OFF);
}

void SckBase::softReset() {
 	wdt.enable(10);
}

/* 	-----------------
 	|	 SD card	|
 	-----------------
*/
bool SckBase::sdPresent() {

	if (sd.cardBegin(CS_SDCARD, SPI_HALF_SPEED)) {
		sckOut(F("Sdcard ready!!"), PRIO_LOW);
		if (mode == MODE_SD) {
			led.update(mode, 0);
		}
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		if (mode == MODE_SD) {
			led.update(mode, 2);
			timerSet(ACTION_CHECK_SD, 500);
		}
		return false;
	}
}

bool SckBase::openPublishFile() {

	ESPcontrol(ESP_OFF);

	char charFileName[publishFileName.length()];

	bool writeHeader = false;

	if (sdPresent()) {

		sd.begin(CS_SDCARD);
		
		uint8_t fi = 1;
		while (fi < 128) {

			publishFileName.toCharArray(charFileName, publishFileName.length() + 1);

			// If file doesn't exist we need to write header
			if (!sd.exists(charFileName)) writeHeader = true;

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
							SensorType index = static_cast<SensorType>(i);
							publishFile.print(sensors.titles[index]);
							
							// In this ones don't put units
							if (index == SENSOR_TIME || SENSOR_NETWORKS) {
								publishFile.print("-");
								publishFile.print(sensors.units[index]);	
							}
							if (index < SENSOR_COUNT) publishFile.print(",");
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

bool SckBase::publishToSD(bool platformPublishedOK) {

	if (openPublishFile()) {
		
		// Write down the platform publish status of this reading
		if (platformPublishedOK) publishFile.print("1,");
		else publishFile.print("0,");

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType index = static_cast<SensorType>(i);

			// Time need a special treatment
			if (index == SENSOR_TIME) {
				publishFile.print(epoch2iso(sensors.readings[index].lastReadingTime));
			}
			// This sensor are rounded
			else if (index == SENSOR_NETWORKS || index == SENSOR_BATTERY)
				publishFile.print((int)sensors.readings[index].value);

			// This are published as float
			else
				publishFile.print(sensors.readings[index].value);
			if (index < SENSOR_COUNT) publishFile.print(",");
		}
		publishFile.println("");
		publishFile.close();
		sckOut(F("Readings published!!"));
		return true;
	} else {
		if (mode == MODE_SD) {
			led.update(mode, 2);
		}
	}
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
  	while (USB->DEVICE.SYNCBUSY.bit.SWRST | USB->DEVICE.CTRLA.bit.SWRST == 1);

	USBDevice.detach();

	digitalWrite(25, HIGH);
	digitalWrite(26, HIGH);

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

	strncpy(credentials.ssid, "ssid", 64);
	strncpy(credentials.password, "password", 64);
	credentials.time = 0;
	clearNetworks();

	strncpy(token, "null", 64);
	sendToken();

	// Save mode for next reboot
	EppromMode toSaveMode;
	toSaveMode.valid = true;
	toSaveMode.mode = MODE_AP;
	eppromMode.write(toSaveMode);

	// Reset configuration
	configuration.readInterval = 60;
	saveConf();

	// Set a periodic tmer for reset when ESP comunication (clear wifi and token) is complete
	timerSet(ACTION_FACTORY_RESET, 500, true);
}


/* 	-------------
 	|	 Power management (por ahora es cun copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
 	-------------
*/
uint16_t SckBase::getBatteryVoltage() {
  uint16_t batVoltage = 2*(readADC(3))*VCC/RESOLUTION_ANALOG;
  return batVoltage;
}

uint16_t SckBase::getCharger() {
  uint16_t temp = 2*(readADC(2))*VCC/RESOLUTION_ANALOG;
  return temp;
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
		case MODE_AP:
			currentPulse = pulseRed;
			break;
		case MODE_NET:
			currentPulse = pulseBlue;
			break;
		case MODE_SD:
			currentPulse = pulsePink;
			break;
		case MODE_SHELL:
			ledRGBcolor = orangeRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_ERROR:
			ledRGBcolor = redRGB;
			break;
		case MODE_FLASH:
			ledRGBcolor = lightBlueRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_BRIDGE:
			ledRGBcolor = lightGreenRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_FIRST_BOOT:
			ledRGBcolor = yellowRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_OFF:
			ledRGBcolor = offRGB;
			pulseMode = PULSE_STATIC;
			break;
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
			if (ledRGBcolor.r == offRGB.r && ledRGBcolor.g == offRGB.g && ledRGBcolor.b == offRGB.b) ledRGBcolor = currentPulse[24];
			else ledRGBcolor = offRGB; 
		}

	} else if (pulseMode == PULSE_HARD_FAST) {

		if (millis() - hardTimer > fastHard) {
			hardTimer = millis();
			if (ledRGBcolor.r == offRGB.r && ledRGBcolor.g == offRGB.g && ledRGBcolor.b == offRGB.b) ledRGBcolor = currentPulse[24];
			else ledRGBcolor = offRGB; 
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


/* 	-------------
 	|	 Timer 	|
 	-------------
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

					} case ACTION_CHECK_SD: {
						if (sdPresent()) {
							if (!onTime) {
								led.update(mode, 1);
								sckOut(F("Error: RTC out of time!!!"));
							} else {
								timerSet(ACTION_PUBLISH, configuration.readInterval*1000, true);
								sckOut(String F("Publishing every ") + String((int)configuration.readInterval) + " seconds");
								sensorReadAll();
								led.update(mode, 0);
							}
						}
						
						break;

					} case ACTION_GET_ESP_STATUS: {
						getStatus();
						break;
					
					} case ACTION_LONG_PRESS: {
						longPress();
						break;

					} case ACTION_VERY_LONG_PRESS: {
						veryLongPress();
						break;

					} case ACTION_FACTORY_RESET: {
						if (BUS_queueIndex == 0) softReset();
						break;

					} case ACTION_READING_FINISHED: {
						if (readingFinished()) {
							timerClear(ACTION_READING_FINISHED);
							sckOut("\n--------------------------");
							for (uint8_t i=0; i<SENSOR_COUNT; i++) {
								SensorType index = static_cast<SensorType>(i);
								if (index == SENSOR_TIME) 
									sckOut(sensors.titles[index] + ": " + epoch2iso(sensors.readings[index].lastReadingTime) + " " + sensors.units[index], PRIO_HIGH);
								else if (index == SENSOR_NETWORKS || index == SENSOR_BATTERY) 
									sckOut(sensors.titles[index] + ": " + String((int)sensors.readings[index].value) + " " + sensors.units[index], PRIO_HIGH);
								else 
									sckOut(sensors.titles[index] + ": " + String(sensors.readings[index].value) + " " + sensors.units[index], PRIO_HIGH);
							}
							sckOut("--------------------------");
							sensorPublish();
						}
						break;

					} case ACTION_PUBLISH: {
						if (!timerExists(ACTION_READING_FINISHED)) sensorReadAll();
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

void SckBase::timerSet(TimerAction action, uint16_t interval, bool isPeriodic) {

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
   data = (int)(value/kr);
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
   return readI2C(POT, ADDR)*kr;
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

BUGS
-- get best wifi via serial dont return anything
-- cuando hago un publish o get readings aparece el numero de nets antes del output (en la consola)
-- a veces el webserver necesita un reload para mostrar la lista de aps
-- un timeout despues de prender el esp para apagarlo, asumir error de conexion y esperar al siguiente intervalo de post
-- checar que el usb se apague (detach) cuando no este conectado
-- habilitar el MICS
-- Poner la curva de bateria
-- a veces al entrar en modo ap hay un par de blinks verdes dentro del pulse rojo
-- en modo SD si quito la card y la reinserto no se da cuenta rapido, segun yo hay un chequeo cada 500 ms, revisarlo.


** STATE MACHINE DESIGN (MODES)
  --it's important to get a very simple design
  --Make a list of triggers that changes the mode
  [x] -- Revisar bien el flow de eventos y excepciones para cada modo
  [x] -- Poner los cambios de modo con el boton
  [x] -- handle wifi errors without entering in setup mode (at least not instantly to avoid confusion on temporal network errors)
  [ ] -- In APmode when succsesfull setted kit (by any means: light, webbrowser, etc) the light will go to green, if you want to set another thing you have to manually cycle button to start again the ap-setup mode
  [ ] -- Unify nomenclatures and call always "setup mode" instead of apmode. Reserve apmode for esp wifi-ap mode.

** INPUT-OUTPUT
  [x] -- sckIn receives a struct with a string and the origin (USB, ESP, LIGHT, etc), process the command, and outputs via sckOut
  [x] -- sckOut recibe un string con el String y lo redirecciona a las outputs disponibles (USB,ESP, SDCARD), dependiendo del nivel de output y de la prioridad del mensaje
  [x] -- Create an array to store command titles saving all the strings in flash
  [x] -- FANCY in the case of interactive inputs, get a prompt!! ej. >, echo of what youre typing and a welcome message with some info.
  [x] -- Avoid hangs with serial port on sleep
  [ ] -- detect if usb serial is connected, and avoid serial communications if there is no usb connected (mejor aun si no hay conexion de datos)

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
  [ ] -- Asociar el Token al wifi con el que se dio de alta, si se da de alta un wifi sin token tomar el ultimo usado, lo mismo a la inversa.
  [ ] -- Reimplementar el esp publish
  [ ] -- Si no tengo un token marco error con el led

** LED
  [x] -- Automatic change depending on mode
  [x] -- Mejorar los fades y colores
  [x] -- Integrate HSI and RGB colors in a tansparent way
  [x] -- Smooth soft pulse with interrupt timers
  [ ] -- White for publishing??
  [x] -- Hard pulse for ERROR modes
  [x] -- Solve led extra white bug (el bug estaba en wiring_digital.c en la funcion de pinmode, en la ultima version de arduino esta corregida (pero no en platformio))
  [ ] -- Feedback on light setup

** RTC - TIME
  [x] -- Cuando !onTime dar feedback con el led
  [x] -- Resolver la integración de las alarmas de la libreria rtc (implemente mi propio timer)
  [x] -- recibir el time por luz
  [x] -- recibir el time por USB
  [x] -- recibir el time por webServer

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
  [ ] -- Led feedback on stages: CRC, WIFI, PING, MQTT
  [ ] -- Usar la implementación de lightRead para poner tambien la trasmision por sonido

** CONFIGURATION
  [x] -- Make hardcoded Default configuration
  [x] -- Save configuration on eeprom
  [ ] -- Get configuration from sdcard
  [x] -- get configuration from ESP
  [x] -- Hacer configurable el interval de readings y agregarlo al web
  [x] -- Interval readings por lightread
  [ ] -- Dar acceso a la configuracion por sensor (enabled e interval)

** POWER MANAGMENT
  [x] -- Implementar el sleep para el SAM
  [ ] -- buscar la manera de estar seguro que el sam esta dormido cuando no hay serialUSB connected
  [ ] -- Manejar los chargemodes
  [ ] -- Poner feedback en el led del nivel de bateria y carga
  [ ] -- Hacer pruebas de consumo
  [ ] -- Emergency mode with interrupt to wake up
  [ ] -- How on battery influences modes
  [ ] -- Diseñar bien la estrategia acerca del ESP y MICS
  [ ] -- Implementar lookup table dinamica en la bateria
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
  [ ] -- Usar como primer ejemplo de lo anterior el SHT31
  [ ] -- Implementar en la configuracion una manera de prender los auxiliares
  [ ] -- Feedback de error en caso de que no se pueda iniciar un auxiliary device
  [ ] -- implement auxiliary device web page for enabling devicas via light
  [ ] -- the same but via web server
  [ ] -- Create a file in the sdcard with the list of supported devices where you can enable them.
  [ ] -- enable auxiliary devices via MQTT
  [ ] -- Crear el dispositivo Auxiliar AlphasenseDelta con todos los sensores internos y demas.
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
  [ ] -- Resolver la recepcion de datos de los sensores por easy transfer
  [ ] -- publicacion por mqtt con confirmacion al SAM de publicacion correcta.
  [ ] -- Hacer ping a la plataforma, después de conectarnos a wifi para saber si tenemos acceso a internet
  [ ] -- Integrar los intervals de posts para cada sensor en el json que se entrega en webShow
  [x] -- Agregar la opcion de set intervals por GET (en pagina set)
  [x] -- En la pagina web ponerle al token (optional), agrandar las letras y el boton, poner el logo (checar el diseño del onboarding).
  [ ] -- Implementar sync time por http con smartcitizen.me como fallback a ntp

** POSTS
  [ ] -- MQTT
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
