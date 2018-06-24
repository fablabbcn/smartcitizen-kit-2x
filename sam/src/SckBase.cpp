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
	delay(1000); 				// Give some time for external boards to boot

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Power management configuration
	/* battSetup(); */
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
	if (rtc.isConfigured() && (rtc.getEpoch() > 1514764800)) st.timeStat.setOk();	// If greater than 01/01/2018
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

	// Urban board
	analogReadResolution(12);
	if (urban.begin(this)) {
		sckOut("Urban board detected");
		urbanPresent = true;
		readLight.setup();
		// readLight.debugFlag = true;
	} else sckOut("No urban board detected!!");


	// Detect and enable auxiliary boards
	bool saveNeeded = false;
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];

		if (wichSensor->location == BOARD_AUX) {
			if (auxBoards.begin(wichSensor->type)) {
				sprintf(outBuff, "Found: %s... ", wichSensor->title);
				sckOut();
				wichSensor->enabled = true;
				saveNeeded = true;
			} else if (wichSensor->enabled)  {
				sprintf(outBuff, "Removed: %s... ", wichSensor->title);
				sckOut();
				wichSensor->enabled = false;
				saveNeeded = true;
			}
		}
	}

	// TEMP for automatic living lab setup
	// Enables only calibrated alphasense (plus temp and hum) and disable urban board temp and hum
	if (sensors[SENSOR_ALPHADELTA_SLOT_1A].enabled) {

		sensors[SENSOR_ALPHADELTA_SLOT_1A].enabled = false;
                sensors[SENSOR_ALPHADELTA_SLOT_1W].enabled = false;
                sensors[SENSOR_ALPHADELTA_SLOT_2A].enabled = false;
                sensors[SENSOR_ALPHADELTA_SLOT_2W].enabled = false;
                sensors[SENSOR_ALPHADELTA_SLOT_3A].enabled = false;
                sensors[SENSOR_ALPHADELTA_SLOT_3W].enabled = false;

                sensors[SENSOR_TEMPERATURE].enabled = false;
                sensors[SENSOR_HUMIDITY].enabled = false;
                sensors[SENSOR_CO].enabled = false;
                sensors[SENSOR_NO2].enabled = false;
	}

	if (saveNeeded) saveConfig();


#ifdef deltaTest
	ESPcontrol(ESP_OFF);
	led.off();
	led.update(led.BLUE, led.PULSE_STATIC);
	String testResult = auxBoards.control(SENSOR_ALPHADELTA_SLOT_1W, "autotest");
	SerialUSB.println(testResult);
	if (testResult.startsWith("1")) {
		led.update(led.GREEN, led.PULSE_STATIC);
	} else {
		led.update(led.RED, led.PULSE_STATIC);
	}
	while(true);
#endif
}
void SckBase::update()
{

	// TEMP
	if (st.onSetup) {
		lightResults = readLight.read();
		if (lightResults.ok) parseLightRead();
	}

	if (millis() % 500 == 0) reviewState();

	if (butState != butOldState) {
		buttonEvent();
		butOldState = butState;
		while(!butState) buttonStillDown();
	}
}

// **** Mode Control
void SckBase::reviewState()
{

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
	/* parseLightRead() */
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

		// TODO poner retrys al net publish

		if (st.helloPending) {

			if (st.wifiStat.ok) {
				if (st.helloStat.retry()) {
					if (sendMessage(ESPMES_MQTT_HELLO, "")) {
						sckOut("Hello sent!");
					} else {
						sckOut("ERROR sending Hello!!!");
					}
				}
			}

		}
		if (!st.timeStat.ok) {
			if (st.wifiStat.ok) {
				if (!st.helloPending) {
					if (st.timeStat.retry()) {
						if (sendMessage(ESPMES_GET_TIME, "")) {
							sckOut("Asking time to ESP...");
						} else {
							sckOut("ERROR Asking time to ESP...");
						}
					}
				}
			} else {
				if (!st.espON) ESPcontrol(ESP_ON);  // TODO integrate here the wifi retry system
			}
		}
		if (st.timeStat.ok && !st.helloPending) {
			if (rtc.getEpoch() - lastPublishTime > config.publishInterval) {
				if (!st.wifiStat.ok) {
					if (st.wifiStat.retrys == 0) {
						if (!st.espON) ESPcontrol(ESP_ON);
					} else if (st.wifiStat.retry()) {
						ESPcontrol(ESP_REBOOT);
					}
					if (st.wifiStat.error) {
						// TODO turn off ESP, go to sleep and try again later
					}
				} else {
					if (st.publishStat.retry()) {
						if (netPublish()) {
							// TODO go to sleep
						}
					}
				}
			}
		}

		if (st.publishStat.error) {
			sdPublish();
			ESPcontrol(ESP_OFF);
			lastPublishTime = rtc.getEpoch(); 	// TODO save this readings on flash and try after next interval
		}

		// Led feedback
		if (st.wifiStat.error || st.timeStat.error || !st.wifiSet || !st.tokenSet || st.helloStat.error) led.update(led.BLUE, led.PULSE_HARD_FAST);
		else led.update(led.BLUE, led.PULSE_SOFT);

	} else if  (st.mode == MODE_SD) {


		/* MODE_SD (!cardPresent || (!onTime && !wifiSet)) -> onSetup */
		/* MODE_SD (cardPresent && !onTime && wifiSet) -> WAITING_TIME */
		/* WAITING_TIME (!onWifi) -> WAITING_WIFI */
		/* WAITING_WIFI (!espON) -> ESP_ON */
		/* WAITING_WIFI -> (espON && wifiError || timeError) -> onSetup */
		/* MODE_SD (cardPresent && onTime) -> ESP_OFF && updateSensors */


		/* if (!st.cardPresent || (!st.onTime && !st.wifiSet)) enterSetup(); */
		/* else if (!st.onTime) { */
		/* 	sckOut("sdcard NOT ON TIME!!"); */
		/* 	if (!st.onWifi) { */
		/* 		if (!st.espON) ESPcontrol(ESP_ON); */
		/* 		else if (st.wifiError) enterSetup(); */
		/* 	} else { */
		/* 		sckOut("Asking time to ESP..."); */
		/* 		sendMessage(ESPMES_GET_TIME, ""); */
		/* 		if (st.timeError) enterSetup(); */
		/* 	} */
		/* } else if (!st.sleeping) { */
		/* 	ESPcontrol(ESP_OFF); */
		/* 	st.onSetup = false; */
		/* 	led.update(led.PINK, led.PULSE_SOFT); */
		/* } */
	}
}
void SckBase::enterSetup()
{

	sckOut("Entering setup mode", PRIO_LOW);
	st.onSetup = true;

	// Update led
	led.update(led.RED, led.PULSE_SOFT);

	// Start wifi APmode
	if (!st.espON) ESPcontrol(ESP_ON);
	sendMessage(ESPMES_START_AP, "");
	// TODO decide how to manage wifiSet && !tokenSet maybe with station/ap mode at the same time??
	// TODO webserver on ESP

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
	sprintf(outBuff, "%swifiSet: %s\r\n", outBuff, st.wifiSet  ? t : f);
	sprintf(outBuff, "%swifiOK: %s\r\n", outBuff, st.wifiStat.ok ? t : f);
	sprintf(outBuff, "%swifiError: %s\r\n", outBuff, st.wifiStat.error ? t : f);


	sprintf(outBuff, "%s\r\ntimeOK: %s\r\n", outBuff, st.timeStat.ok ? t : f);
	sprintf(outBuff, "%stimeError: %s\r\n", outBuff, st.timeStat.error ? t : f);

	sckOut(PRIO_LOW, false);
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
	strOut.toCharArray(outBuff, strOut.length()+1);
	sckOut(priority, newLine);
}
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine)
{
	strncpy(outBuff, strOut, 240);
	sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine)
{

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
void SckBase::prompt()
{

	sckOut("SCK > ", PRIO_MED, false);
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
		wichSensor->interval = config.sensors[i].interval;
	}

	st.wifiSet = config.credentials.set;
	st.tokenSet = config.token.set;
	st.mode = config.mode;
}
void SckBase::saveConfig(bool defaults)
{
	sckOut("Saving config...", PRIO_LOW);
	if (defaults) {
		Configuration defaultConfig;
		config = defaultConfig;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			config.sensors[i].enabled = sensors[static_cast<SensorType>(i)].defaultEnabled;
			config.sensors[i].interval = default_sensor_reading_interval;
		}
	} else {
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
			config.sensors[i].enabled = wichSensor->enabled;
			config.sensors[i].interval = wichSensor->interval;
		}
	}
	eepromConfig.write(config);

	if (urbanPresent) urban.begin(this);

	st.mode = config.mode;
	st.wifiSet = config.credentials.set;
	st.tokenSet = config.token.set;

	st.wifiStat.reset();

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
	if (!st.espON) {
		ESPcontrol(ESP_ON);
		delay(150);
	}
	sckOut("Saved configuration!!", PRIO_LOW);
	if (sendMessage()) sckOut("Saved configuration on ESP!!", PRIO_LOW);

	st.onSetup = false;
	sendMessage(ESPMES_STOP_AP, "");
}
Configuration SckBase::getConfig()
{

	return config;
}
bool SckBase::parseLightRead()
{
	if (lightResults.lines[0].endsWith(F("wifi")) || lightResults.lines[0].endsWith(F("auth"))) {
		if (lightResults.lines[1].length() > 0) {
			lightResults.lines[1].toCharArray(config.credentials.ssid, 64);
			lightResults.lines[2].toCharArray(config.credentials.pass, 64);
			config.credentials.set = true;
		}
	}

	if (lightResults.lines[0].endsWith(F("auth"))) {
		if (lightResults.lines[3].length() > 0) {
			lightResults.lines[3].toCharArray(config.token.token, 7);
			config.token.set = true;
		}
		uint32_t receivedInterval = lightResults.lines[4].toInt();
		if (receivedInterval > minimal_publish_interval && receivedInterval < max_publish_interval) config.publishInterval = receivedInterval;
	}

	if (lightResults.lines[0].endsWith(F("time"))) setTime(lightResults.lines[1]);

	readLight.reset();
	config.mode = MODE_NET;
	st.helloPending = true;
	led.update(led.GREEN, led.PULSE_STATIC);
	saveConfig();
	return true;
}

// **** ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand)
{
	switch(controlCommand){
		case ESP_OFF:
		{
				sckOut("ESP off...");
				st.espON = false;
				st.wifiStat.ok = false;
				st.wifiStat.error = false;
				digitalWrite(pinESP_CH_PD, LOW);
				digitalWrite(pinPOWER_ESP, HIGH);
				digitalWrite(pinESP_GPIO0, LOW);
				espStarted = 0;
				break;

		}
		case ESP_FLASH:
		{

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
						if (millis() - startTimeout > 8000) reset();		// Giva an initial 5 seconds for the flashing to start
					}
				}
				break;

		}
		case ESP_ON:
		{

				sckOut("ESP on...");
				digitalWrite(pinESP_CH_PD, HIGH);
				digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
				digitalWrite(pinPOWER_ESP, LOW);
				st.espON = true;
				espStarted = rtc.getEpoch();

				break;

		}
		case ESP_REBOOT:
		{
				sckOut("Restarting ESP...");
				ESPcontrol(ESP_OFF);
				delay(10);
				ESPcontrol(ESP_ON);
				break;
		}
	}
}
void SckBase::ESPbusUpdate()
{

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
bool SckBase::sendMessage(ESPMessage wichMessage)
{

	// This function is used when &netBuff[1] is already filled with the content

	sprintf(netBuff, "%u", wichMessage);
	return sendMessage();
}
bool SckBase::sendMessage(ESPMessage wichMessage, const char *content)
{

	sprintf(netBuff, "%u%s", wichMessage, content);
	return sendMessage();
}
bool SckBase::sendMessage()
{

	// This function is used when netbuff is already filled with command and content

	if (!st.espON) {
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
void SckBase::receiveMessage(SAMMessage wichMessage)
{

	switch(wichMessage) {
		case SAMMES_SET_CONFIG:
		{

				sckOut("Received new config from ESP");
				StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(netBuff);
				uint8_t intMode = json["mo"];
				SCKmodes wichMode = static_cast<SCKmodes>(intMode);
				config.mode = wichMode;
				config.publishInterval = json["pi"];
				config.credentials.set = json["cs"];
				strcpy(config.credentials.ssid, json["ss"]);
				strcpy(config.credentials.pass, json["pa"]);
				config.token.set = json["ts"];
				strcpy(config.token.token, json["to"]);

				saveConfig();
				break;

		}
		case SAMMES_DEBUG:
		{

				SerialUSB.print("ESP --> ");
				SerialUSB.print(netBuff);
				break;

		}
		case SAMMES_NETINFO:
		{

				StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(netBuff);
				const char* tip = json["ip"];
				const char* tmac = json["mac"];
				const char* thostname = json["hn"];
				sprintf(outBuff, "\r\nHostname: %s\r\nIP address: %s\r\nMAC address: %s", thostname, tip, tmac);
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
			ESPcontrol(ESP_OFF);
			break;

		default: break;
	}
}

// **** SD card
bool SckBase::sdDetect()
{

	// Wait 100 ms to avoid multiple triggered interrupts
	if (millis() - cardLastChange < 100) return st.cardPresent;

	cardLastChange = millis();
	st.cardPresent = !digitalRead(pinCARD_DETECT);

	if (st.cardPresent) return sdSelect();
	else sckOut("No Sdcard found!!");
	return false;
}
bool SckBase::sdSelect()
{

	if (!st.cardPresent) return false;

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

// **** Flash memory
void SckBase::flashSelect()
{

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
}

// **** Power
void SckBase::battSetup()
{

	pinMode(pinBATTERY_ALARM, INPUT_PULLUP);
	attachInterrupt(pinBATTERY_ALARM, ISR_battery, LOW);

	lipo.enterConfig();
	lipo.setCapacity(battCapacity);
	lipo.setGPOUTPolarity(LOW);
	lipo.setGPOUTFunction(SOC_INT);
	lipo.setSOCIDelta(1);
	lipo.exitConfig();
}
void SckBase::batteryEvent()
{

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
void SckBase::batteryReport()
{

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
void SckBase::reset()
{
	sckOut("Bye!!");
	NVIC_SystemReset();
}
void SckBase::chargerEvent()
{
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
void SckBase::goToSleep()
{

	if (sleepTime > 0) sprintf(outBuff, "Sleeping for %lu seconds", (sleepTime) / 1000);
	else sprintf(outBuff, "Sleeping forever!!! (until a button click)");
	sckOut();

	digitalWrite(pinLED_USB, HIGH);
	led.off();
	ESPcontrol(ESP_OFF);

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

	uint32_t localSleepTime = sleepTime;
	sleepTime = 0;

	if (localSleepTime > 0) LowPower.deepSleep(localSleepTime);
	else LowPower.deepSleep();
}
/* void SckBase::wakeUp() */
/* { */
/* 	sckOut("Waked up!!!"); */
/* } */

// **** Sensors
void SckBase::updateSensors()
{

	/* updateSensors */

}
bool SckBase::getReading(SensorType wichSensor, bool wait)
{

	sensors[wichSensor].valid = false;
	String result = "none";

	switch (sensors[wichSensor].location) {
		case BOARD_BASE:
		{
				switch (wichSensor) {
					case SENSOR_BATT_PERCENT:
					{

							uint32_t thisPercent = lipo.soc();
							if (thisPercent > 100) thisPercent = 0;
							result = String(thisPercent);

							break;
					}
					case SENSOR_BATT_VOLTAGE:

						result = String(lipo.voltage()); break;

					case SENSOR_BATT_CHARGE_RATE:

						result = String(lipo.current(AVG)); break;

					case SENSOR_VOLTIN:
					{

							break;
					}
					default: break;
				}
				break;
		}
		case BOARD_URBAN:
		{
				uint32_t currentTime = 0;
				if (st.timeStat.ok) currentTime = rtc.getEpoch();
				result = urban.getReading(this, wichSensor, wait);
				if (result.startsWith("none")) return false;
				break;
		}
		case BOARD_AUX:
		{
				result = String(auxBoards.getReading(wichSensor), 3);	// TODO port auxBoards to String mode
				break;
		}
	}

	sensors[wichSensor].reading = result;
	sensors[wichSensor].lastReadingTime = rtc.getEpoch();
	sensors[wichSensor].valid = true;
	return true;;
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
}
bool SckBase::netPublish()
{

	if (!st.espON) {
		ESPcontrol(ESP_ON);
		return false;
	}

	// Prepare json for sending
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	// Epoch time of the grouped readings
	json["t"] = rtc.getEpoch();

	JsonArray& jsonSensors = json.createNestedArray("s");

	uint8_t count = 0;

	for (uint16_t sensorIndex=0; sensorIndex<SENSOR_COUNT; sensorIndex++) {

		SensorType wichSensor = static_cast<SensorType>(sensorIndex);

		if (sensors[wichSensor].enabled && sensors[wichSensor].id > 0) {
			// TODO update sensors should manage update readings, remove this when update is ready
			if (getReading(wichSensor, true)) {
				JsonArray& jsonThisSensor = jsonSensors.createNestedArray();
				jsonThisSensor.add(sensors[wichSensor].id);
				jsonThisSensor.add(sensors[wichSensor].reading);
				count ++;
			}
		}
	}

	sprintf(outBuff, "Publishing %i sensor readings...   ", count);
	sckOut(PRIO_MED, false);
	sprintf(netBuff, "%u", ESPMES_MQTT_PUBLISH);
	json.printTo(&netBuff[1], json.measureLength() + 1);
	bool result = sendMessage();

	if (result) {
		lastPublishTime = rtc.getEpoch();
		sdPublish();
	}

	return result;
}
bool SckBase::sdPublish()
{
	if (!sdSelect()) return false;

	sprintf(postFile.name, "%02d-%02d-%02d.CSV", rtc.getYear(), rtc.getMonth(), rtc.getDay());
	if (!sd.exists(postFile.name)) writeHeader = true;

	postFile.file = sd.open(postFile.name, FILE_WRITE);

	if (postFile.file) {

		// Write headers
		if (writeHeader) {
			postFile.file.print("Time,");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = static_cast<SensorType>(i);
				if (sensors[wichSensor].enabled) {
					postFile.file.print(sensors[wichSensor].title);
					if (i < SENSOR_COUNT-1) postFile.file.print(",");
				}
			}
			postFile.file.println("");
			postFile.file.print("ISO 8601,");
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensor = static_cast<SensorType>(i);
				if (sensors[wichSensor].enabled) {
					if (String(sensors[wichSensor].unit).length() > 0) {
						postFile.file.print(sensors[wichSensor].unit);
					}
					if (i < SENSOR_COUNT-1) postFile.file.print(",");
				}
			}
			postFile.file.println("");
			writeHeader = false;
		}

		// Write readings
		ISOtime();
		postFile.file.print(ISOtimeBuff);
		postFile.file.print(",");
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			if (sensors[wichSensor].enabled) {
					// This allows sdcard saving of enabled sensors that don't have a platform ID
					// TODO get readings should be managed from outside net ans sd publish (in update sensors) this will be remove when that is ready
					if (sensors[wichSensor].id == 0) {
						getReading(wichSensor, true);
					}
				postFile.file.print(sensors[wichSensor].reading);
				if (i < SENSOR_COUNT-1) postFile.file.print(",");
			}
		}
		postFile.file.println("");
		postFile.file.close();
		sckOut("Sd card publish OK!!   ", PRIO_MED, false);
		return true;
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
	rtc.setEpoch(epoch.toInt());
	if (abs(rtc.getEpoch() - epoch.toInt()) < 2) {
		st.timeStat.setOk();
		if (urbanPresent) {
			// Update MICS clock
			getReading(SENSOR_CO_HEAT_TIME);
			getReading(SENSOR_NO2_HEAT_TIME);
		}
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
}
bool Status::retry()
{
	if (error) return false;
	if (millis() - _lastTryMillis > _timeout) {
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
	retrys = false;
}
