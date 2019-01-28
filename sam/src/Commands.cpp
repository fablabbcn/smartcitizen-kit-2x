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
			if (base->sensors[thisType].enabled) base->sckOut(String(base->sensors[thisType].title) + " (" + String(base->sensors[thisType].everyNint * base->config.readInterval) + " sec)");
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
			} else {
				// Enable extra sensors for PM
				bool extraPrint = false;
				if (sensorToChange == SENSOR_PM_1 || sensorToChange == SENSOR_PM_25 || sensorToChange == SENSOR_PM_10) {
					base->sensors[SENSOR_PM_1].enabled = true; 
					base->sensors[SENSOR_PM_25].enabled = true; 
					base->sensors[SENSOR_PM_10].enabled = true;
					base->sensors[SENSOR_PN_03].enabled = true;
					base->sensors[SENSOR_PN_05].enabled = true;
					base->sensors[SENSOR_PN_1].enabled = true;
					base->sensors[SENSOR_PN_25].enabled = true;
					base->sensors[SENSOR_PN_5].enabled = true;
					base->sensors[SENSOR_PN_10].enabled = true;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_PM_1 || sensorToChange == SENSOR_EXT_PM_25 || sensorToChange == SENSOR_EXT_PM_10) {
					base->sensors[SENSOR_EXT_PM_1].enabled = true; 
					base->sensors[SENSOR_EXT_PM_25].enabled = true; 
					base->sensors[SENSOR_EXT_PM_10].enabled = true;
					base->sensors[SENSOR_EXT_PN_03].enabled = true;
					base->sensors[SENSOR_EXT_PN_05].enabled = true;
					base->sensors[SENSOR_EXT_PN_1].enabled = true;
					base->sensors[SENSOR_EXT_PN_25].enabled = true;
					base->sensors[SENSOR_EXT_PN_5].enabled = true;
					base->sensors[SENSOR_EXT_PN_10].enabled = true;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_A_PM_1 || sensorToChange == SENSOR_EXT_A_PM_25 || sensorToChange == SENSOR_EXT_A_PM_10) {
					base->sensors[SENSOR_EXT_A_PM_1].enabled = true; 
					base->sensors[SENSOR_EXT_A_PM_25].enabled = true; 
					base->sensors[SENSOR_EXT_A_PM_10].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_03].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_05].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_1].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_25].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_5].enabled = true;
					base->sensors[SENSOR_EXT_A_PN_10].enabled = true;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_B_PM_1 || sensorToChange == SENSOR_EXT_B_PM_25 || sensorToChange == SENSOR_EXT_B_PM_10) {
					base->sensors[SENSOR_EXT_B_PM_1].enabled = true; 
					base->sensors[SENSOR_EXT_B_PM_25].enabled = true; 
					base->sensors[SENSOR_EXT_B_PM_10].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_03].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_05].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_1].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_25].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_5].enabled = true;
					base->sensors[SENSOR_EXT_B_PN_10].enabled = true;
					extraPrint = true;
				}
				if (extraPrint) base->sckOut("Also enabled the rest of PM metrics");
				saveNeeded = true;
			}
		} else if (parameters.indexOf("-disable") >=0) {
			if (!base->disableSensor(sensorToChange)) {
				sprintf(base->outBuff, "Failed disabling %s", base->sensors[sensorToChange].title);
				base->sckOut();
			} else {
				// Enable extra sensors for PM
				bool extraPrint = false;
				if (sensorToChange == SENSOR_PM_1 || sensorToChange == SENSOR_PM_25 || sensorToChange == SENSOR_PM_10) {
					base->sensors[SENSOR_PM_1].enabled = false; 
					base->sensors[SENSOR_PM_25].enabled = false; 
					base->sensors[SENSOR_PM_10].enabled = false;
					base->sensors[SENSOR_PN_03].enabled = false;
					base->sensors[SENSOR_PN_05].enabled = false;
					base->sensors[SENSOR_PN_1].enabled = false;
					base->sensors[SENSOR_PN_25].enabled = false;
					base->sensors[SENSOR_PN_5].enabled = false;
					base->sensors[SENSOR_PN_10].enabled = false;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_PM_1 || sensorToChange == SENSOR_EXT_PM_25 || sensorToChange == SENSOR_EXT_PM_10) {
					base->sensors[SENSOR_EXT_PM_1].enabled = false; 
					base->sensors[SENSOR_EXT_PM_25].enabled = false; 
					base->sensors[SENSOR_EXT_PM_10].enabled = false;
					base->sensors[SENSOR_EXT_PN_03].enabled = false;
					base->sensors[SENSOR_EXT_PN_05].enabled = false;
					base->sensors[SENSOR_EXT_PN_1].enabled = false;
					base->sensors[SENSOR_EXT_PN_25].enabled = false;
					base->sensors[SENSOR_EXT_PN_5].enabled = false;
					base->sensors[SENSOR_EXT_PN_10].enabled = false;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_A_PM_1 || sensorToChange == SENSOR_EXT_A_PM_25 || sensorToChange == SENSOR_EXT_A_PM_10) {
					base->sensors[SENSOR_EXT_A_PM_1].enabled = false; 
					base->sensors[SENSOR_EXT_A_PM_25].enabled = false; 
					base->sensors[SENSOR_EXT_A_PM_10].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_03].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_05].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_1].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_25].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_5].enabled = false;
					base->sensors[SENSOR_EXT_A_PN_10].enabled = false;
					extraPrint = true;
				} else if (sensorToChange == SENSOR_EXT_B_PM_1 || sensorToChange == SENSOR_EXT_B_PM_25 || sensorToChange == SENSOR_EXT_B_PM_10) {
					base->sensors[SENSOR_EXT_B_PM_1].enabled = false; 
					base->sensors[SENSOR_EXT_B_PM_25].enabled = false; 
					base->sensors[SENSOR_EXT_B_PM_10].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_03].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_05].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_1].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_25].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_5].enabled = false;
					base->sensors[SENSOR_EXT_B_PN_10].enabled = false;
					extraPrint = true;
				}
				if (extraPrint) base->sckOut("Also disabled the the rest of PM metrics");
				saveNeeded = true;
			}
		} else if (parameters.indexOf("-interval") >=0) {
			String msg;
			msg = "Changing interval of ";
			sensorIndex = parameters.indexOf(" ", parameters.indexOf("-interval"));
			uint16_t intervalIndex = parameters.indexOf(" ", sensorIndex+1);
			String strInterval = parameters.substring(intervalIndex);
			uint32_t intervalInt = strInterval.toInt();
			uint8_t everyNint_pre = intervalInt / base->config.readInterval;
			if (everyNint_pre > 0 && everyNint_pre < 255) {
				base->sensors[sensorToChange].everyNint = everyNint_pre;
				base->sckOut(msg + String(base->sensors[sensorToChange].title));
			} else {
				base->sckOut("Wrong new interval!!!");
			}
		}
		if (saveNeeded) {
			base->saveConfig();
			base->writeHeader = true;
		}
	}
}
void readSensor_com(SckBase* base, String parameters)
{
	SensorType sensorToRead = base->sensors.getTypeFromString(parameters);
	if (!base->sensors[sensorToRead].enabled) sprintf(base->outBuff, "%s sensor is disabled!!!", base->sensors[sensorToRead].title);
	base->getReading(sensorToRead);
	sprintf(base->outBuff, "%s: %s %s", base->sensors[sensorToRead].title, base->sensors[sensorToRead].reading.c_str(), base->sensors[sensorToRead].unit);
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
			base->getReading(sensorsToMonitor[i]);
			sprintf(base->outBuff, "%s%s", base->outBuff, base->sensors[sensorsToMonitor[i]].reading.c_str());
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
void batt_com(SckBase* base, String parameters)
{
	if(!base->battery.isPresent(base->charger)) {
			base->sckOut("No battery present");
			return;
	}

	// get
	if (parameters.length() <= 0) {

			sprintf(base->outBuff, "Charge: %u %%\r\nVoltage: %0.2f V\r\nCharge current: %i mA\r\nPower: %i mW\r\nCapacity: %u/%u mAh\r\nState of health: %u %%",
			base->battery.percent(),
			base->battery.voltage(),
			base->battery.current(),
			base->battery.power(),
			base->battery.remainCapacity(),
			base->battery.designCapacity,
			base->battery.health()
			);
	
			base->sckOut();
	// set
	} else {
	
		uint16_t capI = parameters.indexOf("-cap");
		if (capI >=0) {
			String capC = parameters.substring(capI+5);
			capC.trim();
			uint16_t capU = capC.toInt();

			if (capU > 100 && capU <= 8000) {
				base->sckOut("Reconfiguring battery...");
			
				if (base->config.battDesignCapacity != capU) {
					base->config.battDesignCapacity = capU;
					base->saveConfig();
				}
				batt_com(base, "");
			} else {
				base->sckOut("Wrong or unsuported battery capacity!!");
			}
		}
	
	}
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
void charger_com(SckBase* base, String parameters)
{
	// Get
	if (parameters.length() <= 0) {

		sprintf(base->outBuff, "Battery: %s", base->charger.chargeStatusTitles[base->charger.getChargeStatus()]);
		base->sckOut();

		sprintf(base->outBuff, "USB: %s", base->charger.VBUSStatusTitles[base->charger.getVBUSstatus()]);
		base->sckOut();

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

	// Set
	} else {
	
		uint16_t chargeI = parameters.indexOf("-charge");
		if (chargeI >=0) {
			String chargeC = parameters.substring(chargeI+8);
			if (chargeC.startsWith("on")) base->charger.chargeState(1);
			if (chargeC.startsWith("off")) base->charger.chargeState(0);
		}

		uint16_t otgI = parameters.indexOf("-otg");
		if (otgI >=0) {
			String otgC = parameters.substring(otgI+5);
			if (otgC.startsWith("on")) base->charger.OTG(1);
			if (otgC.startsWith("off")) base->charger.OTG(0);
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
				if (readIntV >= minimal_publish_interval && readIntV <= base->config.publishInterval) base->config.readInterval = readIntV;
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
	if (currentConfig.token.set) sprintf(base->outBuff, "%s%s\r\n", base->outBuff, currentConfig.token.token);
	else sprintf(base->outBuff, "%snot configured\r\n", base->outBuff);

	sprintf(base->outBuff, "%sMac address:  %s", base->outBuff, base->config.mac.address);
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
void state_com(SckBase* base, String parameters)
{

	base->printState();
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
			base->config.sdDebug = !base->config.sdDebug;
			sprintf(base->outBuff, "SD card debug: %s", base->config.sdDebug ? "true" : "false");
			base->sckOut();
			base->saveConfig();
		}
		if (parameters.indexOf("-espcom") >= 0) {
			base->debugESPcom = ! base->debugESPcom;
			sprintf(base->outBuff, "ESP comm debug: %s", base->debugESPcom ? "true" : "false");
			base->sckOut();
		}

	// Get	
	} else {
		sprintf(base->outBuff, "%s\r\nSD card debug: %s", base->outBuff, base->config.sdDebug ? "true" : "false");
		base->sckOut();
		sprintf(base->outBuff, "%s\r\nESP comm debug: %s", base->outBuff, base->debugESPcom ? "true" : "false");
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
	uint8_t mfirst = parameters.indexOf("'", 0);
	uint8_t msecond = parameters.indexOf("'", mfirst + 1);
	uint8_t mthird = parameters.indexOf("'", msecond + 1);
	uint8_t mfourth = parameters.indexOf("'", mthird + 1);
	base->mqttCustom(parameters.substring(mfirst + 1, msecond).c_str(), parameters.substring(mthird + 1, mfourth).c_str());
}
