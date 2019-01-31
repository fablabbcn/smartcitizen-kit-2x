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

void SckBase::setup()
{
	/* delay(3000); */
	SerialUSB.println("Starting...");
	// TEMP turn off PMSpower
	pinMode(pinBOARD_CONN_7, OUTPUT);
	digitalWrite(pinBOARD_CONN_7, HIGH);

	// Led
	led.setup();

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	SerialESP.begin(serialBaudrate);
	manager.init();
	manager.setTimeout(30);
	manager.setRetries(16);
	ESPcontrol(ESP_ON);

	// Internal I2C bus setup
	Wire.begin();

	// Auxiliary I2C bus
	pinMode(pinPOWER_AUX_WIRE, OUTPUT);
	digitalWrite(pinPOWER_AUX_WIRE, LOW);	// LOW -> ON , HIGH -> OFF
	pinPeripheral(pinAUX_WIRE_SDA, PIO_SERCOM);
	pinPeripheral(pinAUX_WIRE_SCL, PIO_SERCOM);
	auxWire.begin();
	delay(2000); 				// Give some time for external boards to boot

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Power management configuration
	charger.setup();
	pinMode(pinBATT_INSERTION, INPUT_PULLUP);
	pinPeripheral(pinBATT_INSERTION, PIO_ANALOG);
	pinMode(pinGAUGE_INT, INPUT_PULLUP);
	attachInterrupt(pinGAUGE_INT, ISR_battery, FALLING);

	// RTC setup
	rtc.begin();
	if (rtc.isConfigured() && (rtc.getEpoch() > 1514764800)) st.timeStat.setOk();	// If greater than 01/01/2018
	else {
		rtc.setTime(0, 0, 0);
		rtc.setDate(1, 1, 15);
	}
	espStarted = rtc.getEpoch();

	// Sanity cyclic reset: If the clock is synced the reset will happen 3 hour after midnight (UTC) otherwise the reset will happen 3 hour after booting
	rtc.setAlarmTime(wakeUP_H, wakeUP_M, wakeUP_S);
	rtc.enableAlarm(rtc.MATCH_HHMMSS);
	rtc.attachInterrupt(NVIC_SystemReset);

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
	flashSelect();
	flash.begin();
	flash.setClock(133000);
	// flash.eraseChip(); // we need to do this on factory reset? and at least once on new kits.

/* #define autoTest  // Uncomment for doing Gases autotest, you also need to uncomment  TODO complete this */

#ifdef autoTest
	// TODO verify led blinking...
	ESPcontrol(ESP_OFF);
	sckOut("Starting Gases Pro Board automated test...", PRIO_HIGH);
	led.off();
	led.update(led.BLUE, led.PULSE_STATIC);
	String testResult = auxBoards.control(SENSOR_GASESBOARD_SLOT_1W, "autotest");
	SerialUSB.println(testResult);
	if (testResult.startsWith("1")) {
		sckOut("Test finished OK!!");
		led.update(led.GREEN, led.PULSE_STATIC);
	} else {
		sckOut("ERROR Test failed, please check your connections");
		led.update(led.RED, led.PULSE_STATIC);
	}
	while(true);
#endif

	// Configuration
	loadConfig();
	if (st.mode == MODE_NOT_CONFIGURED) writeHeader = true;

	bool saveNeeded = false;

	// Urban board
	analogReadResolution(12);
	if (urban.setup()) {
		sckOut("Urban board detected");

		// Check which urban board sensors are enabled
		uint8_t urbanSensorCount = 0;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			OneSensor *wichSensor = &sensors[sensors.sensorsPriorized(i)];
			if (wichSensor->location == BOARD_URBAN && wichSensor->enabled) urbanSensorCount++;
		}

		// If there is none enabled, we enable default sensors
		if (urbanSensorCount == 0) {
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				OneSensor *wichSensor = &sensors[sensors.sensorsPriorized(i)];
				if (wichSensor->location == BOARD_URBAN && wichSensor->defaultEnabled && !wichSensor->enabled) {
					enableSensor(wichSensor->type);
					saveNeeded = true;
				}
			}
		}

		if (!sensors[SENSOR_PM_10].enabled) enableSensor(SENSOR_PM_1); 	// Allow hotplug of PM sensor
		urban.stop(SENSOR_PM_1); 	// Make sure PM is off until battery is ready for it
		urbanPresent = true;

 		// Find out if urban was reinstalled just now
		bool justInstalled = true;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
			if (wichSensor->location == BOARD_URBAN && wichSensor->enabled) {
				justInstalled = false;
			}
		}

		if (justInstalled) {
			sckOut("Enabling default sensors...");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
				if (wichSensor->location == BOARD_URBAN) wichSensor->enabled = wichSensor->defaultEnabled;
			}
			saveConfig();
		}

	} else {
		sckOut("No urban board detected!!");
		urbanPresent = false;

 		// Find out if urban was removed just now
		bool justRemoved = false;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
			if (wichSensor->location == BOARD_URBAN && wichSensor->enabled) {
				justRemoved = true;
			}
		}

		if (justRemoved) {
			sckOut("Disabling sensors...");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
				if (wichSensor->location == BOARD_URBAN && wichSensor->enabled) disableSensor(wichSensor->type);
			}
			saveConfig();
		}
		urbanPresent = false;
	}

	// Detect and enable auxiliary boards
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		OneSensor *wichSensor = &sensors[sensors.sensorsPriorized(i)];

		if (wichSensor->location == BOARD_AUX) {
			if (enableSensor(wichSensor->type)) {
				wichSensor->enabled = true;
				saveNeeded = true;
			} else if (wichSensor->enabled)  {
				disableSensor(wichSensor->type);
				sprintf(outBuff, "Removed: %s... ", wichSensor->title);
				sckOut();
				wichSensor->enabled = false;
				saveNeeded = true;
			}
		}
	}

	if (saveNeeded) saveConfig();
}
void SckBase::update()
{
	if (millis() - reviewStateMillis > 500) {
		reviewStateMillis = millis();
		reviewState();
	}

	if (millis() - updatePowerMillis > 1000) {
		updatePowerMillis = millis();
		updatePower();
	}

	if (butState != butOldState) {
		buttonEvent();
		butOldState = butState;
		while(!butState) buttonStillDown();
	}
}

// **** Mode Control
void SckBase::reviewState()
{

	if (pendingSyncConfig) {
		sendConfig();
	}

	if (sdInitPending) sdInit();

	// SD card debug check file size and backup big files.
	if (config.sdDebug) {
		// Just do this every hour
		if (rtc.getEpoch() % 3600 == 0) {
			if (sdSelect()) {
				debugFile.file = sd.open(debugFile.name, FILE_WRITE);
				if (debugFile.file) {

					uint32_t debugSize = debugFile.file.size();

					// If file is bigger than 50mb rename the file.
					if (debugSize >= 52428800) debugFile.file.rename(sd.vwd(), "DEBUG01.TXT");
					debugFile.file.close();

				} else {
					st.cardPresent = false;
					st.cardPresentError = false;
				}

			}
		}
	}


	/* struct SckState { */
	/* bool onSetup --  in from enterSetup() and out from saveConfig()*/
	/* bool espON */
	/* bool wifiSet */
	/* bool tokenSet */
	/* bool helloPending */
	/* SCKmodes mode */
	/* bool cardPresent */
	/* bool sleeping */
	/* }; */

	/* state can be changed by: */
	/* loadConfig() */
	/* receiveMessage() */
	/* sdDetect() */
	/* buttonEvent(); */

	if (st.sleeping) {


	} else if (st.onShell) {


	} else if (st.onSetup) {


	} else if (st.mode == MODE_NOT_CONFIGURED) {

		if (!st.onSetup) enterSetup();

	} else if (st.mode == MODE_NET) {

		if (!st.wifiSet) {
			if (!st.wifiStat.error) {
				sckOut("ERROR wifi is not configured!!!");
				ESPcontrol(ESP_SLEEP);
				led.update(led.BLUE, led.PULSE_HARD_FAST);
				st.wifiStat.error = true;
			}
			return;
		}

		if (!st.tokenSet) {
			if (!st.tokenError) {
				sckOut("ERROR token is not configured!!!");
				ESPcontrol(ESP_SLEEP);
				led.update(led.BLUE, led.PULSE_HARD_FAST);
				st.tokenError = true;
			}
			return;
		}

		if (!st.helloPending) updateSensors();

		if (st.helloPending || !st.timeStat.ok || timeToPublish || !infoPublished) {

			if (!st.wifiStat.ok) {

				st.wifiStat.retry();

				if (!st.espON) ESPcontrol(ESP_ON);
				else if (st.wifiStat.error) {

					sckOut("ERROR Can't publish without wifi!!!");

					// TODO replace this with flash saving and in next publish push all flash readings
					sdPublish();

					ESPcontrol(ESP_OFF); 		// Hard off not sleep to be sure the ESP state is reset
					led.update(led.BLUE, led.PULSE_HARD_FAST);

					lastPublishTime = rtc.getEpoch();
					st.wifiStat.reset(); 		// Restart wifi retry count
				}

			} else {

				led.update(led.BLUE, led.PULSE_SOFT);

				if (st.helloPending) {

					if (st.helloStat.retry()) {

						if (sendMessage(ESPMES_MQTT_HELLO, ""))	sckOut("Hello sent!");

					} else if (st.helloStat.error) {

						sckOut("ERROR sending hello!!!");

						ESPcontrol(ESP_REBOOT); 		// Try reseting ESP
						led.update(led.BLUE, led.PULSE_HARD_FAST);

						st.helloStat.reset();
					}

				} else if (!st.timeStat.ok) {

					if (st.timeStat.retry()) {

						if (sendMessage(ESPMES_GET_TIME, "")) sckOut("Asking time to ESP...");

					} else if (st.timeStat.error) {

						sckOut("ERROR getting time from the network!!!");

						ESPcontrol(ESP_REBOOT);
						led.update(led.BLUE, led.PULSE_HARD_FAST);

						st.timeStat.reset();
					}

				} else if (!infoPublished) {

					if (st.infoStat.retry()) {

						if (publishInfo()) sckOut("Info sent!");

					} else if (st.infoStat.error){

						sckOut("ERROR sending kit info to platform!!!");
						st.infoStat.reset();

					}

				} else if (timeToPublish) {

					if (st.publishStat.ok) {

						// TODO go to sleep on receive MQTT success message
						sdPublish();

						ESPcontrol(ESP_SLEEP);
						timeToPublish = false;
						lastPublishTime = rtc.getEpoch();
						st.publishStat.reset(); 		// Restart publish error counter

					} else if (st.publishStat.error) {

						sckOut("Will retry on next publish interval!!!");

						// TODO replace this with flash saving and in next publish push all flash readings
						sdPublish();

						led.update(led.BLUE, led.PULSE_HARD_FAST);

						ESPcontrol(ESP_SLEEP);
						timeToPublish = false;
						lastPublishTime = rtc.getEpoch();
						st.publishStat.reset(); 		// Restart publish error counter
					} else if (st.publishStat.retry()) netPublish();
				}
			}
		}

	} else if  (st.mode == MODE_SD) {

		if (!st.cardPresent) {
			if (!st.cardPresentError) {
				sckOut("ERROR can't find SD card!!!");
				if (st.espON) ESPcontrol(ESP_SLEEP);
				led.update(led.PINK, led.PULSE_HARD_FAST);
				st.cardPresentError = true;
			}
			return;

		} else if (!st.timeStat.ok) {

			if (!st.wifiSet)  {
				if (!st.wifiStat.error) {
					sckOut("ERROR time is not synced and no wifi set!!!");
					ESPcontrol(ESP_SLEEP);
					led.update(led.PINK, led.PULSE_HARD_FAST);
					st.wifiStat.error = true;
				}
			} else {

				if (!st.wifiStat.ok) {

					st.wifiStat.retry();

					if (!st.espON) ESPcontrol(ESP_ON);
					else if (st.wifiStat.error) {

						sckOut("ERROR time is not synced!!!");

						ESPcontrol(ESP_SLEEP);
						led.update(led.PINK, led.PULSE_HARD_FAST);
						st.wifiStat.reset();
					}


				} else {

					if (st.timeStat.retry()) {

						if (sendMessage(ESPMES_GET_TIME, "")) sckOut("Asking time to ESP...");

					} else if (st.timeStat.error) {

						sckOut("ERROR time sync failed!!!");
						st.timeStat.reset();
						ESPcontrol(ESP_SLEEP);
						led.update(led.PINK, led.PULSE_HARD_FAST);
					}
				}
			}

		} else {
			led.update(led.PINK, led.PULSE_SOFT);
			if (st.espON) ESPcontrol(ESP_SLEEP);

			updateSensors();

			if (timeToPublish) {
				if (!sdPublish()) {
					sckOut("ERROR failed publishing to SD card");
					led.update(led.PINK, led.PULSE_HARD_FAST);
				}
				timeToPublish = false;
				lastPublishTime = rtc.getEpoch();
			}
		}
	}
}
void SckBase::enterSetup()
{
	sckOut("Entering setup mode", PRIO_LOW);
	st.onSetup = true;

	// Update led
	led.update(led.RED, led.PULSE_SOFT);

	// Clear errors from other modes
	st.tokenError = false;
	st.cardPresentError = false;

	// Start wifi APmode
	if (!st.espON) ESPcontrol(ESP_ON);
	else if (sendMessage(ESPMES_START_AP, "")) sckOut("Started Access point on ESP");
}
void SckBase::printState()
{
	char t[] = "true";
	char f[] = "false";

	sprintf(outBuff, "%sonSetup: %s\r\n", outBuff, st.onSetup  ? t : f);
	sprintf(outBuff, "%stokenSet: %s\r\n", outBuff, st.tokenSet  ? t : f);
	sprintf(outBuff, "%shelloPending: %s\r\n", outBuff, st.helloPending  ? t : f);
	sprintf(outBuff, "%smode: %s\r\n", outBuff, modeTitles[st.mode]);
	sprintf(outBuff, "%scardPresent: %s\r\n", outBuff, st.cardPresent  ? t : f);
	sprintf(outBuff, "%ssleeping: %s\r\n", outBuff, st.sleeping  ? t : f);

	sprintf(outBuff, "%s\r\nespON: %s\r\n", outBuff, st.espON  ? t : f);
	sprintf(outBuff, "%s\r\nespBooting: %s\r\n", outBuff, st.espBooting  ? t : f);
	sprintf(outBuff, "%swifiSet: %s\r\n", outBuff, st.wifiSet  ? t : f);
	sprintf(outBuff, "%swifiOK: %s\r\n", outBuff, st.wifiStat.ok ? t : f);
	sprintf(outBuff, "%swifiError: %s\r\n", outBuff, st.wifiStat.error ? t : f);
	sckOut(PRIO_HIGH, false);

	sprintf(outBuff, "\r\ntimeOK: %s\r\n", st.timeStat.ok ? t : f);
	sprintf(outBuff, "%stimeError: %s\r\n", outBuff, st.timeStat.error ? t : f);

	sprintf(outBuff, "%s\r\npublishOK: %s\r\n", outBuff, st.publishStat.ok ? t : f);
	sprintf(outBuff, "%spublishError: %s\r\n", outBuff, st.publishStat.error ? t : f);
	sprintf(outBuff, "%s\r\ntime to next publish: %li\r\n", outBuff, config.publishInterval - (rtc.getEpoch() - lastPublishTime));

	sckOut(PRIO_HIGH, false);
}

// **** Input
void SckBase::inputUpdate()
{

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
void SckBase::sckOut(String strOut, PrioLevels priority, bool newLine)
{
	if (strOut.equals(outBuff)) {
		outRepetitions++;
		if (outRepetitions >= 10) {
			sckOut("Last message repeated 10 times");
			outRepetitions = 0;
		}
		return;
	}
	outRepetitions = 0;
	strOut.toCharArray(outBuff, strOut.length()+1);
	sckOut(priority, newLine);
}
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine)
{
	if (strncmp(strOut, outBuff, strlen(strOut)) == 0) {
		outRepetitions++;
		if (outRepetitions >= 10) {
			sckOut("Last message repeated 10 times");
			outRepetitions = 0;
		}
		return;
	}
	outRepetitions = 0;
	strncpy(outBuff, strOut, 240);
	sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine)
{
	// Output via USB console
	if (charger.onUSB) {
		if (outputLevel + priority > 1) {
			if (newLine) SerialUSB.println(outBuff);
			else SerialUSB.print(outBuff);
		}
	} else  {
		digitalWrite(pinLED_USB, HIGH);
	}

	// Debug output to sdcard
	if (config.sdDebug) {
		if (!sdSelect()) return;
		debugFile.file = sd.open(debugFile.name, FILE_WRITE);
		if (debugFile.file) {
			ISOtime();
			debugFile.file.print(ISOtimeBuff);
			debugFile.file.print("-->");
			debugFile.file.println(outBuff);
			debugFile.file.close();
		} else st.cardPresent = false;
	}
}
void SckBase::prompt()
{
	sprintf(outBuff, "%s", "SCK > ");
	sckOut(PRIO_MED, false);
}

// **** Config
void SckBase::loadConfig()
{

	sckOut("Loading configuration from eeprom...");

	Configuration savedConf = eepromConfig.read();

	if (savedConf.valid) config = savedConf;
	else {
		// TODO check if there is a valid sdcard config and load it
		sckOut("Can't find valid configuration!!! loading defaults...");
		saveConfig(true);
	}

	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
		wichSensor->enabled = config.sensors[i].enabled;
		wichSensor->everyNint = config.sensors[i].everyNint;
	}

	// If battery capacity is not set, update it
	if (config.battDesignCapacity != battery.designCapacity) {
		battery.designCapacity = config.battDesignCapacity;
		battery.setup(charger, true);
	}

	st.wifiSet = config.credentials.set;
	st.tokenSet = config.token.set;
	st.tokenError = false;
	st.mode = config.mode;
}
void SckBase::saveConfig(bool defaults)
{
	// Save to eeprom
	if (defaults) {
		Configuration defaultConfig;

		if (config.mac.valid) macAddress = String(config.mac.address); 	// If we already have a mac address keep it

		config = defaultConfig;

		if (macAddress.length() > 0) {
			sprintf(config.mac.address, "%s", macAddress.c_str());
			config.mac.valid = true;
		} else {
			config.mac.valid = false;
		}

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			config.sensors[i].enabled = sensors[static_cast<SensorType>(i)].defaultEnabled;
			config.sensors[i].everyNint = 1;
		}
		pendingSyncConfig = true;
	} else {
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
			config.sensors[i].enabled = wichSensor->enabled;
			config.sensors[i].everyNint = wichSensor->everyNint;
		}
	}
	eepromConfig.write(config);
	sckOut("Saved configuration on eeprom!!", PRIO_LOW);

	// If battery capacity changed, update it
	if (config.battDesignCapacity != battery.designCapacity) {
		battery.designCapacity = config.battDesignCapacity;
		battery.setup(charger, true);
	}

	// Update state
	st.mode = config.mode;
	st.wifiSet = config.credentials.set;
	st.tokenSet = config.token.set;
	st.tokenError = false;
	st.wifiStat.reset();
	lastPublishTime = rtc.getEpoch() - config.publishInterval;
	lastSensorUpdate = rtc.getEpoch() - config.readInterval;

	if (st.wifiSet || st.tokenSet) pendingSyncConfig = true;

	// Decide if new mode its valid
	if (st.mode == MODE_NET) {

		if (st.wifiSet && st.tokenSet) {

			infoPublished = false;
			st.helloPending = true;
			st.onSetup = false;
			led.update(led.BLUE, led.PULSE_SOFT);
			sendMessage(ESPMES_STOP_AP, "");

		} else {

			if (!st.wifiSet) sckOut("ERROR Wifi not configured: can't set Network Mode!!!");
			if (!st.tokenSet) sckOut("ERROR Token not configured: can't set Network Mode!!!");
			ESPcontrol(ESP_SLEEP);
			led.update(led.BLUE, led.PULSE_HARD_FAST);
		}

	} else if (st.mode == MODE_SD) {

		st.helloPending = false;
		st.onSetup = false;
		led.update(led.PINK, led.PULSE_SOFT);
		sendMessage(ESPMES_STOP_AP, "");

	}

	if (pendingSyncConfig && !st.espON) ESPcontrol(ESP_ON);
}
Configuration SckBase::getConfig()
{

	return config;
}
bool SckBase::sendConfig()
{
	if (!st.espON) {
		ESPcontrol(ESP_ON);
		return false;
	}
	if (st.espBooting) return false;

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;
	json["ver"] = SAMversion;
	json["bd"] = SAMbuildDate;

	if ((st.mode == MODE_NET && st.wifiSet && st.tokenSet) || (st.mode == MODE_SD && st.wifiSet && !st.wifiStat.error)) json["ac"] = (uint8_t)ESPMES_CONNECT;
	else json["ac"] = (uint8_t)ESPMES_START_AP;

	sprintf(netBuff, "%c", ESPMES_SET_CONFIG);
	json.printTo(&netBuff[1], json.measureLength() + 1);

	if (sendMessage()) {
		pendingSyncConfig = false;
		sckOut("Synced config with ESP!!", PRIO_LOW);
		return true;
	}

	return false;
}
bool SckBase::publishInfo()
{
	// Info file
	if (!espInfoUpdated) sendMessage(ESPMES_GET_NETINFO);
	else {
		// Publish info to platform

		/* { */
		/* 	"time":"2018-07-17T06:55:06Z", */
		/* 	"hw_ver":"2.0", */
		/* 	"id":"6C4C1AF4504E4B4B372E314AFF031619", */
		/* 	"sam_ver":"0.3.0-ce87e64", */
		/* 	"sam_bd":"2018-07-17T06:55:06Z", */
		/* 	"mac":"AB:45:2D:33:98", */
		/* 	"esp_ver":"0.3.0-ce87e64", */
		/* 	"esp_bd":"2018-07-17T06:55:06Z" */
		/* } */

		if (!st.espON) {
			ESPcontrol(ESP_ON);
			return false;
		}

		getUniqueID();

		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();

		json["time"] = ISOtimeBuff;
		json["hw_ver"] = hardwareVer.c_str();
		json["id"] = uniqueID_str;
		json["sam_ver"] = SAMversion.c_str();
		json["sam_bd"] = SAMbuildDate.c_str();
		json["mac"] = config.mac.address;
		json["esp_ver"] = ESPversion.c_str();
		json["esp_bd"] = ESPbuildDate.c_str();

		sprintf(netBuff, "%c", ESPMES_MQTT_INFO);
		json.printTo(&netBuff[1], json.measureLength() + 1);
		if (sendMessage()) return true;
	}
	return false;
}

// **** ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand)
{
	switch(controlCommand){
		case ESP_OFF:
		{
				sckOut("ESP off...", PRIO_LOW);
				st.espON = false;
				digitalWrite(pinESP_CH_PD, LOW);
				digitalWrite(pinPOWER_ESP, HIGH);
				digitalWrite(pinESP_GPIO0, LOW);
				sprintf(outBuff, "Esp was on for %lu seconds", (rtc.getEpoch() - espStarted));
				espStarted = 0;
				break;
		}
		case ESP_FLASH:
		{
				led.update(led.WHITE, led.PULSE_STATIC);

				SerialESP.begin(espFlashSpeed);
				delay(100);

				digitalWrite(pinESP_CH_PD, LOW);
				digitalWrite(pinPOWER_ESP, HIGH);
				digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
				delay(100);

				digitalWrite(pinESP_CH_PD, HIGH);
				digitalWrite(pinPOWER_ESP, LOW);

				uint32_t flashTimeout = millis();
				uint32_t startTimeout = millis();
				while(1) {
					if (SerialUSB.available()) {
						SerialESP.write(SerialUSB.read());
						flashTimeout = millis();
					}
					if (SerialESP.available()) {
						SerialUSB.write(SerialESP.read());
					}
					if (millis() - flashTimeout > 1000) {
						if (millis() - startTimeout > 10000) sck_reset();  // Initial 10 seconds for the flashing to start
					}
				}
				break;
		}
		case ESP_ON:
		{
				if (st.espBooting) return;
				sckOut("ESP on...", PRIO_LOW);
				digitalWrite(pinESP_CH_PD, HIGH);
				digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
				digitalWrite(pinPOWER_ESP, LOW);
				st.wifiStat.reset();
				st.espON = true;
				st.espBooting = true;
				espStarted = rtc.getEpoch();
				break;

		}
		case ESP_REBOOT:
		{
				sckOut("Restarting ESP...", PRIO_LOW);
				ESPcontrol(ESP_OFF);
				delay(50);
				ESPcontrol(ESP_ON);
				break;
		}
		case ESP_WAKEUP:
		{
				sckOut("ESP wake up...");
				digitalWrite(pinESP_CH_PD, HIGH);
				st.espON = true;
				espStarted = rtc.getEpoch();
				break;
		}
		case ESP_SLEEP:
		{
				sckOut("ESP deep sleep...", PRIO_LOW);
				sendMessage(ESPMES_LED_OFF);
				st.espON = false;
				digitalWrite(pinESP_CH_PD, LOW);
				sprintf(outBuff, "Esp was awake for %lu seconds", (rtc.getEpoch() - espStarted));
				sckOut(PRIO_LOW);
				espStarted = 0;
				break;
		}
	}
}
void SckBase::ESPbusUpdate()
{
	if (manager.available()) {

		uint8_t len = NETPACK_TOTAL_SIZE;

		if (manager.recvfromAck(netPack, &len)) {

			if (debugESPcom) {
				sprintf(outBuff, "Receiving msg from ESP in %i parts", netPack[0]);
				sckOut();
			}

			// Identify received command
			uint8_t pre = netPack[1];
			SAMMessage wichMessage = static_cast<SAMMessage>(pre);

			// Get content from first package (1 byte less than the rest)
			memcpy(netBuff, &netPack[2], NETPACK_CONTENT_SIZE - 1);

			// Get the rest of the packages (if they exist)
			for (uint8_t i=0; i<netPack[0]-1; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	{
					memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE) + (NETPACK_CONTENT_SIZE - 1)], &netPack[1], NETPACK_CONTENT_SIZE);
				}
				else return;
			}

			if (debugESPcom) sckOut(netBuff);

			// Process message
			receiveMessage(wichMessage);
		}
	}
}
bool SckBase::sendMessage(ESPMessage wichMessage)
{
	sprintf(netBuff, "%c", wichMessage);
	return sendMessage();
}
bool SckBase::sendMessage(ESPMessage wichMessage, const char *content)
{
	sprintf(netBuff, "%c%s", wichMessage, content);
	return sendMessage();
}
bool SckBase::sendMessage()
{

	// This function is used when netbuff is already filled with command and content

	if (!st.espON || st.espBooting) {
		if (debugESPcom) sckOut("Can't send message, ESP is off or still booting...");
		return false;
	}

	uint16_t totalSize = strlen(netBuff);
	uint8_t totalParts = (totalSize + NETPACK_CONTENT_SIZE - 1)  / NETPACK_CONTENT_SIZE;

	if (debugESPcom) {
		sprintf(outBuff, "Sending msg to ESP with %i parts and %i bytes", totalParts, totalSize);
		sckOut();
		SerialUSB.println(netBuff);
	}

	for (uint8_t i=0; i<totalParts; i++) {
		netPack[0] = totalParts;
		memcpy(&netPack[1], &netBuff[(i * NETPACK_CONTENT_SIZE)], NETPACK_CONTENT_SIZE);
		if (!manager.sendtoWait(netPack, NETPACK_TOTAL_SIZE, ESP_ADDRESS)) {
			sckOut("ERROR sending mesg to ESP!!!");
			return false;
		}
		if (debugESPcom) {
			sprintf(outBuff, "Sent part num %i", i);
			sckOut();
		}
	}
	return true;
}
void SckBase::receiveMessage(SAMMessage wichMessage)
{
	switch(wichMessage) {
		case SAMMES_SET_CONFIG:
		{

				sckOut("Received new config from ESP");
				StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(netBuff);

				if (json.containsKey("mo")) {
					String stringMode = json["mo"];
					if (stringMode.startsWith("net")) config.mode = MODE_NET;
					else if (stringMode.startsWith("sd")) config.mode = MODE_SD;
				} else config.mode = MODE_NOT_CONFIGURED;

				if (json.containsKey("pi")) {
					if (json["pi"] > minimal_publish_interval && json["pi"] < max_publish_interval)	config.publishInterval = json["pi"];
				} else config.publishInterval = default_publish_interval;

				if (json.containsKey("ss")) {
					config.credentials.set = true;
					strcpy(config.credentials.ssid, json["ss"]);
					if (json.containsKey("pa")) strcpy(config.credentials.pass, json["pa"]);
				} else config.credentials.set = false;


				if (json.containsKey("to")) {
					config.token.set = true;
					strcpy(config.token.token, json["to"]);
				} else config.token.set = false;

				st.helloPending = true;
				saveConfig();
				break;

		}
		case SAMMES_DEBUG:
		{

				sckOut("ESP --> ", PRIO_HIGH, false);
				sckOut(netBuff);
				break;

		}
		case SAMMES_NETINFO:
		{
				StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(netBuff);
				ipAddress = json["ip"].as<String>();
				hostname = json["hn"].as<String>();

				sprintf(outBuff, "\r\nHostname: %s\r\nIP address: %s\r\nMAC address: %s", hostname.c_str(), ipAddress.c_str(), macAddress.c_str());
				sckOut();
				sprintf(outBuff, "ESP version: %s\r\nESP build date: %s", ESPversion.c_str(), ESPbuildDate.c_str());
				sckOut();

				break;
		}
		case SAMMES_WIFI_CONNECTED:

			sckOut("Conected to wifi!!", PRIO_LOW); st.wifiStat.setOk(); break;

		case SAMMES_SSID_ERROR:

			sckOut("ERROR Access point not found!!"); st.wifiStat.error = true; break;

		case SAMMES_PASS_ERROR:

			sckOut("ERROR wrong wifi password!!"); st.wifiStat.error = true; break;

		case SAMMES_WIFI_UNKNOWN_ERROR:

			sckOut("ERROR unknown wifi error!!"); st.wifiStat.error = true; break;

		case SAMMES_TIME:
		{
				String strTime = String(netBuff);
				setTime(strTime);
				break;
		}
		case SAMMES_MQTT_HELLO_OK:
		{
				st.helloPending = false;
				st.helloStat.setOk();
				sckOut("Hello OK!!");
				break;
		}
		case SAMMES_MQTT_PUBLISH_OK:

			st.publishStat.setOk();
			sckOut("Network publish OK!!   ");
			break;

		case SAMMES_MQTT_PUBLISH_ERROR:

			sckOut("ERROR on MQTT publish");
			st.publishStat.error = true;
			break;

		case SAMMES_MQTT_INFO_OK:

			st.infoStat.setOk();
			infoPublished = true;
			sckOut("Info publish OK!!");
			break;

		case SAMMES_MQTT_INFO_ERROR:

			st.infoStat.error = true;
			sckOut("ERROR on Info publish!!");
			break;

		case SAMMES_MQTT_CUSTOM_OK:

			sckOut("Custom MQTT publish OK!!");
			break;

		case SAMMES_MQTT_CUSTOM_ERROR:

			sckOut("ERROR on custom MQTT publish");
			break;

		case SAMMES_BOOTED:
		{
			sckOut("ESP finished booting");

			st.espBooting = false;

			StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(netBuff);
			macAddress = json["mac"].as<String>();
			ESPversion = json["ver"].as<String>();
			ESPbuildDate = json["bd"].as<String>();

			// Udate mac address if we haven't yet
			if (!config.mac.valid) {
				sckOut("Updated MAC address");
				sprintf(config.mac.address, "%s", macAddress.c_str());
				config.mac.valid = true;
				saveConfig();
			}

			if (!espInfoUpdated) {
				espInfoUpdated = true;
				saveInfo();
			}

			sendConfig();
			break;
		}
		default: break;
	}
}
void SckBase::mqttCustom(const char *topic, const char *payload)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["to"] = topic;
	json["pl"] = payload;

	sprintf(netBuff, "%c", ESPMES_MQTT_CUSTOM);
	json.printTo(&netBuff[1], json.measureLength() + 1);

	if (sendMessage()) sckOut("MQTT message sent to ESP...", PRIO_LOW);
}

// **** SD card
bool SckBase::sdInit()
{
	sdInitPending = false;

	if (sd.begin(pinCS_SDCARD, SPI_HALF_SPEED)) {
		sckOut("Sd card ready to use");
		st.cardPresent = true;
		st.cardPresentError = false;
		return true;
	}
	sckOut("ERROR on Sd card Init!!!");
	st.cardPresent = false; 	// If we cant initialize sdcard, don't use it!
	st.cardPresentError = false;
	return false;
}
bool SckBase::sdDetect()
{
	st.cardPresent = !digitalRead(pinCARD_DETECT);
	st.cardPresentError = false;

	if (!digitalRead(pinCARD_DETECT)) {
		sckOut("Sdcard inserted");
		sdInitPending = true;
	} else sckOut("No Sdcard found!!");
	return false;
}
bool SckBase::sdSelect()
{

	if (!st.cardPresent) return false;

	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash
	digitalWrite(pinCS_SDCARD, LOW);

	return true;
}
bool SckBase::saveInfo()
{
	// Save info to sdcard
	if (!infoSaved) {
		if (sdSelect()) {
			if (sd.exists(infoFile.name)) sd.remove(infoFile.name);
			infoFile.file = sd.open(infoFile.name, FILE_WRITE);
			getUniqueID();
			sprintf(outBuff, "Hardware Version: %s\r\nSAM Hardware ID: %s\r\nSAM version: %s\r\nSAM build date: %s", hardwareVer.c_str(), uniqueID_str, SAMversion.c_str(), SAMbuildDate.c_str());
			infoFile.file.println(outBuff);
			sprintf(outBuff, "ESP MAC address: %s\r\nESP version: %s\r\nESP build date: %s\r\n", config.mac.address, ESPversion.c_str(), ESPbuildDate.c_str());
			infoFile.file.println(outBuff);
			infoFile.file.close();
			infoSaved = true;
			sckOut("Saved INFO.TXT file!!");
			return true;
		}
	}
	return false;
}

// **** Flash memory
void SckBase::flashSelect()
{
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
}

// **** Power
void SckBase::sck_reset()
{
	sckOut("Bye!!");
	NVIC_SystemReset();
}
void SckBase::goToSleep()
{
	digitalWrite(pinLED_USB, HIGH);
	led.off();
	ESPcontrol(ESP_SLEEP);

	// ESP control pins savings
	digitalWrite(pinESP_CH_PD, LOW);
	digitalWrite(pinESP_GPIO0, LOW);
	digitalWrite(pinESP_RX_WIFI, LOW);
	digitalWrite(pinESP_TX_WIFI, LOW);


	// TODO MICS heaters saving
	// TODO checke every component for power optimizations

	// Disconnect USB
	/* USBDevice.detach(); */
	/* USBDeviceAttached = false; */

	// Stop PM sensor
	if (urban.sck_pm.started) urban.sck_pm.stop();

	if (sleepTime > 0) sprintf(outBuff, "Sleeping for %lu seconds", (sleepTime) / 1000);
	else sprintf(outBuff, "Sleeping forever!!! (until a button click)");
	sckOut();

	uint32_t localSleepTime = sleepTime;
	sleepTime = 0;

	if (localSleepTime > 0) LowPower.deepSleep(localSleepTime);
	else {
		// Disable the Sanity cyclic reset so it doesn't wake us up
		rtc.disableAlarm();
		rtc.detachInterrupt();

		LowPower.deepSleep();
	}
}
void SckBase::updatePower()
{

	// Update battery present status
	bool prevBattPresent = battery.present;
	battery.isPresent(charger);
	bool battChanged = false;

	// Update USB connection status
	charger.detectUSB();

	// If battery status changed enable/disable charging
	if (prevBattPresent != battery.present) {

		battChanged = true;

		if (battery.present) {
			sckOut("Battery inserted!!");
			if (!charger.chargeState()) charger.chargeState(true); 	// Enable charging
		} else {
			sckOut("Battery removed!!");
			sckOut("Stoping PM sensor...");
			charger.chargeState(false); 	// Disable charging
			urban.sck_pm.stop();
			led.chargeStatus = led.CHARGE_NULL; 	// No led feedback if no battery
		}
	}

	if (battPendingEvent) {
		battery.percent();
		sprintf(outBuff, "Battery changed: %u %%", battery.lastPercent);
		sckOut();
		battPendingEvent = false;
	}

	if (charger.onUSB) {

		// Reset lowBatt counter
		battery.lowBatCounter = 0;

		// Reset emergencyLowBatt counter
		battery.emergencyLowBatCounter = 0;

		// Update charge status
		SckCharger::ChargeStatus prevChargeStatus = charger.chargeStatus;
		charger.chargeStatus = charger.getChargeStatus();

		bool justStoppedCharging = false;
		// If charger status changed
		if (prevChargeStatus != charger.chargeStatus || battChanged) {

			if (battery.present) {

				switch(charger.chargeStatus) {
					case charger.CHRG_PRE_CHARGE:
					case charger.CHRG_FAST_CHARGING:

						sckOut("Charging battery...");
						led.chargeStatus = led.CHARGE_CHARGING;
						break;

					case charger.CHRG_NOT_CHARGING:
					case charger.CHRG_CHARGE_TERM_DONE:
						if (charger.chargeState()) charger.chargeState(false);
						// Verify again that battery is present
						if (battery.isPresent(charger)) {
							sckOut("Battery fully charged");
							led.chargeStatus = led.CHARGE_FINISHED;
							justStoppedCharging = true;
						}
						break;

					default: break;
				}

			} else {
				if (charger.chargeState()) charger.chargeState(false);
				sckOut("Battery is not charging");
				led.chargeStatus = led.CHARGE_NULL; 	// No led feedback if no battery
			}
		}

		// Avoid PM sensor discharging battery when USB connected
		if (battery.lastPercent < battery.threshold_recharge && battery.present && !justStoppedCharging) charger.chargeState(true);

	} else {

		// Emergency lowBatt
		if (battery.lastPercent < battery.threshold_emergency) {
			if (battery.emergencyLowBatCounter < 5) {
				battery.emergencyLowBatCounter++;
				led.chargeStatus = led.CHARGE_NULL;
			} else led.chargeStatus = led.CHARGE_LOW;
			// TODO replace this with proper led feeback and sleep mode
			/* led.powerEmergency = true; */

		// Detect lowBatt
		} else if (battery.lastPercent < battery.threshold_low) {
			if (battery.lowBatCounter < 5) {
				battery.lowBatCounter++;
				led.chargeStatus = led.CHARGE_NULL;
			} else led.chargeStatus = led.CHARGE_LOW;

		} else {
			sckOut("Battery is not charging");
			led.chargeStatus = led.CHARGE_NULL; 	// No led feedback if no battery

		}
	}


	// TODO rmove this when oneShot mode is integrated
	// PM sensor only works if battery is available
	if (sensors[SENSOR_PM_1].enabled && !st.sleeping) {
		if (!urban.sck_pm.started) {
			if (millis() > 10000 && battery.present) {
				if (battery.lastPercent > battery.threshold_emergency || charger.chargeStatus == charger.CHRG_FAST_CHARGING) {
					if (urban.sck_pm.start()) sckOut("Started PM sensor...");
				}
			}
		}
	}
}
/* void SckBase::wakeUp() */
/* { */
/* 	sckOut("Waked up!!!"); */
/* } */

// **** Sensors
void SckBase::updateSensors()
{
	if (!st.timeStat.ok || st.helloPending) return;
	if (st.onSetup) return;
	if (st.mode == MODE_SD && !st.cardPresent) return;
	else if (st.mode == MODE_NET && st.wifiStat.error) return;

	// Main reading loop
	if (rtc.getEpoch() - lastSensorUpdate >= config.readInterval) {
		
		lastSensorUpdate = rtc.getEpoch();

		// TODO  create new RAM group with this timestamp

		sckOut("\r\n-----------", PRIO_LOW);
		ISOtime();
		sckOut(ISOtimeBuff, PRIO_LOW);

		for (uint8_t i=0; i<SENSOR_COUNT; i++) {

			// Get next sensor based on priority
			OneSensor wichSensor = sensors[sensors.sensorsPriorized(i)];

			// Check if it is enabled
			if (wichSensor.enabled) {

				// Is time to read it?
				if ((lastSensorUpdate - wichSensor.lastReadingTime) >= (wichSensor.everyNint * config.readInterval)) {

					getReading(&wichSensor);

					if (wichSensor.state == 0) {

						wichSensor.lastReadingTime = lastSensorUpdate;

						// TODO Save reading on RAM/Flash

						sprintf(outBuff, "%s: %s %s", wichSensor.title, wichSensor.reading.c_str(), wichSensor.unit);
						sckOut(PRIO_LOW);

					} else if (wichSensor.state > 0) {
					
						// Append pending sensors to pending list
						pendingSensors ++;
						pendingSensorsList[pendingSensors - 1] = wichSensor.type;

					} else if (wichSensor.state == -1) {
					
						// Sensor reading ERROR
						// TODO save null value
					}
				}
			}
		}

	// If some sensor is pendig to be reapending d
	} else if (pendingSensors > 0) {

		SensorType tmpPendingSensorList[pendingSensors];
		uint8_t tmpPendingSensors = 0;
		
		for (uint8_t i=0; i<pendingSensors; i++) {

			OneSensor wichSensor = sensors[pendingSensorsList[i]];
			getReading(&wichSensor);

			if (wichSensor.state == 0) {
			
				wichSensor.lastReadingTime = lastSensorUpdate;

				// TODO Save reading on RAM/Flash
				
				sprintf(outBuff, "%s: %s %s", wichSensor.title, wichSensor.reading.c_str(), wichSensor.unit);
				sckOut(PRIO_LOW);
				

			} else if (wichSensor.state > 0) {

				// Reappend the sensor to the pending list
				tmpPendingSensorList[i] = wichSensor.type;
				tmpPendingSensors ++;

			} else if (wichSensor.state == -1) {
			
				// ERROR on getting reading
				// TODO save a null value
				
			}
		}

		pendingSensors = tmpPendingSensors;
		for (uint8_t i=0; i<pendingSensors; i++) pendingSensorsList[i] = tmpPendingSensorList[i];
	}


	if (rtc.getEpoch() - lastPublishTime >= config.publishInterval) {
		timeToPublish = true;
	}
}
bool SckBase::enableSensor(SensorType wichSensor)
{
	bool result = false;
	switch (sensors[wichSensor].location) {
		case BOARD_BASE:
		{
			switch (wichSensor) {
				case SENSOR_BATT_PERCENT:
				 	// Allow enabling battery even if its not present so it can be posted to platform (reading will return -1 if the batery is not present)
					result = true;
					break;
				case SENSOR_BATT_VOLTAGE:
				case SENSOR_BATT_CHARGE_RATE:
				case SENSOR_BATT_POWER:
					if (battery.isPresent(charger)) result = true;
					break;
				case SENSOR_SDCARD:
					result = true;
					break;
				default: break;
			}
		}
		case BOARD_URBAN: if (urban.start(wichSensor)) result = true; break;
		case BOARD_AUX:	{
					if (auxBoards.start(wichSensor)) result = true;
					break;
				}
		default: break;
	}

	if (result) {
		sprintf(outBuff, "Enabling %s", sensors[wichSensor].title);
		sensors[wichSensor].enabled = true;
		sckOut();
		return true;
	}

	return false;
}
bool SckBase::disableSensor(SensorType wichSensor)
{
	bool result = false;
	switch (sensors[wichSensor].location) {
		case BOARD_BASE:
		{
			switch (wichSensor) {
				case SENSOR_BATT_PERCENT:
				case SENSOR_BATT_VOLTAGE:
				case SENSOR_BATT_CHARGE_RATE:
				case SENSOR_BATT_POWER:
				case SENSOR_SDCARD:
					result = true;
					break;
				default: break;
			}
		}
		case BOARD_URBAN: if (urban.stop(wichSensor)) result = true; break;
		case BOARD_AUX: if (auxBoards.stop(wichSensor)) result = true; break;
		default: break;
	}

	if (result) {
		sprintf(outBuff, "Disabling %s", sensors[wichSensor].title);
		sensors[wichSensor].enabled = false;
		sckOut();
		return true;
	}

	return false;
}
void SckBase::getReading(OneSensor *wichSensor)
{
	switch (wichSensor->location) {
		case BOARD_BASE:
		{
				switch (wichSensor->type) {
					case SENSOR_BATT_PERCENT:
					{
						if (!battery.isPresent(charger)) {
							wichSensor->reading = String("-1");
							break;
						}
						uint32_t thisPercent = battery.percent();
						if (thisPercent > 100) thisPercent = 100;
						else if (thisPercent < 0) thisPercent = 0;
						wichSensor->reading = String(thisPercent);
						break;
					}
					case SENSOR_BATT_VOLTAGE:
						if (!battery.isPresent(charger)) {
							wichSensor->reading = String("-1");
							break;
						}
						wichSensor->reading = String(battery.voltage());
						break;

					case SENSOR_BATT_CHARGE_RATE:
						if (!battery.isPresent(charger)) {
							wichSensor->reading = String("-1");
							break;
						}
						wichSensor->reading = String(battery.current());
						break;
					case SENSOR_BATT_POWER:

						if (!battery.isPresent(charger)) {
							wichSensor->reading = String("-1");
							break;
						}
						wichSensor->reading = String(battery.power());
						break;
					case SENSOR_SDCARD:
						if (st.cardPresent) wichSensor->reading = String("1");
						else wichSensor->reading = String("0");
						break;
					default: break;
				}
				wichSensor->state = 0;
				break;
		}
		case BOARD_URBAN:
		{
				urban.getReading(wichSensor);
				break;
		}
		case BOARD_AUX:
		{
				auxBoards.getReading(wichSensor, this);
				break;
		}
	}
}
bool SckBase::controlSensor(SensorType wichSensorType, String wichCommand)
{
	if (sensors[wichSensorType].controllable)  {
		sprintf(outBuff, "%s: %s", sensors[wichSensorType].title, wichCommand.c_str());
		sckOut();
		switch (sensors[wichSensorType].location) {
				case BOARD_URBAN: urban.control(this, wichSensorType, wichCommand); break;
				case BOARD_AUX: sckOut(auxBoards.control(wichSensorType, wichCommand)); break;
				default: break;
			}

	} else {
		sprintf(outBuff, "No configured command found for %s sensor!!!", sensors[wichSensorType].title);
		sckOut();
		return false;
	}
	return true;
}
bool SckBase::netPublish()
{

	if (!st.espON) {
		ESPcontrol(ESP_ON);
		return false;
	}

	// /* Example
		// {	"data":[
		// 		{"recorded_at":"2017-03-24T13:35:14Z",
		// 			"sensors":[
		// 				{"id":29,"value":48.45},
		// 				{"id":13,"value":66},
		// 				{"id":12,"value":28},
		// 				{"id":10,"value":4.45}
		// 			]
		// 		}
		// 	]
		// }
		// 	*/


	memset(netBuff, 0, sizeof(netBuff));
	sprintf(netBuff, "%c", ESPMES_MQTT_PUBLISH);
	bool timeSet = false;
	uint8_t count = 0;

	sprintf(netBuff, "%s%s", netBuff, "{");

	for (uint16_t sensorIndex=0; sensorIndex<SENSOR_COUNT; sensorIndex++) {

		SensorType wichSensor = sensors.sensorsPriorized(sensorIndex);

		if (sensors[wichSensor].enabled && sensors[wichSensor].id > 0 && (abs(sensors[wichSensor].lastReadingTime - lastSensorUpdate) < (config.readInterval / 2)) && !sensors[wichSensor].reading.startsWith("null")) {
			if (!timeSet) {
				char thisTime[20];
				epoch2iso(sensors[wichSensor].lastReadingTime, thisTime);
				sprintf(netBuff, "%st:%s", netBuff, thisTime);
				timeSet = true;
				sprintf(netBuff, "%s,%u:%g", netBuff, sensors[wichSensor].id, sensors[wichSensor].reading.toFloat());;
			} else {
				sprintf(netBuff, "%s,%u:%g", netBuff, sensors[wichSensor].id, sensors[wichSensor].reading.toFloat());;
			}
			count ++;
		}
	}

	sprintf(netBuff, "%s%s", netBuff, "}");

	sprintf(outBuff, "Publishing %i sensor readings...   ", count);
	sckOut(PRIO_MED);

	bool result = sendMessage();
	return result;
}
bool SckBase::sdPublish()
{
	if (!sdSelect()) return false;

	sprintf(postFile.name, "%02d-%02d-%02d.CSV", rtc.getYear(), rtc.getMonth(), rtc.getDay());
	if (!sd.exists(postFile.name)) writeHeader = true;
	else {
		if (writeHeader) {
			// This means actual enabled/disabled sensors are different from saved ones
			// So we rename original file and start a new one
			char newName[13];
			char prefix[13];
			sprintf(prefix, "%02d-%02d-%02d.", rtc.getYear(), rtc.getMonth(), rtc.getDay());
			bool fileExists = true;
			uint16_t fileNumber = 1;
			while (fileExists) {
				sprintf(newName, "%s%02u", prefix, fileNumber);
				fileNumber++;
				if (!sd.exists(newName)) fileExists = false;
			}
			sd.rename(postFile.name, newName);
		}
	}

	postFile.file = sd.open(postFile.name, FILE_WRITE);

	if (postFile.file) {

		// Write headers
		if (writeHeader) {
			postFile.file.print("TIME");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = sensors.sensorsPriorized(i);
				if (sensors[wichSensor].enabled) {
					postFile.file.print(",");
					postFile.file.print(sensors[wichSensor].shortTitle);
				}
			}
			postFile.file.println("");
			postFile.file.print("ISO 8601");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = sensors.sensorsPriorized(i);
				if (sensors[wichSensor].enabled) {
					postFile.file.print(",");
					if (String(sensors[wichSensor].unit).length() > 0) {
						postFile.file.print(sensors[wichSensor].unit);
					}
				}
			}
			postFile.file.println("");
			postFile.file.print("Time");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = sensors.sensorsPriorized(i);
				if (sensors[wichSensor].enabled) {
					postFile.file.print(",");
					postFile.file.print(sensors[wichSensor].title);
				}
			}
			postFile.file.println("");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = sensors.sensorsPriorized(i);
				if (sensors[wichSensor].enabled) {
					postFile.file.print(",");
					postFile.file.print(sensors[wichSensor].id);
				}
			}
			postFile.file.println("");
			writeHeader = false;
		}

		// Write readings
		bool timeSet = false;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = sensors.sensorsPriorized(i);
			if (sensors[wichSensor].enabled) {
				if (abs(sensors[wichSensor].lastReadingTime - lastSensorUpdate) < (config.readInterval / 2) && !sensors[wichSensor].reading.startsWith("null")) {
					if (!timeSet) {
						epoch2iso(sensors[wichSensor].lastReadingTime, ISOtimeBuff);
						postFile.file.print(ISOtimeBuff);
						timeSet = true;
					}
					postFile.file.print(",");
					postFile.file.print(sensors[wichSensor].reading);
				} else {
					postFile.file.print(",");
					postFile.file.print("null");
				}
			} 
		}
		postFile.file.println("");
		postFile.file.close();
		sckOut("Sd card publish OK!!", PRIO_MED);
		return true;
	} else  {
		st.cardPresent = false;
		st.cardPresentError = false;
	}
	return false;
}
void SckBase::publish()
{
	if (st.mode == MODE_NET) netPublish();
	else if (st.mode == MODE_SD) sdPublish();
	else sckOut("Can't publish without been configured!!");
}

// **** Time
bool SckBase::setTime(String epoch)
{
	uint32_t wasOn = rtc.getEpoch() - espStarted;
	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		st.timeStat.setOk();
		if (urbanPresent) {
			// Update MICS clock
			getReading(&sensors[SENSOR_CO_HEAT_TIME]);
			getReading(&sensors[SENSOR_NO2_HEAT_TIME]);
		}
		espStarted = rtc.getEpoch() - wasOn;
		ISOtime();
		sprintf(outBuff, "RTC updated: %s", ISOtimeBuff);
		sckOut();
		return true;
	} else {
		sckOut("RTC update failed!!");
	}
	return false;
}
bool SckBase::ISOtime()
{
	if (st.timeStat.ok) {
		epoch2iso(rtc.getEpoch(), ISOtimeBuff);
		return true;
	} else {
		sprintf(ISOtimeBuff, "0");
		return false;
	}
}
void SckBase::epoch2iso(uint32_t toConvert, char* isoTime)
{

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


void SckBase::getUniqueID()
{
	volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
	uniqueID[0] = *ptr1;

	volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
	uniqueID[1] = *ptr;
	ptr++;
	uniqueID[2] = *ptr;
	ptr++;
	uniqueID[3] = *ptr;

	sprintf(uniqueID_str,  "%lX%lX%lX%lX", uniqueID[0], uniqueID[1], uniqueID[2], uniqueID[3]);
}
bool I2Cdetect(TwoWire *_Wire, byte address)
{
	_Wire->beginTransmission(address);
	byte error = _Wire->endTransmission();

	if (error == 0) return true;
	else return false;
}

void Status::setOk()
{
	ok = true;
	error = false;
	retrys = 0;
	_lastTryMillis = 0;
}
bool Status::retry()
{
	if (error) return false;
	if (millis() - _lastTryMillis > _timeout || _lastTryMillis == 0) {
		if (retrys == _maxRetrys) {
			error = true;
			return false;
		}
		_lastTryMillis = millis();
		retrys++;
		return true;
	}

	return false;
}
void Status::reset()
{
	ok = false;
	error = false;
	retrys = 0;
	_lastTryMillis = 0;
}
