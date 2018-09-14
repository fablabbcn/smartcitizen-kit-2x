#include "Commands.h"
#include "SckBase.h"


void AllCommands::in(SckBase* base, String strIn)
{

	if (strIn.length() <= 0) return;

	CommandType reqComm = COM_COUNT;

	// Search in the command list
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);

		OneCom *thisCommand = &com_list[thisType];

		if (strIn.startsWith(thisCommand->title)) {
			reqComm = thisType;
			strIn.replace(thisCommand->title, "");
			strIn.trim();
			break;
		}
	}

	if (reqComm < COM_COUNT) com_list[reqComm].function(base, strIn);
	else base->sckOut("Unrecognized command!!");
}

void reset_com(SckBase* base, String parameters)
{

	base->sck_reset();
}
void getVersion_com(SckBase* base, String parameters)
{
	base->getUniqueID();
	sprintf(base->outBuff, "Hardware Version: %s\r\nSAM version: %s\r\nESP version: %s\r\nHardware ID: %lx-%lx-%lx-%lx", hardwareVer.c_str(), SAMversion.c_str(), ESPversion.c_str(), base->uniqueID[0], base->uniqueID[1], base->uniqueID[2], base->uniqueID[3]);
	base->sckOut();
}
void resetCause_com(SckBase* base, String parameters)
{

	uint8_t resetCause = PM->RCAUSE.reg;

	switch(resetCause){
		case 1:
			base->sckOut("POR: Power On Reset");
			break;
		case 2:
			base->sckOut("BOD12: Brown Out 12 Detector Reset");
			break;
		case 4:
			base->sckOut("BOD33: Brown Out 33 Detector Reset");
			break;
		case 16:
			base->sckOut("EXT: External Reset");
			break;
		case 32:
			base->sckOut("WDT: Watchdog Reset");
			break;
		case 64:
			base->sckOut("SYST: System Reset Request");
			break;
	}
}
void outlevel_com(SckBase* base, String parameters)
{

	// get
	if (parameters.length() <= 0) {

		sprintf(base->outBuff, "Current output level: %s", base->outLevelTitles[base->outputLevel]);
		base->sckOut();

		// set
	} else {

		uint8_t newLevel = parameters.toInt();

		// Parameter sanity check
		if (newLevel >= 0 && newLevel < OUT_COUNT) {
			OutLevels thisLevel = static_cast<OutLevels>(newLevel);
			base->outputLevel = thisLevel;
			sprintf(base->outBuff, "New output level: %s", base->outLevelTitles[newLevel]);
			base->sckOut();
		} else {
			base->sckOut("unrecognized output level!!");
			return;
		}
	}
}
void help_com(SckBase* base, String parameters)
{

	// TODO manage multiline help. Maybe a simple general help and a per command help: "help config"
	base->sckOut();
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);
		OneCom *thisCommand = &base->commands[thisType];

		String sep = "            ";
		sep.remove(sep.length() -String(thisCommand->title).length());

		sprintf(base->outBuff, "%s:%s%s", thisCommand->title, sep.c_str(), thisCommand->help);
		base->sckOut();
	}
}
void pinmux_com(SckBase* base, String parameters)
{

	for (uint8_t pin=0; pin<PINS_COUNT; pin++) {  // For all defined pins
		pinmux_report(pin, base->outBuff, 0);
		base->sckOut();
	}
}
void sensorConfig_com(SckBase* base, String parameters)
{
	// Get
	if (parameters.length() <= 0) {

		SensorType thisType = SENSOR_COUNT;

		sprintf(base->outBuff, "\r\nDisabled\r\n----------");
		base->sckOut();
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			thisType = static_cast<SensorType>(i);

			if (!base->sensors[thisType].enabled) base->sckOut(base->sensors[thisType].title);
		}

		sprintf(base->outBuff, "\r\nEnabled\r\n----------");
		base->sckOut();
		// Get sensor type
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			thisType = static_cast<SensorType>(i);

			if (base->sensors[thisType].enabled) base->sckOut(String(base->sensors[thisType].title) + " (" + String(base->sensors[thisType].interval) + " sec)");
		}

	} else {
		uint16_t sensorIndex = parameters.indexOf(" ", parameters.indexOf("-"));
		SensorType sensorToChange = base->sensors.getTypeFromString(parameters.substring(sensorIndex));
		bool saveNeeded = false;

		if (sensorToChange == SENSOR_COUNT) {
			base->sckOut("ERROR sensor not found");
			return;
		} else if (parameters.indexOf("-enable") >=0) {
			if (!base->enableSensor(sensorToChange)) {
				sprintf(base->outBuff, "Failed enabling %s", base->sensors[sensorToChange].title);
				base->sckOut();
			} else saveNeeded = true;
		} else if (parameters.indexOf("-disable") >=0) {
			if (!base->disableSensor(sensorToChange)) {
				sprintf(base->outBuff, "Failed disabling %s", base->sensors[sensorToChange].title);
				base->sckOut();
			} else saveNeeded = true;
		} else if (parameters.indexOf("-interval") >=0) {
			String msg;
			msg = "Changing interval of ";
			sensorIndex = parameters.indexOf(" ", parameters.indexOf("-interval"));
			uint16_t intervalIndex = parameters.indexOf(" ", sensorIndex+1);
			String strInterval = parameters.substring(intervalIndex);
			uint32_t intervalInt = strInterval.toInt();
			if (intervalInt > minimal_sensor_reading_interval && intervalInt < max_sensor_reading_interval)	base->sensors[sensorToChange].interval = strInterval.toInt();
			base->sckOut(msg + String(base->sensors[sensorToChange].title));
			saveNeeded = true;
		}
		if (saveNeeded) base->saveConfig();
	}
}
void readSensor_com(SckBase* base, String parameters)
{
	SensorType sensorToRead = base->sensors.getTypeFromString(parameters);
	if (!base->sensors[sensorToRead].enabled) sprintf(base->outBuff, "%s sensor is disabled!!!", base->sensors[sensorToRead].title);
	else if (base->getReading(sensorToRead, true)) sprintf(base->outBuff, "%s: %s %s", base->sensors[sensorToRead].title, base->sensors[sensorToRead].reading.c_str(), base->sensors[sensorToRead].unit);
	else sprintf(base->outBuff, "ERROR reading %s sensor!!!", base->sensors[sensorToRead].title);
	base->sckOut();
}
void controlSensor_com(SckBase* base, String parameters)
{
  	SensorType wichSensor = base->sensors.getTypeFromString(parameters);

	if (parameters.length() < 1) {
		base->sckOut("ERROR No command received!! please try again...");
		return;
	} else if (wichSensor == SENSOR_COUNT) {
		base->sckOut("ERROR Sensor not found!!!");
		return;
	} else {
		base->controlSensor(wichSensor, base->sensors.removeSensorName(parameters));
	}
}
void monitorSensor_com(SckBase* base, String parameters)
{
	SensorType sensorsToMonitor[SENSOR_COUNT];
	uint8_t index = 0;
	bool sdSave = false;
	bool printTime = true;
	bool printMs = true;

	if (parameters.indexOf("-sd") >=0) {
		sdSave = true;
		parameters.replace("-sd", "");
		parameters.trim();
		if (!base->st.cardPresent) {
			base->sckOut("ERROR No sd card found!!!");
			return;
		}
		base->monitorFile.file = base->sd.open(base->monitorFile.name, FILE_WRITE);
	}
	if (parameters.indexOf("-notime") >=0) {
		printTime = false;
		parameters.replace("-notime", "");
		parameters.trim();
	}
	if (parameters.indexOf("-noms") >=0) {
		printMs = false;
		parameters.replace("-noms", "");
		parameters.trim();
	}
	if (parameters.length() > 0) {
		while (parameters.length() > 0) {
			uint8_t sep = parameters.indexOf(",");
			if (sep == 0) sep = parameters.length();
			String thisSensor = parameters.substring(0, sep);
			parameters.remove(0, sep+1);
			SensorType thisSensorType = base->sensors.getTypeFromString(thisSensor);
			if (base->sensors[thisSensorType].enabled) {
				sensorsToMonitor[index] = thisSensorType;
				index ++;
			} else {
				sprintf(base->outBuff, "%s is disabled, enable it first!!!", base->sensors[thisSensorType].title);
				base->sckOut();
				return;
			}
		}
	} else {
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			if (base->sensors[static_cast<SensorType>(i)].enabled) {
				sensorsToMonitor[index] = static_cast<SensorType>(i);
				index++;
			}
		}

	}

	// Titles
	strncpy(base->outBuff, "", 240);
	if (printTime) sprintf(base->outBuff, "%s\t", "Time");
	if (printMs) sprintf(base->outBuff, "%s%s\t", base->outBuff, "Miliseconds");
	for (uint8_t i=0; i<index; i++) {
		sprintf(base->outBuff, "%s%s", base->outBuff, base->sensors[sensorsToMonitor[i]].title);
		if (i < index - 1) sprintf(base->outBuff, "%s\t", base->outBuff);
	}
	if (sdSave) base->monitorFile.file.println(base->outBuff);
	base->sckOut();

	// Readings
	strncpy(base->outBuff, "", 240);
	uint32_t lastMillis = millis();
	while (!SerialUSB.available()) {
		sprintf(base->outBuff, "%s", "");
		base->ISOtime();
		if (printTime) sprintf(base->outBuff, "%s%s\t", base->outBuff, base->ISOtimeBuff);
		if (printMs) sprintf(base->outBuff, "%s%lu\t", base->outBuff, millis() - lastMillis);
		lastMillis = millis();
		for (uint8_t i=0; i<index; i++) {
			if (base->getReading(sensorsToMonitor[i], true)) sprintf(base->outBuff, "%s%s", base->outBuff, base->sensors[sensorsToMonitor[i]].reading.c_str());
			else sprintf(base->outBuff, "%s%s", base->outBuff, "none");
			if (i < index - 1) sprintf(base->outBuff, "%s\t", base->outBuff);
		}
		if (sdSave) base->monitorFile.file.println(base->outBuff);
		base->sckOut();
	}
	base->monitorFile.file.close();
}
void publish_com(SckBase* base, String parameters)
{
	base->publish();
}
extern "C" char *sbrk(int i);
void freeRAM_com(SckBase* base, String parameters)
{

	char stack_dummy = 0;
	uint32_t free = &stack_dummy - sbrk(0);
	sprintf(base->outBuff, "Free RAM: %lu bytes", free);
	base->sckOut();
}
void battReport_com(SckBase* base, String parameters)
{
	if (parameters.startsWith("-present")) base->battPresent();
	base->batteryReport();
}
void i2cDetect_com(SckBase* base, String parameters)
{

	for (uint8_t wichWire=0; wichWire<2; wichWire++) {

		if (wichWire == 0) base->sckOut("Searching for devices on internal I2C bus...");
		else base->sckOut("\r\nSearching for devices on auxiliary I2C bus...");

		uint8_t nDevices = 0;
		for(uint8_t address = 1; address < 127; address++ ) {

			uint8_t error;
			if (wichWire == 0) {
				Wire.beginTransmission(address);
				error = Wire.endTransmission();
			} else {
				auxWire.beginTransmission(address);
				error = auxWire.endTransmission();
			}

			if (error == 0) {
				sprintf(base->outBuff, "I2C device found at address 0x%02x!!", address);
				base->sckOut();
				nDevices++;
			} else if (error==4) {
				sprintf(base->outBuff, "Unknow error at address 0x%02x!!", address);
				base->sckOut();
			}
		}
		sprintf(base->outBuff, "Found %u devices", nDevices);
		base->sckOut();
	}
}
void getCharger_com(SckBase* base, String parameters)
{

	// sprintf(base->outBuff, "%u", base->charger.chargeTimer(parameters.toInt()));
	// base->sckOut();

	if (parameters.endsWith("otg")) base->charger.OTG(0);

	sprintf(base->outBuff, "Battery: %s", base->charger.chargeStatusTitles[base->charger.getChargeStatus()]);
	base->sckOut();

	sprintf(base->outBuff, "USB: %s", base->charger.VBUSStatusTitles[base->charger.getVBUSstatus()]);
	base->sckOut();

	sprintf(base->outBuff, "OTG: %s", base->charger.enTitles[base->charger.OTG()]);
	base->sckOut();

	sprintf(base->outBuff, "Charge: %s", base->charger.enTitles[base->charger.chargeState()]);
	base->sckOut();

	sprintf(base->outBuff, "Charger current limit: %u mA", base->charger.chargerCurrentLimit());
	base->sckOut();

	sprintf(base->outBuff, "Input current limit: %u mA", base->charger.inputCurrentLimit());
	base->sckOut();

	sprintf(base->outBuff, "I2c watchdog timer: %u sec (0: disabled)", base->charger.I2Cwatchdog());
	base->sckOut();

	sprintf(base->outBuff, "Charging safety timer: %u hours (0: disabled)", base->charger.chargeTimer());
	base->sckOut();
}
void config_com(SckBase* base, String parameters)
{

	// Set
	if (parameters.length() > 0) {

		if (parameters.indexOf("-defaults") >= 0) {
			base->saveConfig(true);
		} else {
			// Shows or sets configuration [-defaults -mode sdcard/network -pubint publish-interval -wifi \"ssid/null\" [\"pass\"] -token token/null]
			uint16_t modeI = parameters.indexOf("-mode");
			if (modeI >= 0) {
				String modeC = parameters.substring(modeI+6);
				modeC.toLowerCase();
				if (modeC.startsWith("sd")) base->config.mode = MODE_SD;
				else if (modeC.startsWith("net")) base->config.mode = MODE_NET;
			}
			uint16_t pubIntI = parameters.indexOf("-pubint");
			if (pubIntI >= 0) {
				String pubIntC = parameters.substring(pubIntI+8);
				uint32_t pubIntV = pubIntC.toInt();
				if (pubIntV > minimal_publish_interval && pubIntV < max_publish_interval) base->config.publishInterval = pubIntV;
			}
			uint16_t readIntI = parameters.indexOf("-readint");
			if (readIntI >= 0) {
				String readIntC = parameters.substring(readIntI+8);
				uint32_t readIntV = readIntC.toInt();
				if (readIntV > minimal_publish_interval && readIntV <= base->config.publishInterval) base->config.readInterval = readIntV;
			}
			uint16_t credI = parameters.indexOf("-wifi");
			if (credI >= 0) {
				uint8_t first = parameters.indexOf("\"", credI+6);
				uint8_t second = parameters.indexOf("\"", first + 1);
				uint8_t third = parameters.indexOf("\"", second + 1);
				uint8_t fourth = parameters.indexOf("\"", third + 1);
				if (parameters.substring(first + 1, second).length() > 0) {
					parameters.substring(first + 1, second).toCharArray(base->config.credentials.ssid, 64);
					base->config.credentials.set = true;
				}
				if (parameters.substring(third + 1, fourth).length() > 0) {
					parameters.substring(third + 1, fourth).toCharArray(base->config.credentials.pass, 64);
				}
			}
			uint16_t tokenI = parameters.indexOf("-token");
			if (tokenI >= 0) {
				String tokenC = parameters.substring(tokenI+7);
				if (tokenC.length() >= 6) {
					tokenC.toCharArray(base->config.token.token, 7);
					base->config.token.set = true;
				}
			}
			base->saveConfig();
		}
		sprintf(base->outBuff, "-- New config --\r\n");

		// Get
	} else base->outBuff[0] = 0;

	Configuration currentConfig = base->getConfig();

	sprintf(base->outBuff, "%sMode: %s\r\nPublish interval: %lu\r\n", base->outBuff, base->modeTitles[currentConfig.mode], currentConfig.publishInterval);
	sprintf(base->outBuff, "%sReading interval: %lu\r\n", base->outBuff, currentConfig.readInterval);

	sprintf(base->outBuff, "%sWifi credentials: ", base->outBuff);
	if (currentConfig.credentials.set) sprintf(base->outBuff, "%s%s - %s\r\n", base->outBuff, currentConfig.credentials.ssid, currentConfig.credentials.pass);
	else sprintf(base->outBuff, "%snot configured\r\n", base->outBuff);

	sprintf(base->outBuff, "%sToken: ", base->outBuff);
	if (currentConfig.token.set) sprintf(base->outBuff, "%s%s", base->outBuff, currentConfig.token.token);
	else sprintf(base->outBuff, "%snot configured", base->outBuff);
	base->sckOut();
}
void esp_com(SckBase* base, String parameters)
{

	if (parameters.equals("-on")) base->ESPcontrol(base->ESP_ON);
	else if (parameters.equals("-off")) base->ESPcontrol(base->ESP_OFF);
	else if (parameters.equals("-reboot")) base->ESPcontrol(base->ESP_REBOOT);
	else if (parameters.equals("-flash")) base->ESPcontrol(base->ESP_FLASH);
	else if (parameters.equals("-sleep")) base->ESPcontrol(base->ESP_SLEEP);
	else if (parameters.equals("-wake")) base->ESPcontrol(base->ESP_WAKEUP);
	else base->sckOut("Unrecognized command , try help!!");
}
void netInfo_com(SckBase* base, String parameters)
{

	base->sendMessage(ESPMES_GET_NETINFO);
}
void time_com(SckBase* base, String parameters)
{

	if (parameters.length() <= 0) {

		if (base->ISOtime()) {
			sprintf(base->outBuff, "Time: %s", base->ISOtimeBuff);
			base->sckOut();
		} else {
			base->sckOut("Time is not synced, trying to sync...");
			base->sendMessage(ESPMES_GET_TIME, "");
		}
	} else if (parameters.toInt() > 0) base->setTime(parameters);
}
void state_com(SckBase* base, String parameters)
{

	base->printState();
}
void hello_com(SckBase* base, String parameters)
{

	base->st.helloPending = true;
	base->sckOut("Waiting for MQTT hello response...");
}
void debug_com(SckBase* base, String parameters)
{
	// Set
	if (parameters.length() > 0) {
		if (parameters.equals("-light")) {
			base->readLight.debugFlag = !base->readLight.debugFlag;
			sprintf(base->outBuff, "ReadLight debugFlag: %s", base->readLight.debugFlag  ? "true" : "false");
			base->sckOut();
		}
		if (parameters.equals("-sdcard")) {
			base->config.sdDebug = !base->config.sdDebug;
			sprintf(base->outBuff, "SD card debug: %s", base->config.sdDebug ? "true" : "false");
			base->sckOut();
			base->saveConfig();
		}
	// Get	
	} else {
		sprintf(base->outBuff, "ReadLight debugFlag: %s", base->readLight.debugFlag  ? "true" : "false");
		sprintf(base->outBuff, "%s\r\nSD card debug: %s", base->outBuff, base->config.sdDebug ? "true" : "false");
		base->sckOut();
	}
}
void shell_com(SckBase* base, String parameters)
{
	if (parameters.length() > 0) {
		if (parameters.equals("-on")) {
			base->st.onShell = true;
			base->st.onSetup = false;
			base->led.update(base->led.YELLOW, base->led.PULSE_STATIC);
		} else if (parameters.equals("-off")) {
			base->st.onShell = false;
		}	
	}
	sprintf(base->outBuff, "Shell mode: %s", base->st.onShell ? "on" : "off");
	base->sckOut();
}

