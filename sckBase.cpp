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

/* 	----------------------------------
 	|	SmartCitizen Kit Baseboard   |
 	----------------------------------
*/
void SckBase::setup() {

	String buildDate = __DATE__;
	buildDate.replace(' ', '-');
	version += buildDate + '-' + String(__TIME__);

	modeTitles[MODE_AP]			= 	"Ap mode";
	modeTitles[MODE_NET] 		=	"Network mode";
	modeTitles[MODE_SD] 		= 	"SD card mode";
	modeTitles[MODE_SHELL] 		= 	"Shell mode";
	modeTitles[MODE_FLASH] 		= 	"ESP flashing mode";
	modeTitles[MODE_BRIDGE] 	= 	"ESP bridging mode";
	modeTitles[MODE_ERROR] 		= 	"Error mode";
	modeTitles[MODE_FIRST_BOOT] = 	"First boot mode";

	// I2C Configuration
	Wire.begin();				// Init wire library

	// Serial Ports Configuration
	Serial1.begin(baudrate);
	SerialUSB.begin(baudrate);


	// ESP Configuration
	pinMode(POWER_WIFI, OUTPUT);
	pinMode(CH_PD, OUTPUT);
	pinMode(GPIO0, OUTPUT);
	digitalWrite(POWER_WIFI, HIGH);
	ESPcontrol(ESP_OFF);

	// Sensor Board Conector
	pinMode(IO0, OUTPUT);	// PA7 -- CO Sensor Heather
	pinMode(IO1, OUTPUT);	// PA6 -- NO2 Sensor Heater
	pinMode(S0, INPUT);		// PA4 -- CO Sensor
	pinMode(S1, INPUT);		// PA5 -- NO2 Sensor
	pinMode(S2, INPUT);		// PB8 -- CO Current Sensor
	pinMode(S3, INPUT);		// PB9 -- NO2 Current Sensor
	pinMode(S4, INPUT);		// PB2 -- Sound Sensor

	// SPI Configuration
	pinMode(MOSI, OUTPUT);
  	pinMode(SCK, OUTPUT);
  	// pinMode(MISO, INPUT);
	pinMode(CS_SDCARD, OUTPUT);
	digitalWrite(CS_SDCARD, HIGH);
	pinMode(CS_ESP, OUTPUT);		// SPI Select ESP
	digitalWrite(CS_ESP, HIGH);		// Disable ESP SPI

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

	configureTimer5(refreshPeriod);		// Hardware timer led refresh period

	ESPcontrol(ESP_ON);

	changeMode(MODE_FIRST_BOOT);	// Start in first boot mode until we are connected, or wifi fail

	// if (SD.begin(CS_SDCARD)) {
	// 	sdPresent = true;
	// 	sckOut(F("Sdcard ready!!"));
	// } else {
	// 	sckOut(F("Sdcard not found!!"));
	// }
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
					// led.crcOK();
					if (lightResults.lines[0].endsWith(F("auth"))) {
						ESPsetWifi(lightResults.lines[1], lightResults.lines[2]);
						ESPsetToken(lightResults.lines[3]);
					}
				 	lightResults.commited = true;
				}
			}

		// //----------------------------------------
		// // 	MODE_NET
		// //----------------------------------------
		} else if (mode == MODE_NET) {
			if (!onWifi) {
				
			}
		}
	}
}

void SckBase::changeMode(SCKmodes newMode) {

	if (newMode != mode) {

		if (newMode == MODE_AP && mode != MODE_AP) {
			lightResults.ok = false;
			lightResults.commited = false;
		}else if (newMode == MODE_BRIDGE) {
			prevOutputLevel = outputLevel;
			outputLevel = OUT_SILENT;
		} else {
			outputLevel = prevOutputLevel;
		}

		sckOut(String F("Changing mode to ") + modeTitles[newMode] + F(" from ") + modeTitles[mode]);
		prevMode = mode;
		mode = newMode;
		led.update(newMode);
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
				delay(100);
				ESPon = false;
				digitalWrite(CH_PD, LOW);
				digitalWrite(POWER_WIFI, HIGH);		// Turn off ESP
				digitalWrite(GPIO0, LOW);
				delay(100);
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
				configureTimer5(refreshPeriod);
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
				delay(100);
				digitalWrite(CH_PD, HIGH);
				digitalWrite(GPIO0, HIGH);		// HIGH for normal mode
				delay(50);
				digitalWrite(POWER_WIFI, LOW); 		// Turn on ESP
				delay(500);					// Give time to ESP to boot and stabilize (later add a sanity check or run a reboot on ESP)
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

void SckBase::ESPsetWifi(String ssid, String pass) {

	const String comToSend PROGMEM = "wifi.sta.config(\"" + ssid + "\", \"" + pass + "\")";
	ESPsendCommand(comToSend, 0);
	delay(20);
	String answer = ESPsendCommand(F("=wifi.sta.getconfig()"));

	if (!answer.equals(F("timedout"))) {

		uint8_t first = answer.indexOf('\t');
		uint8_t second = answer.indexOf('\t', first + 1);

		String settedSsid = answer.substring(0, first);
		String settedPass = answer.substring(first + 1, second);

		if (ssid.equals(settedSsid) && pass.equals(settedPass)) {
			sckOut(F("Setting credentials on ESP succeded!!!"));
			ESPsendCommand(F("sck.saveConf()"));
		} else {
			sckOut(F("Setting credentials on ESP failed!!!"));
			sckOut(F("Please try again."));
		}
	} else {
		sckOut(F("No response from ESP"));
	}
}

void SckBase::ESPsetToken(String token) {
	const String comToSend PROGMEM = "config.token=\"" + token + "\"";
	ESPsendCommand(comToSend, 0);
	delay(20);
	String answer = ESPsendCommand(F("print(config.token)"));

	if (!answer.equals(F("timedout"))) {
		if (answer.equals(token)) {
			String tokenSaved = ESPsendCommand(F("=sck.saveConf()"));
			if (tokenSaved.equals(F("true"))) {
				sckOut(F("Setting token on ESP succeded!!!"));
				String saved = ESPsendCommand(F("sck.saveConf()"));
				helloPublished = false;
			}
		} else {
			sckOut(F("Setting token on ESP failed!!!"));
			sckOut(F("Please try again."));
		}
	} else {
		sckOut(F("No response from ESP"));
	}
}

void SckBase::ESPpublish() {
	// hay que buscar una libreria json encode
	const String comToSend PROGMEM = "sck.publish(\"{\\\"time\\\":\\\"" + payloadData.time + "\\\",\\\"noise\\\":\\\"" + String(payloadData.noise, 2) + "\\\",\\\"temperature\\\":\\\"" + String(payloadData.temperature, 2) + "\\\",\\\"humidity\\\":\\\"" + String(payloadData.humidity, 2) + "\\\",\\\"battery\\\":\\\"" + String(payloadData.battery) + "\\\"}\")";
	ESPsendCommand(comToSend);
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
			if (!helloPublished || !hostNameSet) {
				changeMode(MODE_FIRST_BOOT);
			}
			else if (mode != MODE_SHELL && mode != MODE_BRIDGE) changeMode(MODE_NET);
			prompt();
			break;
		case ESP_WIFI_ERROR:
			onWifi = false;
			sckOut(F("Wifi conection failed: Unknown cause"));
			if (mode != MODE_SHELL && mode != MODE_BRIDGE) changeMode(MODE_AP);
			prompt();
			break;
		case ESP_WIFI_ERROR_PASS:
			onWifi = false;
			sckOut(String F("Wifi conection failed: wrong password"));
			if (mode != MODE_SHELL && mode != MODE_BRIDGE) changeMode(MODE_AP);
			prompt();
			break;
		case ESP_WIFI_ERROR_AP:
			onWifi = false;
			sckOut(String F("Wifi conection failed: AP not found"));
			if (mode != MODE_SHELL && mode != MODE_BRIDGE) changeMode(MODE_AP);
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
		if (!ESPon) ESPcontrol(ESP_ON);
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
		if (!ESPon) ESPcontrol(ESP_ON);
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

		// TEMP tengo que rediseñar como será esto
		// aux
		// sckOut(String F("Bus Voltage:   ") + aux1.getBusVoltage_V() + "mV");
		// sckOut(String F("Current:   ") + aux1.getCurrent_mA() + "mV");

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

	} else if (strIn.startsWith(comTitles[26])) {

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

	// Sleep

	sckOut(F("longPressStillDown"), PRIO_MED);
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
	String header = "Time,Noise, Humidity,Temperature\n";		//TEMP

	if (sdPresent) {
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

	led.off();
	disableTimer5();
}

void SckBase::wakeUp() {
	configureTimer5(refreshPeriod);
}

void SckBase::factoryReset() {
	changeMode(MODE_SHELL);
	ESPcontrol(ESP_REBOOT);
	delay(2000);
	ESPsetWifi("null", "password");
	ESPsetToken("null");
	delay(2000);
	// ESPcontrol(ESP_REBOOT);
	// delay(1000);
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
	hue = 40;
	sat = 1.0;
	inten = 0;
	dir = true;
	newHue = hue;
	newSat = sat;
	pulseMode = PULSE_STATIC;
	newPulseMode = PULSE_STATIC;
}

/* Call this every time there is an event that changes SCK mode
 *
 */ 
void Led::update(SCKmodes newMode) {

	switch (newMode) {
		case MODE_AP:
			newHue = 5;
			newSat = 0.92;
			newPulseMode = PULSE_SOFT;
			break;
		case MODE_NET:
			newHue = 233;
			newSat = 1.0;
			newPulseMode = PULSE_SOFT;
			break;
		case MODE_SD:
			newHue = 308;
			newSat = 0.85;
			newPulseMode = PULSE_SOFT;
			break;
		case MODE_SHELL:
			newHue = 40;
			newSat = 1.0;
			newPulseMode = PULSE_STATIC;
			break;
		case MODE_ERROR:
			newHue = 28;
			newPulseMode = PULSE_HARD;
			break;
		case MODE_FLASH:
			newHue = 170;
			newSat = 1;
			newPulseMode = PULSE_STATIC;
			break;
		case MODE_BRIDGE:
			newHue = 170;
			newSat = 1;
			newPulseMode = PULSE_STATIC;
			break;
		case MODE_FIRST_BOOT:
			newHue = 40;
			newSat = 1.0;
			newPulseMode = PULSE_STATIC;
			break;
	}
}

void Led::reading() {
	setRGBColor(white);
	timerReading = millis(); //substituir esto por una libreria de timers
}

void Led::wifiOK() {

	// Green
	newHue = 120;
	newSat = 1.0;
	setHSIColor(120, 1.0, inten);
	newPulseMode = PULSE_STATIC;
}

void Led::crcOK() {

	// Yellow
	newHue = 35;
	newSat = 1.0;
	setHSIColor(26, 0.87, inten);
	newPulseMode = PULSE_STATIC;
}

void Led::bridge() {
	setRGBColor(white);
}

void Led::tick() {
	
	if(pulseMode != PULSE_STATIC) {
		//Esto hay que substituirlo por el modelo COS o por el de la tabla de gamma
		
		pulseMode = newPulseMode;

		if (dir) {
			inten = inten + 0.03;
			// When the led is in the top part of the pulse
			if (inten >= 1) {
				dir = false;
			}
		} else {
			// When the led is in the off part of the pulse
			inten = inten - 0.03;
			if (inten <= 0) {
				dir = true;
				sat = newSat;
				hue = newHue;
			}
		}
	} else {
		dir = false;
		inten = 1;
		sat = newSat;
		hue = newHue;
		pulseMode = newPulseMode;
	}
	setHSIColor(hue, sat, inten);
}

/* Change Led color based on RGB values
 *
 */
void Led::setRGBColor(oneColor myColor) {
	analogWrite(PIN_LED_RED, 255 - constrain(myColor.r, 0, 255));
	analogWrite(PIN_LED_GREEN, 255 - constrain(myColor.g, 0, 255));
	analogWrite(PIN_LED_BLUE, 255 - constrain(myColor.b, 0, 255));
};

/* Change Led color based on HSI values (Hue, Saturation, Intensity)
 *
 */
void Led::setHSIColor(float h, float s, float i) 
{
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
  [ ] -- dejan de postear despues de un tiempo (puede ser un problema de reconexion a wifi si lo pierden o MQTT)


-- PARA el onboarding del miercoles
(HAY QUE TENER TODO ESTO EL LUNES 21 y HACER PRUEBAS HASTA EL MIERCOLES)

	[ ] -- CREDENCIALES input de credenciales por el usuario
	  	[ ] -- Terminar todos los pendientes de lightRead
	  		[ ] -- Verificar el funcionamiento de la versión Non-blocking
	  		[ ] -- Probarla con el codigo del onboarding en pantallas mate, móvil, reflejantes...
	  		[ ] -- Implementar una manera de reiniciar el lightRead cuando el CRC es correcto pero la conexion a wifi falla
	  	[ ] -- Web server en APmode como fallback para el lightRead
	  	[ ] -- Soporte para el configurador antiguo via serial.
	[ ] -- POSTING
		[ ] -- Simplificar el flow para cada publishReading
		[ ] -- Apagar y prender al ESP solo cuando se vaya a postear
		[ ] -- Verificar la rutina del ESP 
			[ ] -- Conexión Wifi 	(onWifi)
			[ ] -- SNTP				
			[ ] -- MQTT
				[ ] -- Crear cliente
				[ ] -- Suscripción con sesion persistente
				[ ] -- Recepcion de configuracion (por ahora solo UN intervalo)
				[ ] -- Publicación de readings y/o errores en su caso
				[ ] -- Aviso al SAM de status (algun error o todo bien) y luego irse a dormir
		[ ] -- Guardar lecturas cuando no sea exitosa la conexion (en el ESP?)
	[ ] -- CONFIGURACION
		[ ] -- Guardado de configuracion del SAM por lo menos los intervalos de lecturas
		[ ] -- Implementar el Factory reset para la configuracion del SAM (por ahora solo intervalo de lecturas)
		[ ] -- Asociar el veryLongPress a Factory Reset
	[ ] -- SONIDO
		[ ] -- Revision del codigo de jano.
		[ ] -- Pruebas de niveles con la referencia del sonómetro para calibrado
	[ ] -- POWER
		[ ] -- Pruebas de consumo y fórmula para estimación de duración de batería (sólo para el caso fixed de Making Sense)
		[ ] -- SAM sleep mode y wakeup para cada intervalo
		[ ] -- Leer y Reportar el nivel de bateria.
		[ ] -- Revisar y testear el charge de la bateria
	[ ] -- DEBUGGING
		[ ] -- Reporte de Sensor errors via mqtt
		[ ] -- Integrar en los readings deteccion de comportamientos anormales en los sensores(fuera de rangos, demasiado tiempo sin cambiar, etc)
	[ ] -- FEEDBACK
		[ ] -- revisar que el comportamiento del led sea consistente para cada caso
		[ ] -- Poner un led del wifi en fast pulsin on sending/receiving data.


NOTAS
-----BASE BOARD-------

** STATE MACHINE DESIGN (MODES)
  --it's important to get a very simple design
  --Make a list of triggers that changes the mode

** SHELL OBJECT:
	--Solo hay que pensar como manejar multiples outputs.
		Un output tipo broadcast siempre que se ejecute una accion, y que vaya a todos los outputs activos en ese momento
		De esta manera podemos escuchar todas las acciones desde cualquier canal.
		Para esto tendriamos que tener una lista donde podamos ver que outputs estan activos y en que nivel.
		Los niveles podrian ser 3: SILENT, MEDIUM and VERBOSE y cuando se establezca un canal de comunicacion se define su nivel.
		enum outLevels {OUT_SILENT, OUT_NORMAL, OUT_VERBOSE};
		enum prioLevels {PRIO_LOW, PRIO_MED, PRIO_HIGH};
		OUT_SILENT + PRIO_LOW = 0
		OUT_SILENT + PRIO_MED = 1
		OUT_SILENT + PRIO_HIGH = 2 *
		OUT_NORMAL + PRIO_LOW = 1
		OUT_NORMAL + PRIO_MED = 2 *
		OUT_NORMAL + PRIO_HIGH = 3 *
		OUT_VERBOSE + PRIO_LOW = 2 *
		OUT_VERBOSE + PRIO_MED = 3 *
		OUT_VERBOSE + PRIO_HIGH = 4 *
		output si la suma es mayor que 1

		** INPUT-OUTPUT
		  [ ] -- Diseñar una clase IOobject para crear instancias con USB, ESP, SDCARD, donde se definan sus caracteristicas (es in y/o out, nivel de out, es interactivo, esta activo.)
		  [x] -- sckIn receives a struct with a string and the origin (USB, ESP, LIGHT, etc), process the command, and outputs via sckOut
		  [x] -- sckOut recibe un string con el String y lo redirecciona a las outputs disponibles (USB,ESP, SDCARD), dependiendo del nivel de output y de la prioridad del mensaje
		  [x] -- Create an array to store command titles saving all the strings in flash
		  [x] -- FANCY in the case of interactive inputs, get a prompt!! ej. >, echo of what youre typing and a welcome message with some info.
		  [x] -- Revisar la libreria de cmdMessenger https://github.com/thijse/Arduino-CmdMessenger para ver si vale la pena hacer el cambio.
		  [ ] -- Migrar la solucion actual a cmdMessenger, el ejemplo ConsoleShell.ino tiene todo lo necesario...

** ESP COMMUNICATION
  [ ] -- Hacer que los commandos SAM <> ESP sean numericos
  [ ] -- Migrar todo el codigo a arduino
  [x] -- Esp control  (ESP_OFF, ESP_ON, ESP_REBOOT, etc)
  [x] -- Implementar una manera de salir de ESPbridging mode
  [x] -- Send Serial commands
  [ ] -- Get net status

** LED
  [x] -- Automatic change depending on mode
  [ ] -- Mejorar el Fade ROSA
  [ ] -- Integrate HSI and RGB colors in a tansparent way
  [ ] -- Smooth soft pulse with interrupt timers
  [ ] -- Funtions for exceptions (reading, config, etc)

** RTC - TIME
  [ ] --integrate time library with rtczero AND esp as sync providers... FIND OUT HOW
  	https://github.com/PaulStoffregen/Time
  	http://www.pjrc.com/teensy/td_libs_TimeAlarms.html
	-lo ideal seria que la time library maneje todo, esp, usb y rtc as syncproviders y setteando el rtc desde los otros dos providers.
	-despues poner encima de time la libreria timealarms que permite mucha versatilidad como time scheduler.

** SCHEDULER - TIMER SYSTEM
  [ ] -- usar timeAlarms

** BUTTON
  [x] -- Interrupt
  [x] -- Detect Down, Up, and shortPress
  [ ] -- Detect long and verylong (faltan los timers del RTC para que esto funcione)

** READING-LIGHT
  [ ] -- Debug the new nonbloking version
  [ ] -- Probar en todas las pantallas posibles y en moviles
  [ ] -- Hacer commit a github de la non blocking
  [x] -- Integrate library
  [x] -- Send esp command on succsesfull received credentials
  [x] -- Change led with checksum OK.
  [ ] -- Only use it if Urban board is present, if not, go to fallback

** CONFIGURATION
  [ ] -- Make hardcoded Default configuration
  [ ] -- Save configuration on eeprom
  [ ] -- get configuration from ESP

** POWER MANAGMENT
  [ ] -- Emergency mode with interrupt to wake up
  [ ] -- How on battery influences modes
  [ ] -- Diseñar bien la estrategia acerca del ESP y MICS

** BATTERY CHARGING
  [ ] -- Automatic managment of charge modes

** SDCARD
  [ ] -- Integrate SD library
  [ ] -- Detect if sdcard is present

** ACCELEROMETER


-----URBAN BOARD------

** DETECTION AND DIAGNOSIS

  --detect if urban present and run diagnosis

** SENSOR TEMPLATE

** SOUND
  --basic average
  --design modes

** TEMP AND HUMIDITY

** LIGHT

** GASES
  [ ] -- Revisar muy bien el codigo para ver lo del power consumption y asegurarse de que no cree problemas de reboots.

--------ESP8266-------
** POWER
  [ ] -- Buscar a fondo como disminuir el consumo de energia para evitar reboots cuando corre junto con el MICS

** APMODE
  [ ] -- Http GET ssid & pass
  [ ] -- HttpServer

** RTC - TIME
  [x] -- Update

** POSTS
  [ ] -- Http
  [x] -- MQTT
  [ ] -- SSL

** NET DEBUG OUTPUT
  [ ] -- Net console (netcat)
  [ ] -- Errors via mqtt

** STANDALONE MODE
  [ ] -- Http server (evitar que nada de esto use memoria en otros modos)

*/
