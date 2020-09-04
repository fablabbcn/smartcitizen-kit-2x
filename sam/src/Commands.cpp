#include "Commands.h"
#include "SckBase.h"


void AllCommands::in(SckBase* base, String strIn)
{

	if (strIn.length() <= 0) return;
	if (strIn.startsWith("zz")) {
		strIn.replace("zz", "");
		strIn.trim();
		wildCard(base, strIn);
		return;
	}

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
void AllCommands::wildCard(SckBase* base, String strIn)
{

	shell_com(base, "-on");
	base->ESPcontrol(base->ESP_OFF);
}

void reset_com(SckBase* base, String parameters)
{

	base->sck_reset();
}
void getVersion_com(SckBase* base, String parameters)
{
	base->getUniqueID();
	sprintf(base->outBuff, "Hardware Version: %s\r\nSAM Hardware ID: %s\r\nSAM version: %s\r\nSAM build date: %s", base->hardwareVer.c_str(), base->uniqueID_str, base->SAMversion.c_str(), base->SAMbuildDate.c_str());
	base->sckOut();
	sprintf(base->outBuff, "ESP MAC address: %s\r\nESP version: %s\r\nESP build date: %s", base->config.mac.address, base->ESPversion.c_str(), base->ESPbuildDate.c_str());
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

			thisType = base->sensors.sensorsPriorized(i);

			if (!base->sensors[thisType].enabled) base->sckOut(base->sensors[thisType].title);
		}

		sprintf(base->outBuff, "\r\nEnabled\r\n----------");
		base->sckOut();
		// Get sensor type
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {

			thisType = base->sensors.sensorsPriorized(i);
			if (base->sensors[thisType].enabled) {

				snprintf(base->outBuff, sizeof(base->outBuff), "%s (%lu sec)", base->sensors[thisType].title, (base->sensors[thisType].everyNint * base->config.readInterval));
				base->sckOut(PRIO_MED, false);

				if (base->sensors[SENSOR_GROVE_OLED].enabled && base->config.sensors[thisType].oled_display)  base->sckOut(" - oled");
				else base->sckOut();
			}
		}

	} else {

		int16_t sensorEndIndex = parameters.indexOf("-") - 1;
		SensorType sensorToChange = base->sensors.getTypeFromString(parameters.substring(0, sensorEndIndex));
		bool saveNeeded = false;

		// PM and PN sensors are grouped to make changes to the full group
		SensorType urban_pm[] = { SENSOR_PM_1, SENSOR_PM_25, SENSOR_PM_10 };
		SensorType urban_pn[] = { SENSOR_PN_03, SENSOR_PN_05, SENSOR_PN_1, SENSOR_PN_25, SENSOR_PN_5, SENSOR_PN_10 };
		SensorType ext_pm[] = { SENSOR_EXT_PM_1, SENSOR_EXT_PM_25, SENSOR_EXT_PM_10, SENSOR_EXT_A_PM_1, SENSOR_EXT_A_PM_25, SENSOR_EXT_A_PM_10, SENSOR_EXT_B_PM_1, SENSOR_EXT_B_PM_25, SENSOR_EXT_B_PM_10 };
		SensorType ext_pn[] = { SENSOR_EXT_PN_03, SENSOR_EXT_PN_05, SENSOR_EXT_PN_1, SENSOR_EXT_PN_25, SENSOR_EXT_PN_5, SENSOR_EXT_PN_10, SENSOR_EXT_A_PN_03, SENSOR_EXT_A_PN_05, SENSOR_EXT_A_PN_1, SENSOR_EXT_A_PN_25, SENSOR_EXT_A_PN_5, SENSOR_EXT_A_PN_10, SENSOR_EXT_B_PN_03, SENSOR_EXT_B_PN_05, SENSOR_EXT_B_PN_1, SENSOR_EXT_B_PN_25, SENSOR_EXT_B_PN_5, SENSOR_EXT_B_PN_10 };
		SensorType *pm_sensors[] = { urban_pm, urban_pn, ext_pm, ext_pn };
		uint8_t group_size[] = { 3, 6, 9, 18 };

		// Find out if sensor belongs to a PM group
		SensorType *groupToChange = NULL;
		uint8_t groupToChange_size = 0;
		for (uint8_t i=0; i<4; i++) {
			for(uint8_t ii=0; ii<group_size[i]; ii++) {
				if(pm_sensors[i][ii] == sensorToChange) {
					groupToChange = pm_sensors[i];
					groupToChange_size = group_size[i];
				}
			}
		}

		if (sensorToChange == SENSOR_COUNT) {
			base->sckOut("ERROR sensor not found");
			return;
		}

		if (parameters.indexOf("-enable") >=0) {
			if (!base->enableSensor(sensorToChange)) {
				sprintf(base->outBuff, "Failed enabling %s", base->sensors[sensorToChange].title);
				base->sckOut();
			} else {
				// Enable sensor also in config to make changes persistent
				base->config.sensors[sensorToChange].enabled = true;

				// Just for PM/PN enable the rest of sensors in the same group
				for (uint8_t i=0; i<groupToChange_size; i++) {
					// Enable them in runtime
					base->sensors[groupToChange[i]].enabled = true;

					// Enable them also in config to make changes persistent
					base->config.sensors[groupToChange[i]].enabled = true;

					sprintf(base->outBuff, "Enabling %s", base->sensors[groupToChange[i]].title);
					base->sckOut();
				}
				saveNeeded = true;
			}
		} else if (parameters.indexOf("-disable") >=0) {
			if (!base->disableSensor(sensorToChange)) {
				sprintf(base->outBuff, "Failed disabling %s", base->sensors[sensorToChange].title);
				base->sckOut();
			} else {
				// Disable sensor also in config to make changes persistent
				base->config.sensors[sensorToChange].enabled = false;

				// Just for PM/PN disable the rest of sensors in the same group
				for (uint8_t i=0; i<groupToChange_size; i++) {
					// Disable them in runtime
					base->sensors[groupToChange[i]].enabled = false;

					// Disable them also in config to make changes persistent
					base->config.sensors[groupToChange[i]].enabled = false;

					sprintf(base->outBuff, "Disabling %s", base->sensors[groupToChange[i]].title);
					base->sckOut();
				}
				saveNeeded = true;
			}
		}

		if (parameters.indexOf("-oled") >=0) {

			base->config.sensors[sensorToChange].oled_display = !base->config.sensors[sensorToChange].oled_display;

			snprintf(base->outBuff, sizeof(base->outBuff), "%s will %s on oled display", base->sensors[sensorToChange].title, base->config.sensors[sensorToChange].oled_display ? "be displayed" : "not show");
			base->sckOut();

			saveNeeded = true;

		}

		if (parameters.indexOf("-interval") >=0) {

			// Get the number of seconds user asked for as new interval
			int16_t intervalIndex = parameters.indexOf("-interval") + 10;
			String strInterval = parameters.substring(intervalIndex);
			uint32_t intervalInt = strInterval.toInt();

			// Calculate how many general intervals between sensor readings
			uint8_t newEveryNint = intervalInt / base->config.readInterval;
			if (newEveryNint < 1) newEveryNint = 1;

			base->sckOut("The sensor read interval is calculated as a multiple of general read interval (" + String(base->config.readInterval) + ")");
			if (newEveryNint < 255) {
				if (groupToChange_size > 0) {
					// Just for PM/PN change all the sensors in the same group
					for (uint8_t i=0; i<groupToChange_size; i++) {
						base->sensors[groupToChange[i]].everyNint = newEveryNint;
						base->config.sensors[groupToChange[i]].everyNint = newEveryNint;
						sprintf(base->outBuff, "Changing interval of %s to %lu", base->sensors[groupToChange[i]].title, base->sensors[groupToChange[i]].everyNint * base->config.readInterval);
						base->sckOut();
					}
				} else {
					base->sensors[sensorToChange].everyNint = newEveryNint;
					base->config.sensors[sensorToChange].everyNint = newEveryNint;
					sprintf(base->outBuff, "Changing interval of %s to %lu", base->sensors[sensorToChange].title, base->sensors[sensorToChange].everyNint * base->config.readInterval);
					base->sckOut();
				}
				saveNeeded = true;
			} else {
				base->sckOut("Wrong new interval!!!");
			}
		}
		if (saveNeeded) base->saveConfig();
	}
}
void readSensor_com(SckBase* base, String parameters)
{
	SensorType wichType = base->sensors.getTypeFromString(parameters);
	OneSensor sensorToRead = base->sensors[wichType];

	if (!sensorToRead.enabled) {
		sprintf(base->outBuff, "%s sensor is disabled!!!", sensorToRead.title);
		base->sckOut();
		return;
	} else base->getReading(&sensorToRead);

	if (sensorToRead.state == 0) sprintf(base->outBuff, "%s: %s %s", sensorToRead.title, sensorToRead.reading.c_str(), sensorToRead.unit);
	else if (sensorToRead.state == -1) sprintf(base->outBuff, "ERROR reading %s sensor!!!", sensorToRead.title);
	else sprintf(base->outBuff, "Your reading will be ready in %i seconds try again!!", sensorToRead.state);
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
	bool oled = false;

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
	if (parameters.indexOf("-oled") >=0) {
		oled = true;
		parameters.replace("-oled", "");
		parameters.trim();
	}
	if (parameters.length() > 0) {
		while (parameters.length() > 0) {

			int16_t sep = parameters.indexOf(",");
			String thisSensor;

			if (sep <= 0) {
				thisSensor = parameters;
				parameters.remove(0, parameters.length());
			} else {
				thisSensor = parameters.substring(0, sep);
				parameters.remove(0, sep+1);
				parameters.trim();
			}

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
	if (printMs) snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s%s\t", base->outBuff, "Miliseconds");
	for (uint8_t i=0; i<index; i++) {
		sprintf(base->outBuff, "%s%s", base->outBuff, base->sensors[sensorsToMonitor[i]].title);
		if (i < index - 1) snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s\t", base->outBuff);
		if (oled && i==0) base->plot(base->sensors[sensorsToMonitor[i]].reading, base->sensors[sensorsToMonitor[i]].title, base->sensors[sensorsToMonitor[i]].unit);
	}
	if (sdSave) base->monitorFile.file.println(base->outBuff);
	base->sckOut();

	// Readings
	strncpy(base->outBuff, "", 240);
	uint32_t lastMillis = millis();
	while (!SerialUSB.available()) {

		sprintf(base->outBuff, "%s", "");

		if (printTime) {
			base->ISOtime();
			snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s%s\t", base->outBuff, base->ISOtimeBuff);
		}

		if (printMs) {
			snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s%lu\t", base->outBuff, millis() - lastMillis);
			lastMillis = millis();
		}

		bool theFirst = true;
		for (uint8_t i=0; i<index; i++) {
			// TODO check what will happen here when one shot PM is implemented
			OneSensor wichSensor = base->sensors[sensorsToMonitor[i]];
			base->getReading(&wichSensor);

			if (wichSensor.state == 0) {

				sprintf(base->outBuff, "%s%s", base->outBuff, wichSensor.reading.c_str());

				if (theFirst && oled) {
					base->plot(wichSensor.reading);
					theFirst = false;
				}
			} else snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s%s", base->outBuff, "none");

			if (i < index - 1) snprintf(base->outBuff, sizeof(base->outBuff) - strlen(base->outBuff), "%s\t", base->outBuff);
		}

		if (sdSave) base->monitorFile.file.println(base->outBuff);

		base->sckOut();
	}
	base->monitorFile.file.close();
}
void readings_com(SckBase* base, String parameters)
{
	bool details = false;
	if (parameters.indexOf("-details") >=0) {
		details = true;
		parameters.replace("-details", "");
		parameters.trim();
	}

	uint32_t savedGroups = base->readingsList.countGroups();

	if (savedGroups <= 0) {
		base->sckOut("No readings stored on memory");
		return;
	}

	for (uint32_t thisGroup=0; thisGroup<savedGroups; thisGroup++) {

		uint16_t readingsOnThisGroup = base->readingsList.countReadings(thisGroup);
		char thisTime[25];
		base->epoch2iso(base->readingsList.getTime(thisGroup), thisTime);
		base->sckOut("-----------");
		sprintf(base->outBuff, "%lu - %s - %i readings on Memory.", thisGroup+1, thisTime, readingsOnThisGroup);
		base->sckOut();
		if (details) {
			sprintf(base->outBuff, "Published to the platform: %s\r\nPublished to sdcard: %s", base->readingsList.getFlag(thisGroup, base->readingsList.NET_PUBLISHED) ? "true" : "false", base->readingsList.getFlag(thisGroup, base->readingsList.SD_PUBLISHED) ? "true" : "false");
			base->sckOut();
			for (uint16_t re=0; re<readingsOnThisGroup; re++) {
				OneReading thisReading = base->readingsList.readReading(thisGroup, re);
				sprintf(base->outBuff, "%s: %s %s", base->sensors[thisReading.type].title, thisReading.value.c_str(), base->sensors[thisReading.type].unit);
				base->sckOut();
			}
		}
		base->sckOut("-----------");
	}
	base->sckOut(" ");

	// TODO code for -publish option
	/* base->publish(); */
}
extern "C" char *sbrk(int i);
void freeRAM_com(SckBase* base, String parameters)
{

	char stack_dummy = 0;
	uint32_t free = &stack_dummy - sbrk(0);
	sprintf(base->outBuff, "Free RAM: %lu bytes", free);
	base->sckOut();
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
void power_com(SckBase* base, String parameters)
{
	// Get

	int16_t extraI = parameters.indexOf("-info");
	if (parameters.length() <= 0 || extraI >= 0) {

		if (base->config.sleepTimer == 0) sprintf (base->outBuff, "Sleep disabled");
		else sprintf(base->outBuff, "Sleep after: %u min", base->config.sleepTimer);
		base->sckOut();

		if (base->battery.present) {
			sprintf(base->outBuff, "Battery: %u%% (%lumAh) - %0.2fv (%s)", base->battery.percent(&base->charger), base->config.battConf.battCapacity, base->battery.voltage(), base->charger.chargeStatusTitles[base->charger.getChargeStatus()]);
		} else {
			sprintf(base->outBuff, "Battery: NOT present");
		}
		base->sckOut();

		sprintf(base->outBuff, "USB: %s", base->charger.VBUSStatusTitles[base->charger.getVBUSstatus()]);
		base->sckOut();

		if (extraI >= 0) {

			sprintf(base->outBuff, "OTG: %s", base->charger.enTitles[base->charger.OTG()]);
			base->sckOut();

			sprintf(base->outBuff, "Charge: %s", base->charger.enTitles[base->charger.chargeState()]);
			base->sckOut();

			sprintf(base->outBuff, "Batfet: %s", base->charger.enTitles[base->charger.batfetState()]);
			base->sckOut();

			sprintf(base->outBuff, "Charger current limit: %u mA", base->charger.chargerCurrentLimit());
			base->sckOut();

			sprintf(base->outBuff, "Input current limit: %u mA", base->charger.inputCurrentLimit());
			base->sckOut();

			sprintf(base->outBuff, "I2c watchdog timer: %u sec (0: disabled)", base->charger.I2Cwatchdog());
			base->sckOut();

			sprintf(base->outBuff, "Charging safety timer: %u hours (0: disabled)", base->charger.chargeTimer());
			base->sckOut();

			sprintf(base->outBuff, "Min system voltage: %0.2f volts", base->charger.sysMinVolt());
			base->sckOut();

			sprintf(base->outBuff, "Battery lower than %0.2f: %s", base->charger.sysMinVolt(), base->charger.getBatLowerSysMin() ? "true" : "false");
			base->sckOut();

		}


	// Set
	} else {

		int16_t chargeI = parameters.indexOf("-charge");
		if (chargeI >=0) {
			String chargeC = parameters.substring(chargeI+8);
			if (chargeC.startsWith("on")) base->charger.chargeState(1);
			if (chargeC.startsWith("off")) base->charger.chargeState(0);
		}

		int16_t otgI = parameters.indexOf("-otg");
		if (otgI >=0) {
			String otgC = parameters.substring(otgI+5);
			if (otgC.startsWith("on")) base->charger.OTG(1);
			if (otgC.startsWith("off")) base->charger.OTG(0);
		}

		int16_t batcapI = parameters.indexOf("-batcap");
		if ( batcapI>=0) {
			String batcapC = parameters.substring(batcapI+8);
			batcapC.trim();
			int32_t batcapInt = batcapC.toInt();
			if (batcapInt > 200 && batcapInt < 20000) {
				base->config.battConf.battCapacity = batcapInt;
				base->saveConfig();
				sprintf(base->outBuff, "New battery capacity: %lu\r\nWe need to reset for changes to take effect\r\nClick any key.", base->config.battConf.battCapacity);
				base->sckOut();
				while (!SerialUSB.available())
					;
				base->sck_reset();
			} else {
				sprintf(base->outBuff, "Wrong battery capacity");
			}
			base->sckOut();
		}

		int16_t sleepI = parameters.indexOf("-sleep");
		if (sleepI>=0) {
			String sleepC = parameters.substring(sleepI+7);
			sleepC.trim();
			int32_t sleepInt = sleepC.toInt();
			if (sleepInt >= 0 && sleepInt < 480) { 	// Max 8 hours
				base->config.sleepTimer = sleepInt;
				base->saveConfig();
				base->lastUserEvent = millis();
				sprintf(base->outBuff, "New sleep timer period: %u.", base->config.sleepTimer);
			} else {
				sprintf(base->outBuff, "Wrong sleep timer period (0-480)");
			}
			base->sckOut();
		}

	}
}
void config_com(SckBase* base, String parameters)
{

	// Set
	if (parameters.length() > 0) {

		if (parameters.indexOf("-defaults") >= 0) {
			base->saveConfig(true);
		} else {
			// Shows or sets configuration [-defaults -mode sdcard/network -pubint publish-interval -wifi \"ssid/null\" [\"pass\"] -token token/null]
			int16_t modeI = parameters.indexOf("-mode");
			if (modeI >= 0) {
				String modeC = parameters.substring(modeI+6);
				modeC.toLowerCase();
				if (modeC.startsWith("sd")) base->config.mode = MODE_SD;
				else if (modeC.startsWith("net")) base->config.mode = MODE_NET;
			}
			int16_t pubIntI = parameters.indexOf("-pubint");
			if (pubIntI >= 0) {
				String pubIntC = parameters.substring(pubIntI+8);
				uint32_t pubIntV = pubIntC.toInt();
				// TODO remove this when flash storage is ready
				if (pubIntV > 180) {
					base->sckOut("For now we can only store safely 3 set of readings on RAM memory, sorry.");
				} else if (pubIntV >= minimal_publish_interval && pubIntV <= max_publish_interval) base->config.publishInterval = pubIntV;
			}
			int16_t readIntI = parameters.indexOf("-readint");
			if (readIntI >= 0) {
				String readIntC = parameters.substring(readIntI+8);
				uint32_t readIntV = readIntC.toInt();
				if (readIntV >= minimal_publish_interval && readIntV <= base->config.publishInterval) base->config.readInterval = readIntV;
			}
			int16_t credI = parameters.indexOf("-wifi");
			if (credI >= 0) {
				int16_t first = parameters.indexOf("\"", credI+6);
				int16_t second = parameters.indexOf("\"", first + 1);
				int16_t third = parameters.indexOf("\"", second + 1);
				int16_t fourth = parameters.indexOf("\"", third + 1);
				if (parameters.substring(first + 1, second).length() > 0) {
					parameters.substring(first + 1, second).toCharArray(base->config.credentials.ssid, 64);
					base->config.credentials.set = true;
				}
				if (parameters.substring(third + 1, fourth).length() > 0) {
					parameters.substring(third + 1, fourth).toCharArray(base->config.credentials.pass, 64);
				} else {
					strncpy(base->config.credentials.pass, "", 64);
				}
			}
			int16_t tokenI = parameters.indexOf("-token");
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
		base->sckOut();

		// Get
	} else base->outBuff[0] = 0;

	Configuration currentConfig = base->getConfig();

	sprintf(base->outBuff, "Mode: %s\r\nPublish interval: %lu", base->modeTitles[currentConfig.mode], currentConfig.publishInterval);
	base->sckOut();

	sprintf(base->outBuff, "Reading interval: %lu", currentConfig.readInterval);
	base->sckOut();

	if (currentConfig.credentials.set) sprintf(base->outBuff, "Wifi credentials: %s - %s", currentConfig.credentials.ssid, currentConfig.credentials.pass);
	else sprintf(base->outBuff, "Wifi credentials: not configured");
	base->sckOut();

	if (currentConfig.token.set) sprintf(base->outBuff, "Token: %s", currentConfig.token.token);
	else sprintf(base->outBuff, "Token not configuredn");
	base->sckOut();

	sprintf(base->outBuff, "Mac address:  %s", base->config.mac.address);
	base->sckOut();
}
void esp_com(SckBase* base, String parameters)
{

	if (parameters.equals("-on")) base->ESPcontrol(base->ESP_ON);
	else if (parameters.equals("-off")) base->ESPcontrol(base->ESP_OFF);
	else if (parameters.equals("-reboot")) base->ESPcontrol(base->ESP_REBOOT);
	else if (parameters.startsWith("-flash")) {
		parameters.replace("-flash ", "");
		uint32_t speed = parameters.toInt();
		if (speed >= 115200 && speed <= 921600) base->espFlashSpeed = speed;
		base->ESPcontrol(base->ESP_FLASH);
	} else if (parameters.equals("-sleep")) base->ESPcontrol(base->ESP_SLEEP);
	else if (parameters.equals("-wake")) base->ESPcontrol(base->ESP_WAKEUP);
	else base->sckOut("Unrecognized command , try help!!");
}
void netInfo_com(SckBase* base, String parameters)
{

	base->sendMessage(ESPMES_GET_NETINFO);
}
void time_com(SckBase* base, String parameters)
{

	base->epoch2iso(base->rtc.getEpoch(), base->ISOtimeBuff);
	base->sckOut();

	if (parameters.length() <= 0) {

		if (base->ISOtime()) {
			sprintf(base->outBuff, "Time: %s", base->ISOtimeBuff);
			base->sckOut();
		} else {
			base->sckOut("Time is not synced, trying to sync...");
			base->sendMessage(ESPMES_GET_TIME, "");
		}
	}

	// Force sync
	if (parameters.equals("-sync")) {
		if (!base->st.espON) {
			base->ESPcontrol(base->ESP_ON);
			delay(200);
		}
		if (base->sendMessage(ESPMES_GET_TIME, "")) base->sckOut("Asking time to ESP...");
	}

	// Receive Epoch time and sync
	if (parameters.toInt() > 0) base->setTime(parameters);
}
void hello_com(SckBase* base, String parameters)
{

	if (base->sendMessage(ESPMES_MQTT_HELLO, "")) base->sckOut("Hello sent!");
	base->sckOut("Waiting for MQTT hello response...");
}
void debug_com(SckBase* base, String parameters)
{
	// Set
	if (parameters.length() > 0) {
		if (parameters.indexOf("-sdcard") >= 0) {
			base->config.debug.sdcard = !base->config.debug.sdcard;
			sprintf(base->outBuff, "SD card debug: %s", base->config.debug.sdcard ? "true" : "false");
			base->sckOut();
			base->saveConfig();
		}
		if (parameters.indexOf("-esp") >= 0) {
			base->config.debug.esp = !base->config.debug.esp;
			sprintf(base->outBuff, "ESP comm debug: %s", base->config.debug.esp ? "true" : "false");
			base->sckOut();
		}
		if (parameters.indexOf("-oled") >= 0) {
			base->config.debug.oled = !base->config.debug.oled;
			sprintf(base->outBuff, "Oled display debug: %s", base->config.debug.oled ? "true" : "false");
			base->sckOut();
		}
		if (parameters.indexOf("-flash") >= 0) {
			base->config.debug.flash = !base->config.debug.flash;
			sprintf(base->outBuff, "Flash memory debug: %s", base->config.debug.flash ? "true" : "false");
			base->sckOut();
		}
	// Get
	} else {
		sprintf(base->outBuff, "SD card debug: %s", base->config.debug.sdcard ? "true" : "false");
		base->sckOut();
		sprintf(base->outBuff, "ESP comm debug: %s", base->config.debug.esp ? "true" : "false");
		base->sckOut();
		sprintf(base->outBuff, "Oled display debug: %s", base->config.debug.oled ? "true" : "false");
		base->sckOut();
		sprintf(base->outBuff, "Flash memory debug: %s", base->config.debug.flash ? "true" : "false");
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
void custom_mqtt_com(SckBase* base, String parameters)
{
	int16_t mfirst = parameters.indexOf("\"", 0);
	int16_t msecond = parameters.indexOf("\"", mfirst + 1);
	int16_t mthird = parameters.indexOf("\"", msecond + 1);
	int16_t mfourth = parameters.indexOf("\"", mthird + 1);
	base->mqttCustom(parameters.substring(mfirst + 1, msecond).c_str(), parameters.substring(mthird + 1, mfourth).c_str());
}
