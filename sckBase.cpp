#include "sckBase.h"


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
typedef struct {
	bool valid;
	char ssid[64];
	char pass[64];
} Credentials;

typedef struct {
	bool valid;
	char token[64];
} Token;

FlashStorage(eppromMode, SCKmodes);
FlashStorage(eppromCred, Credentials);
FlashStorage(eppromToken, Token);


/* 	----------------------------------
 	|	SmartCitizen Kit Baseboard   |
 	----------------------------------
*/
void SckBase::setup() {

	// Serial Ports Configuration
	Serial1.begin(baudrate);
	SerialUSB.begin(baudrate);


	// ESP Configuration
	pinMode(POWER_WIFI, OUTPUT);
	pinMode(CH_PD, OUTPUT);
	pinMode(GPIO0, OUTPUT);
	digitalWrite(POWER_WIFI, HIGH);
	ESPcontrol(ESP_OFF);


	String buildDate = __DATE__;
	buildDate.replace(' ', '-');
	version += buildDate + '-' + String(__TIME__) + "-ALPHA";

	modeTitles[MODE_AP]			= 	"Ap mode";
	modeTitles[MODE_NET] 		=	"Network mode";
	modeTitles[MODE_SD] 		= 	"SD card mode";
	modeTitles[MODE_SHELL] 		= 	"Shell mode";
	modeTitles[MODE_FLASH] 		= 	"ESP flashing mode";
	modeTitles[MODE_BRIDGE] 	= 	"ESP bridging mode";
	modeTitles[MODE_ERROR] 		= 	"Error mode";
	modeTitles[MODE_FIRST_BOOT] = 	"First boot mode";
	modeTitles[MODE_OFF]		=	"Off mode";

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

	// SPI Configuration
	// pinMode(MOSI, OUTPUT);
	// pinMode(SCK, OUTPUT);
  	// pinMode(MISO, INPUT);
	// pinMode(CS_SDCARD, OUTPUT);
	// digitalWrite(CS_SDCARD, LOW);

	pinMode(CS_ESP, OUTPUT);
	digitalWrite(CS_ESP, HIGH);		// Disable ESP SPI
	pinMode(SS, OUTPUT);

	// Power management configuration
  	pinMode(PS, OUTPUT);
  	digitalWrite(PS, HIGH);
  	writeCurrent(200); 
	
 	// Peripheral setup
 	rtc.begin();
	button.setup();
	led.setup();

	// Output level
	outputLevel = OUT_NORMAL;
	outputLevel = OUT_VERBOSE;				// Remove for production

	analogReadResolution(12);				// Set Analog resolution to MAX

	SCKmodes previousMode = eppromMode.read();
	if (previousMode) changeMode(previousMode);
	else changeMode(MODE_OFF);
	eppromMode.write(mode);

};

void SckBase::update() {

	// Button Stuff
	if (button.isDown) {
		if (millis() - button.lastPress > longPressInterval && !longPressStillDownTrigered)	longPressStillDown();
		else if (millis() - button.lastPress > veryLongPressInterval && !veryLongPressStillDownTrigered) veryLongPressStillDown();
	}

	// General Stuff
	if (mode == MODE_FLASH){
		if (SerialUSB.available()) Serial1.write(SerialUSB.read());
		if (Serial1.available()) SerialUSB.write(Serial1.read());
	} else {
		inputUpdate();

		//----------------------------------------
		// 	MODE_FIRST_BOOT
		//----------------------------------------
		if (mode == MODE_FIRST_BOOT) {

			if (!tokenSynced) {
				tokenSynced = true;
				ESPsyncToken();
			}

			if (!onTime && onWifi) {
				ESPsendCommand(F("sck.getTime()"));
			} else if (!hostNameSet && onWifi) {
				ESPsendCommand(F("sck.updateHostName()"));
			} else if (!helloPublished && onWifi) {
				ESPsendCommand(F("sck.hello()"));
			} else if (helloPublished && hostNameSet && onWifi) {
				changeMode(MODE_NET);
				ESPpublish();
			} else if (!onWifi && millis() - netStatusTimer > netStatusPeriod) {
				if (!wifiSynced) ESPsyncWifi();
				netStatusTimer = millis();
				ESPsendCommand(F("sck.netStatus()"));
			}


		//----------------------------------------
		// 	MODE_AP
		//----------------------------------------
		} else if (mode == MODE_AP) {
			if (!onWifi) {
				if(!lightResults.ok  && readLightEnabled) lightResults = readLight.read();
				else if(!lightResults.commited) {

					changeMode(MODE_FIRST_BOOT);
					if (lightResults.lines[0].endsWith(F("auth"))) {
						ESPsetWifi(lightResults.lines[1], lightResults.lines[2]);
						ESPsetToken(lightResults.lines[3]);
					} else if (lightResults.lines[0].endsWith(F("wifi"))) {
						ESPsetWifi(lightResults.lines[1], lightResults.lines[2]);
					}
				 	lightResults.commited = true;
				}
			}

		// //----------------------------------------
		// // 	MODE_NET
		// //----------------------------------------
		} else if (mode == MODE_NET) {
			if (millis() - lastPublishTime > postInterval * 1000 * 5) softReset();	// Something went wrong!! RESET
			if (!onWifi) {
				
			}
		}
	}
}

void SckBase::changeMode(SCKmodes newMode) {

	if (newMode != mode) {

		if (prevMode == MODE_BRIDGE) outputLevel = prevOutputLevel;

		if (newMode == MODE_AP) {
			lightResults.ok = false;
			lightResults.commited = false;
			if (!ESPon) ESPcontrol(ESP_ON);
		} else if (newMode == MODE_BRIDGE) {
			prevOutputLevel = outputLevel;
			outputLevel = OUT_SILENT;
		} else if (newMode == MODE_OFF) {
			goToSleep();
		} else if (newMode == MODE_NET) {
			intervalTimer = 0;	// start instantly
		} else if (newMode == MODE_SD) {
			if (ESPon) ESPcontrol(ESP_OFF);
		}

		sckOut(String F("Changing mode to ") + modeTitles[newMode] + F(" from ") + modeTitles[mode]);
		prevMode = mode;
		mode = newMode;
		led.update(newMode);
		eppromMode.write(mode);
	}	
}

void SckBase::inputUpdate() {

	if (SerialUSB.available()) {
		char buff = SerialUSB.read();
		serialBuff += buff;
		if (mode == MODE_BRIDGE) Serial1.write(buff); 			// Send buffer to ESP
		else sckOut((String)buff, PRIO_MED, false);				// Shell echo
		if (buff == 13 || buff == 10) { 						// New line detected
			sckOut("");
			sckIn(serialBuff);									// Process input
			serialBuff = "";
			prompt();
		}
	}

	while (Serial1.available()) {
		char buff = Serial1.read();
		espBuff += buff;
		if (mode == MODE_BRIDGE) SerialUSB.write(buff);
		else if (buff == 13 || buff == 10) {
			espMessage(espBuff);
			espBuff = "";
		}
	}
}

void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF:		// DEJA de tener output por el serial si espbridging true y ejecutas espoff
			onWifi = false;
			if (ESPon && mode != MODE_BRIDGE) {
				sckOut(F("Turning off ESP..."));
				// delay(100);
				ESPon = false;
				digitalWrite(CH_PD, LOW);
				digitalWrite(POWER_WIFI, HIGH);		// Turn off ESP
				digitalWrite(GPIO0, LOW);
				espTotalOnTime += millis() - espLastOn;
				sckOut(String F("ESP was on for milliseconds: ") + String(millis() - espLastOn));
				// delay(100);
			} else {
				sckOut(F("ESP already off!"));
			}
			break;
		case ESP_FLASH:			// The only way out off flash mode is phisically reset SCK (buscar alguna otra manera: podria ser con el boton, si espflash true then reset)
			changeMode(MODE_FLASH);
			led.bridge();
			disableTimer5();
			sckOut(F("Putting ESP in flash mode...\r\nRemember to reboot ESP after flashing (esp reboot)!"));
			if (ESPon) ESPcontrol(ESP_OFF);
			SerialUSB.begin(230400);
			Serial1.begin(230400);
			delay(500);
			digitalWrite(CH_PD, HIGH);
			digitalWrite(GPIO0, LOW);			// LOW for flash mode
			digitalWrite(POWER_WIFI, LOW); 		// Turn on ESP
			break;
		case ESP_BRIDGE_ON:
			if (mode != MODE_BRIDGE) {
				led.bridge();
				disableTimer5();
				changeMode(MODE_BRIDGE);
				if (!ESPon) ESPcontrol(ESP_ON);
				sckOut(F("Starting ESP bridge mode..."));
			} else {
				sckOut(F("ESP bridge already on!"));
			}
			break;
		case ESP_BRIDGE_OFF:
			if (mode == MODE_BRIDGE) {
				configureTimer5(led.refreshPeriod);
				changeMode(prevMode);
				sckOut("");
				sckOut(F("Turning off ESP bridge..."));
			} else {
				sckOut(F("ESP bridge already off!"));
			}
			break;
		case ESP_ON:
			if (!ESPon) {
				sckOut(F("Turning on ESP..."));
				// delay(100);
				digitalWrite(CH_PD, HIGH);
				digitalWrite(GPIO0, HIGH);		// HIGH for normal mode
				// delay(50);
				digitalWrite(POWER_WIFI, LOW); 		// Turn on ESP
				espLastOn = millis();
				// delay(500);					// Give time to ESP to boot and stabilize (later add a sanity check or run a reboot on ESP)
											// Also verify if this operation is ressetting SAM with reset cause and logit
											// Also turn it off after some sleep time...
				ESPon = true;
			} else {
				sckOut(F("ESP already on!"));
			}
			break;
		case ESP_REBOOT:
			ESPcontrol(ESP_OFF);
			delay(5);
			ESPcontrol(ESP_ON);
			break;
	}
}

/*
 * @params: timeout waiting for response. 0 = no response needed
 */
String SckBase::ESPsendCommand(String command, float timeout, bool external) {

	if(!ESPon) ESPcontrol(ESP_ON);
	if (!ESPworking) {
		ESPworking = true;
		sckOut(String F("Sending command : ") + command, PRIO_LOW);
		Serial1.write('\n');
		delay(2);
		while (Serial1.available()) Serial1.read(); 	// Cleans all chars in the buffer
		float sentCommand = millis();
		Serial1.print(command);
		Serial1.write('\n');
		Serial1.flush();								// Waits for transmission to end
		char buff;
		espBuff = "";
		while (millis() - sentCommand < timeout) {
			if (Serial1.available()) {
				buff = Serial1.read();
				espBuff += (char)buff;
				if ((espBuff.endsWith("\n") || espBuff.endsWith("\r")) && espBuff.length() > 1) {
					espBuff.replace("\n", "");
					espBuff.replace("\r", "");
					if (command.startsWith(espBuff)) {
						espBuff = "";					// Clears echo line
					} else {
						prioLevels custom_PRIO = PRIO_LOW;
						if (external) custom_PRIO = PRIO_HIGH;
						sckOut(String F("Response: ") + espBuff, custom_PRIO);
						espBuff.replace("\n", "");
						espBuff.replace("\r", "");
						espBuff.trim();
						ESPworking = false;
						return espBuff;
					}
				}
			}
		}
	} else {
		return F("ESP Working!!");
	}
	ESPworking = false;
	return F("timedout");
}

bool SckBase::ESPsetToken(String token, int retrys) {

	if (retrys > 5) {
		sckOut(F("Failed setting ESP token..."));
		return false;
	}

	sckOut(String F("Setting token: ") + token);

	if (retrys == 0) {
		Token tokenToSave;
		tokenToSave.valid = true;
		token.toCharArray(tokenToSave.token, 64);
		eppromToken.write(tokenToSave);
	}

	const String comToSend PROGMEM = "config.token=\"" + token + "\"";
	String answer = ESPsendCommand(comToSend);
	ESPsyncToken(retrys);
	answer = ESPsendCommand(F("sck.saveConf()"));
	return true;
}

bool SckBase::ESPgetToken(){

	String answer = ESPsendCommand(F("=config.token"));

	if (!answer.equals(F("timedout"))) {

		answer.replace(">", "");
		answer.trim();

		ESPtoken = answer;

		sckOut(String F("ESPtoken: ") + ESPtoken);
		return true;
	}
	return false;
}

bool SckBase::ESPsyncToken(int retrys){

	Token readedToken;
	readedToken = eppromToken.read();

	if (readedToken.valid) {

		String SAMtoken = readedToken.token;

		sckOut(String F("SAMtoken: ") + SAMtoken);

		retrys = retrys + 1;

		if (ESPgetToken()) {
			if (SAMtoken.equals(ESPtoken)) {
				sckOut(F("Esp token OK"));
				return true;
			} else {
				sckOut(F("Syncing ESP token..."));
				ESPsetToken(SAMtoken, retrys);
			}
		}
	}
	sckOut(F("No valid token saved on EPPROM"));
	return false;
}

bool SckBase::ESPsetWifi(String ssid, String pass, int retrys) {

	if (retrys > 5) {
		sckOut(F("Failed setting ESP credentials..."));
		return false;
	}

	sckOut(String F("Setting ssid: ") + ssid + F(" pass: ") + pass);

	if (retrys == 0) {
		Credentials credentials;
		credentials.valid = true;
		ssid.toCharArray(credentials.ssid, 64);
		pass.toCharArray(credentials.pass, 64);
		eppromCred.write(credentials);
	}

	const String comToSend PROGMEM = "wifi.sta.config(\"" + ssid + "\", \"" + pass + "\")";
	String answer = ESPsendCommand(comToSend);
	ESPsyncWifi(retrys);
	answer = ESPsendCommand(F("sck.saveConf()"));
	return true;
}

bool SckBase::ESPgetWifi(){

	String answer = ESPsendCommand(F("=wifi.sta.getconfig()"));

	if (!answer.equals(F("timedout"))) {

		answer.replace(">", "");
		answer.trim();

		uint8_t first = answer.indexOf('\t');
		uint8_t second = answer.indexOf('\t', first + 1);

		ESPssid = answer.substring(0, first);
		ESPpass = answer.substring(first + 1, second);

		sckOut(String F("ESPssid: ") + ESPssid + F(" ESPpass: ") + ESPpass);
		return true;
	}
	return false;
}

bool SckBase::ESPsyncWifi(int retrys){

	Credentials credentials;
	credentials = eppromCred.read();

	if (credentials.valid) {

		String SAMssid = credentials.ssid;
		String SAMpass = credentials.pass;

		// if (retrys > 1) ESPcontrol(ESP_OFF);
		sckOut(String F("SAMssid: ") + SAMssid + F(" SAMpass: ") + SAMpass);

		retrys = retrys + 1;

		if (ESPgetWifi()) {
			if (SAMssid.equals(ESPssid) && SAMpass.equals(ESPpass)) {
				wifiSynced = true;
				sckOut(F("Esp credentials OK"));
				return true;
			} else {
				sckOut(F("Syncing ESP credentials..."));
				ESPsetWifi(SAMssid, SAMpass, retrys);
			}
		}
	}
	sckOut(F("No valid credentials saved on EPPROM"));
	return false;
}

void SckBase::ESPpublish() {
	// hay que buscar una libreria json encode
	const String comToSend PROGMEM = "sck.publish(\"{\\\"time\\\":\\\"" + payloadData.time + "\\\",\\\"noise\\\":\\\"" + String(payloadData.noise, 2) + "\\\",\\\"temperature\\\":\\\"" + String(payloadData.temperature, 2) + "\\\",\\\"humidity\\\":\\\"" + String(payloadData.humidity, 2) + "\\\",\\\"battery\\\":\\\"" + String(payloadData.battery) + "\\\"}\")";
	lastPublishTime = millis();
	String answer = ESPsendCommand(comToSend);
}

void SckBase::espMessage(String message) {

	// sckOut("received: " + message);


	message.replace(">", "");
	message.trim();

	int intMessage = 0;
	String parameters = "";

	if (message.indexOf("\t") > -1) {
		intMessage = message.substring(0, message.indexOf("\t")).toInt();
		parameters = message.substring(message.indexOf("\t"));	
	} else {
		intMessage = message.toInt();
	}
	
	// int intMessage = message.toInt();

	switch (intMessage) {
		case ESP_WIFI_CONNECTED:
			onWifi = true;
			sckOut(F("Connected to Wifi!!"));
			if (mode == MODE_FIRST_BOOT || mode == MODE_AP) {
				if (!helloPublished || !hostNameSet) changeMode(MODE_FIRST_BOOT);
				else changeMode(MODE_NET);
			}
			prompt();
			break;
		case ESP_WIFI_ERROR:
			onWifi = false;
			sckOut(F("Wifi conection failed: Unknown cause"));
			if (mode == MODE_NET) changeMode(MODE_AP);
			prompt();
			break;
		case ESP_WIFI_ERROR_PASS:
			onWifi = false;
			sckOut(String F("Wifi conection failed: wrong password"));
			if (mode == MODE_NET) changeMode(MODE_AP);
			prompt();
			break;
		case ESP_WIFI_ERROR_AP:
			onWifi = false;
			sckOut(String F("Wifi conection failed: AP not found"));
			if (mode == MODE_NET) changeMode(MODE_AP);
			prompt();
			break;
		case ESP_TIME_FAIL:
			sckOut(F("Time sync failed!!!"));
			if (onWifi) ESPsendCommand(F("sck.getTime()"));
			prompt();
			break;
		case ESP_TIME_NEW:
			parameters.replace("\n", "");
			parameters.replace("\r", "");
			parameters.trim();
			if (parameters.equals("0")) {
				sckOut(F("Wrong time received from ESP!!"));
			} else {
				setTime(parameters);
			}
			prompt();
			break;
		case ESP_MODE_AP:
			sckOut(F("ESP entered AP mode"));
			prompt();
			break;
		case ESP_MODE_STA:
			sckOut(F("ESP entered station mode"));
			prompt();
			break;
		case ESP_WEB_STARTED:
			sckOut(F("ESP started Web Server"));
			prompt();
			break;
		case ESP_MQTT_HELLO_OK:
			helloPublished = true;
			sckOut(F("MQTT hellow OK"));
			prompt();
			break;
		case ESP_MQTT_PUBLISH_OK:
			sckOut(F("MQTT publish OK"));
			if (mode == MODE_NET) {
				ESPcontrol(ESP_OFF);
				// goToSleep(); // todavia falta implementar el wakeup correctamente...
			}
			prompt();
			break;
		case ESP_MQTT_ERROR:
			sckOut(F("MQTT connection error"));
			if (publishRetryCounter < maxPublishRetry) {
				publishRetryCounter = publishRetryCounter + 1;
				sckOut(String F("MQTT publish retry number ") + publishRetryCounter);
				ESPcontrol(ESP_REBOOT);
				ESPpublish();
			} else {
				sckOut(F("MQTT persistent error, giving up!!"));
				if (mode == MODE_NET) ESPcontrol(ESP_OFF);
			}
			prompt();
			break;
		case ESP_HOSTNAME_UPDATED:
			hostNameSet = true;
			sckOut(F("HostName updated!!"));
			prompt();
			break;
	}
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

	//PENSAR LA MEJOR MANERA DE PONER LAS EXCEPCIONES PARA EL ESP BRIDGE Y FLASH
	if (strIn.equals(comTitles[0])){

		// esp bridge on
		ESPcontrol(ESP_BRIDGE_ON);
	
	} else if (strIn.equals(comTitles[1])) {
		
		// esp bridge off
		ESPcontrol(ESP_BRIDGE_OFF);

	} else if (strIn.equals(comTitles[2])) {

		// esp flash
		ESPcontrol(ESP_FLASH);

	} else if (strIn.equals(comTitles[3])) {

		// esp reboot
		ESPcontrol(ESP_REBOOT); 

	} else if (strIn.equals(comTitles[4])) {

		// esp off
		ESPcontrol(ESP_OFF);

	} else if (strIn.equals(comTitles[5])) {

		// esp on
		ESPcontrol(ESP_ON);

	} else if (strIn.equals(comTitles[6])) {

		// esp start
		if (!ESPon) ESPcontrol(ESP_ON);
		ESPsendCommand(F("dofile(\"preinit.lua\")"));

	} else if (strIn.equals(comTitles[7])) {

		// esp listap
		ESPsendCommand(F("sck.getAPlist(function(t) print(t) end)"), 5000);
		
	} else if (strIn.startsWith(comTitles[8])) {

		// setwifi SSID PASS
		String separator;
		if (strIn.indexOf('"') >= 0) separator = '"';
		else if (strIn.indexOf("'") >= 0) separator = "'";
		uint8_t first = strIn.indexOf(separator);
		uint8_t second = strIn.indexOf(separator, first + 1);
		uint8_t third = strIn.indexOf(separator, second + 1);
		uint8_t fourth = strIn.indexOf(separator, third + 1);

		String ssid = strIn.substring(first + 1, second);
		String pass = strIn.substring(third + 1, fourth);

		if (pass.length() < 8) {
			sckOut(F("Wifi password should have at least 8 characters!!!"));
		} else {
			ESPsetWifi(ssid, pass);
		}
		

	} else if (strIn.startsWith(comTitles[9])) {

		// settoken TOKEN
		strIn.replace(comTitles[9], "");
		strIn.trim();
		ESPsetToken(strIn);

	} else if (strIn.startsWith(comTitles[10])) {

		// esp command
		if (!ESPon) ESPcontrol(ESP_ON);
		strIn.replace(comTitles[10], "");
		strIn.trim();
		ESPsendCommand(strIn, 2000, true);

	} else if (strIn.startsWith(comTitles[11])) {

		// debug level
		strIn.replace(comTitles[11], "");
		strIn.trim();
		sckOut(F("Current output level is "), PRIO_HIGH, false);
		if (strIn.length() > 0) { 
			uint8_t newLevel = (uint8_t)strIn.toInt();
			if (newLevel == 0) outputLevel = OUT_SILENT;
			else if (newLevel == 1) outputLevel = OUT_NORMAL;
			else if (newLevel == 2) outputLevel = OUT_VERBOSE;
		}
		if (outputLevel == 0) sckOut(F("Silent"), PRIO_HIGH);
		else if (outputLevel == 1) sckOut(F("Normal"), PRIO_HIGH);
		else if (outputLevel == 2) sckOut(F("Verbose"), PRIO_HIGH);


	} else if (strIn.startsWith(comTitles[12])) {

		// urban present
		if (urbanBoardDetected()) {
			sckOut(F("Urban board detected!!"));
		} else {
			sckOut(F("Urban board not found!!"));
		}

	} else if (strIn.equals(comTitles[13])) {

		// reset cause
		int resetCause = PM->RCAUSE.reg;
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
		
	} else if (strIn.equals(comTitles[14])) {

		// reset
		sckOut(F("Bye!"), PRIO_HIGH);
		softReset();

	}  else if (strIn.equals(comTitles[15])) {

		// card mode
		sckOut(F("Entering SD card mode..."));
		changeMode(MODE_SD);

	} else if (strIn.equals(comTitles[16])) {

		// net mode
		sckOut(F("Entering Net mode..."));
		changeMode(MODE_NET);

	} else if (strIn.equals(comTitles[17])) {

		// time
		sckOut(ISOtime()); 

	} else if (strIn.equals(comTitles[18])) {

		// sync time
		if (!ESPon) {
			sckOut(F("Turning on ESP... Try sync time in a moment"));
			ESPcontrol(ESP_ON);
		} else if (!onWifi) {
			sckOut(F("Wifi not connected... can't sync time"));	
		} else {
			ESPsendCommand(F("sck.getTime()"));
		}

	} else if (strIn.equals(comTitles[19])) {

		// last sync
		if (lastTimeSync > 0){
			uint32_t seconds = rtc.getEpoch() - lastTimeSync;
			if (seconds > 86400) {
				sckOut(String(seconds / 86400) + F(" days "), PRIO_MED, false);
				seconds = seconds % 86400;
			}
			if (seconds > 3600) {
				sckOut(String(seconds / 3600) + F(" hours "), PRIO_MED, false);
				seconds = seconds % 3600;	
			}
			if (seconds > 60) {
				sckOut(String(seconds / 60) + F(" minutes "), PRIO_MED, false);
				seconds = seconds % 60;
			}
			sckOut(String(seconds) + F(" seconds"));
		} else {
			sckOut(F("Time not synced since last reset!!!"));
		}
		

	} else if (strIn.equals(comTitles[20])) {

		// checkconfig
		ESPgetWifi();
		ESPgetToken();

	} else if (strIn.equals(comTitles[21])) {

		// publish
		ESPpublish();

	} else if (strIn.equals(comTitles[22])) {

		// gettoken
		ESPsendCommand(F("=config.token"), 1000, true);

	} else if (strIn.equals(comTitles[23])) {

		// shell mode
		if (mode != MODE_SHELL) {
			changeMode(MODE_SHELL);
		} else changeMode(prevMode);

	} else if (strIn.equals(comTitles[24])) {

		// getmode
		sckOut(modeTitles[mode]);

	} else if (strIn.equals(comTitles[25])) {

		// getversion
		sckOut(version);

	} else if (strIn.equals(comTitles[26])) {

		// sleep
		goToSleep();

	} else if (strIn.equals(comTitles[27])) {

		// led off
		led.off();

	} else if (strIn.equals(comTitles[28])) {

		// get esp time
		sckOut(String(espTotalOnTime));

	} else if (strIn.startsWith(comTitles[29])) {

		//set time
		strIn.replace(comTitles[29], "");
		strIn.trim();
		setTime(strIn);

	} else if (strIn.startsWith(comTitles[30])) {

		// help
		sckOut(F("Available commands:"));
		sckOut("");
		for (uint i = 0; i < (sizeof(comTitles)/sizeof(*comTitles)) - 1;) {
			for (int ii = 0; ii < 3; ++ii) {
				sckOut(comTitles[i] + "\t\t", PRIO_MED, false);
				if (comTitles[i].length() < 8) sckOut(String('\t'), PRIO_MED, false);
				i++;
			}
			sckOut("");
		}
		sckOut("");

	} else {
		if (strIn.length() > 0) {
			sckOut(F("Unrecognized command"));
			sckOut(F("Try 'help' for a list of commands."));
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
void SckBase::sckOut(String strOut, prioLevels priority, bool newLine) {

	if (outputLevel + priority > 1) {
		SerialUSB.print(strOut);
		if (newLine) SerialUSB.println();
	}
}

void SckBase::prompt() {

	sckOut("SCK > ", PRIO_MED, false);
}

bool SckBase::setTime(String epoch) {
	// validate time here!!!
	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		sckOut(F("RTC updated!!!"));
		onTime = true;
		lastTimeSync = rtc.getEpoch();
		return true;
	}
	else sckOut(F("RTC update failed!!"));
	return false;
}

String SckBase::ISOtime() {
	// leading zeros
	//return string.format("%04d-%02d-%02dT%02d:%02d:%02dZ", tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
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



/* 	--------------
 	|	Button   |
 	--------------
*/
void SckBase::buttonEvent() {
	if (!digitalRead(PIN_BUTTON)) {
		button.isDown = true;
		button.lastPress = millis();
		buttonDown();
	} else {
		button.isDown = false;
		button.lastRelease = millis();
		buttonUp();
	} 
}

void SckBase::buttonDown() {
	sckOut(F("buttonDown"), PRIO_MED);
	if (mode == MODE_OFF) {
		changeMode(prevMode);
	} else if (mode == MODE_SD) {
		changeMode(MODE_AP);
	} else if (mode == MODE_FIRST_BOOT || mode == MODE_NET) {
		changeMode(MODE_SD);
	} else if (mode == MODE_AP) {
		changeMode(MODE_NET);
	}
}

void SckBase::buttonUp() {
	sckOut(F("buttonUp"), PRIO_MED);

	longPressStillDownTrigered = false;
	veryLongPressStillDownTrigered = false;

	float pressedTime = button.lastRelease - button.lastPress;
	if (pressedTime < longPressInterval) {
		shortPress();	
	} else if (pressedTime < veryLongPressInterval) {
		sckOut(String F("Pressed time: ") + String(pressedTime));
		longPress();
	} else {
		sckOut(String F("Pressed time: ") + String(pressedTime));
		veryLongPress();
	}
	
}

void SckBase::shortPress() {

	// Atach to an interrupt to wave up

	sckOut(F("shortPress"), PRIO_MED);
}

void SckBase::longPress() {
	sckOut(F("longPress"), PRIO_MED);
}

void SckBase::veryLongPress() {
	sckOut(F("veryLongPress"), PRIO_MED);
}

void SckBase::longPressStillDown() {
	longPressStillDownTrigered = true;
	sckOut(F("longPressStillDown"), PRIO_MED);
	changeMode(MODE_OFF);
	goToSleep();
}

void SckBase::veryLongPressStillDown() {
	veryLongPressStillDownTrigered = true;

	// Factory reset
	factoryReset();


	sckOut(F("veryLongPressStillDown"), PRIO_MED);
}



void SckBase::softReset() {
	WatchdogSAMD wdt;
 	wdt.enable(10);
}

bool SckBase::openPublishFile() {

	char charFileName[publishFileName.length()];

	bool writeHeader = false;
	String header = "Time,Noise,Humidity,Temperature,Battery\n";		//TEMP

	if (sdPresent()) {
		int i = 1;
		while (i < 512) {
			publishFileName.toCharArray(charFileName, publishFileName.length());
			if (!SD.exists(charFileName)) writeHeader = true;
			publishFile = SD.open(charFileName , FILE_WRITE);
			if (publishFile) {
				if (writeHeader) publishFile.print(header);
				if (publishFile.size() < FileSizeLimit) return true;
				else {
					publishFileName = String F("POST") + leadingZeros(String(i), 3) + F(".CSV");
					publishFile.close();
				}
			}
		}
	}
	return false;
}

bool SckBase::sdPresent() {
	if (SD.begin(CS_SDCARD)) {
		sckOut(F("Sdcard ready!!"));
		return true;
	} else {
		sckOut(F("Sdcard not found!!"));
		return false;
	}
}

void SckBase::goToSleep() {

	rtc.setAlarmSeconds(postInterval);
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
  	// Button interrupt for wake up
  	// MISSING HERE BUTTON INTERRUPT

	// rtc.standbyMode();	//luego ponerlo de modo que espere a que el led llegue a zero
	ESPcontrol(ESP_OFF);
	led.off();
	disableTimer5();
}

void SckBase::wakeUp() {
	configureTimer5(led.refreshPeriod);
}

void SckBase::factoryReset() {
	ESPcontrol(ESP_REBOOT);
	delay(1000);
	ESPsetWifi("null", "password");
	ESPsetToken("null");
	softReset();
}

/* 	-------------
 	|	 POT control (por ahora es cun copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
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



/* 	-------------
 	|	 Power management (por ahora es cun copy paste del codigo de miguel, hay que revisarlo y adaptarlo)	|
 	-------------
*/
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
	
	float originalValue = readResistor(6);
	writeResistor(6, 1200.0);
	float compareValue = readResistor(6);
	writeResistor(6, originalValue);
	if (abs(compareValue - 1176.47) < 5) {
		return true;
	}
	return false;
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


/* 	-------------
 	|	 Led	|
 	-------------
*/
void Led::setup() {
	pinMode(PIN_LED_RED, OUTPUT);
	pinMode(PIN_LED_GREEN, OUTPUT);
	pinMode(PIN_LED_BLUE, OUTPUT);
	off();
	dir = true;
	colorIndex = 0;
	ledRGBcolor = yellowRGB;
	pulseMode = PULSE_STATIC;
}

/* Call this every time there is an event that changes SCK mode
 *
 */ 
void Led::update(SCKmodes newMode) {

	configureTimer5(refreshPeriod);

	switch (newMode) {
		case MODE_AP:
			currentPulse = pulseRed;
			pulseMode = PULSE_SOFT;
			break;
		case MODE_NET:
			currentPulse = pulseBlue;
			pulseMode = PULSE_SOFT;
			break;
		case MODE_SD:
			currentPulse = pulsePink;
			pulseMode = PULSE_SOFT;
			break;
		case MODE_SHELL:
			ledRGBcolor = yellowRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_ERROR:
			ledRGBcolor = orangeRGB;
			pulseMode = PULSE_HARD;
			break;
		case MODE_FLASH:
			ledRGBcolor = lightBLueRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_BRIDGE:
			ledRGBcolor = lightBLueRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_FIRST_BOOT:
			ledRGBcolor = yellowRGB;
			pulseMode = PULSE_STATIC;
			break;
		case MODE_OFF:
			off();
			break;
	}
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
	
	if(pulseMode != PULSE_STATIC) {
		
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
	}

	setRGBColor(ledRGBcolor);
}

/* Change Led color based on RGB values
 *
 */
void Led::setRGBColor(RGBcolor myColor) {
	analogWrite(PIN_LED_RED, 255 - constrain(myColor.r, 0, 255));
	analogWrite(PIN_LED_GREEN, 255 - constrain(myColor.g, 0, 255));
	analogWrite(PIN_LED_BLUE, 255 - constrain(myColor.b, 0, 255));
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
	digitalWrite(PIN_LED_RED, HIGH);
	digitalWrite(PIN_LED_GREEN, HIGH);
	digitalWrite(PIN_LED_BLUE, HIGH);
}

/* 	-----------------
 	|	 Button 	|
 	-----------------
*/
void Button::setup() {
	pinMode(PIN_BUTTON, INPUT);
	attachInterrupt(PIN_BUTTON, ISR_button, CHANGE);
	longPressDuration = LONG_PRESS_DURATION;
	veryLongPressDuration = VERY_LONG_PRESS_DURATION;
};


/*

 00000000    0000     00000       0000
    00     00    00   00  00    00    00
    00    00      00  00   00  00      00
    00    00      00  00   00  00      00
    00     00    00   00  00    00    00
    00       0000     00000       0000

-- BUGS

NOTAS
-----BASE BOARD-------

** STATE MACHINE DESIGN (MODES)
  --it's important to get a very simple design
  --Make a list of triggers that changes the mode

** INPUT-OUTPUT
  [ ] -- Diseñar una clase IOobject para crear instancias con USB, ESP, SDCARD, donde se definan sus caracteristicas (es in y/o out, nivel de out, es interactivo, esta activo.)
  [x] -- sckIn receives a struct with a string and the origin (USB, ESP, LIGHT, etc), process the command, and outputs via sckOut
  [x] -- sckOut recibe un string con el String y lo redirecciona a las outputs disponibles (USB,ESP, SDCARD), dependiendo del nivel de output y de la prioridad del mensaje
  [x] -- Create an array to store command titles saving all the strings in flash
  [x] -- FANCY in the case of interactive inputs, get a prompt!! ej. >, echo of what youre typing and a welcome message with some info.
  [x] -- Revisar la libreria de cmdMessenger https://github.com/thijse/Arduino-CmdMessenger para ver si vale la pena hacer el cambio.
  [ ] -- Migrar la solucion actual a cmdMessenger, el ejemplo ConsoleShell.ino tiene todo lo necesario...

** ESP COMMUNICATION
  [x] -- Hacer que los commandos SAM <> ESP sean numericos
  [ ] -- Migrar todo el codigo a arduino
  [x] -- Esp control  (ESP_OFF, ESP_ON, ESP_REBOOT, etc)
  [x] -- Implementar una manera de salir de ESPbridging mode
  [x] -- Send Serial commands
  [ ] -- Get net status
  [ ] -- Reimplementar el esp publish

** LED
  [x] -- Automatic change depending on mode
  [ ] -- Mejorar el Fade ROSA
  [x] -- Integrate HSI and RGB colors in a tansparent way
  [x] -- Smooth soft pulse with interrupt timers
  [ ] -- Funtions for exceptions (reading, config, etc)

** RTC - TIME
  [ ] -- Resolver la integración de las alarmas de la libreria rtc
  [ ] -- Implementar otras opciones de recibir el time ademas del ESP, (light, sound, USB)

** BUTTON
  [x] -- Interrupt
  [x] -- Detect Down, Up, and shortPress
  [ ] -- Detect long and verylong (faltan resolver los hanhgs de los timers del RTC para que esto funcione)

** READING-LIGHT-SOUND
  [ ] -- Debug the new nonbloking version
  [ ] -- Probar en todas las pantallas posibles y en moviles
  [ ] -- Hacer commit a github de la non blocking
  [x] -- Integrate library
  [x] -- Send esp command on succsesfull received credentials
  [x] -- Change led with checksum OK.
  [ ] -- Only use it if Urban board is present, if not, go to fallback
  [ ] -- Usar la implementación de lightRead para poner tambien el sonido

** CONFIGURATION
  [x] -- Make hardcoded Default configuration
  [x] -- Save configuration on eeprom
  [ ] -- get configuration from ESP
  [ ] -- Hacer configurable el periodo de readings

** POWER MANAGMENT
  [ ] -- Manejar los chargemodes
  [ ] -- Poner feedback en el led del nivel de bateria y carga
  [ ] -- Terminar el sleep mode y hacer pruebas de consumo
  [ ] -- Emergency mode with interrupt to wake up
  [ ] -- How on battery influences modes
  [ ] -- Diseñar bien la estrategia acerca del ESP y MICS

** SDCARD
  [ ] -- Encontrar la combinación entre ESP y SD para que el SPi no se cuelgue
  [ ] -- Integrate SD library
  [ ] -- Detect if sdcard is present

** ACCELEROMETER
  [ ] -- Crear una funcion get Acc


-----URBAN BOARD------

** DETECTION AND DIAGNOSIS

  [x] -- Detect if urban present
  [ ] -- Hacer que el funcionamiento con/sin urban board sea mas robusto

** SENSOR TEMPLATE

** SOUND
  [x] -- Basic average

** TEMP AND HUMIDITY
  [ ] -- Buscar una libreria para el SHT21

** LIGHT
  [ ] -- Implementar el get light
  [ ] -- Implementar el get UV

** GASES
  [ ] -- Implementar el get CO
  [ ] -- Implementar el get No2
  [ ] -- Hacer pruebas de consumo y plantear modos de uso

--------ESP8266-------
***DECIDIR SI MIGRAR O NO A C++
** APMODE
  [ ] -- Http GET ssid & pass
  [ ] -- HttpServer

** POSTS
  [ ] -- Http
  [x] -- MQTT
  [ ] -- SSL

** NET DEBUG OUTPUT
  [ ] -- Net console (netcat)
  [ ] -- Errors via mqtt

*/
