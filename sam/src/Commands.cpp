#include "Commands.h"
#include "SckBase.h"


void AllCommands::in(SckBase* base, String strIn) {

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

void reset_com(SckBase* base, String parameters) {

	base->reset();
}
void getVersion_com(SckBase* base, String parameters) {
	base->sckOut("Hardware Version: ");
	base->sckOut("SAM Version: ");
	base->sckOut("ESP version: ");

	base->getUniqueID();
	sprintf(base->outBuff, "Hardware ID: %lx-%lx-%lx-%lx", base->uniqueID[0], base->uniqueID[1], base->uniqueID[2], base->uniqueID[3]);
	base->sckOut();
}
void resetCause_com(SckBase* base, String parameters) {

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
void outlevel_com(SckBase* base, String parameters) {

	// get
	if (parameters.length() <= 0) {
	
		sprintf(base->outBuff, "Current output level: %s", base->outLevelTitles[base->outputLevel]);
		base->sckOut();
	
	// set
	} else {
		
		uint8_t newLevel = parameters.toInt();
		
		// Parameter sanity check
		if (newLevel > 0 && newLevel < OUT_COUNT) {
			sprintf(base->outBuff, "New output level: %s", base->outLevelTitles[newLevel]);
			base->sckOut();
		} else {
			base->sckOut("unrecognized output level!!");
			return;
		}
	}
}
void help_com(SckBase* base, String parameters) {

	base->sckOut();
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);
		OneCom *thisCommand = &base->commands[thisType];

		String sep = "            ";
		sep.remove(sep.length() -String(thisCommand->title).length());

		sprintf(base->outBuff, "%s:%s%s", thisCommand->title, sep.c_str(), thisCommand->help);
		base->sckOut();
	}
	base->sckOut();
}
void pinmux_com(SckBase* base, String parameters){

	for (uint8_t pin=0; pin<PINS_COUNT; pin++) {  // For all defined pins
    	pinmux_report(pin, base->outBuff, 0);
    	base->sckOut();	
	}
}
void listSensor_com(SckBase* base, String parameters) {

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
}
void readSensor_com(SckBase* base, String parameters) {

	SensorType sensorToRead = base->sensors.getTypeFromString(parameters);

	if (base->getReading(sensorToRead, true)) sprintf(base->outBuff, "%s: %s %s", base->sensors[sensorToRead].title, base->sensors[sensorToRead].reading.c_str(), base->sensors[sensorToRead].unit);
	else sprintf(base->outBuff, "ERROR reading %s sensor!!!", base->sensors[sensorToRead].title);
	base->sckOut();
}
extern "C" char *sbrk(int i);
void freeRAM_com(SckBase* base, String parameters) {
	
	char stack_dummy = 0;
	uint32_t free = &stack_dummy - sbrk(0);
	sprintf(base->outBuff, "Free RAM: %lu bytes", free);
	base->sckOut();
}
void battReport_com(SckBase* base, String parameters) {

	base->batteryReport();
}
void i2cDetect_com(SckBase* base, String parameters) {

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
void getCharger_com(SckBase* base, String parameters) {

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
void esp_com(SckBase* base, String parameters) {

	if (parameters.length() <= 0) {
		if (base->espStarted > 0) {
			sprintf(base->outBuff, "ESP is on since %lu seconds ago", (millis() - base->espStarted) / 1000);
			base->sckOut();
		} else base->sckOut("ESP is off");
	} else if (parameters.equals("on")) base->ESPcontrol(base->ESP_ON);
	else if (parameters.equals("off")) base->ESPcontrol(base->ESP_OFF);
	else if (parameters.equals("reboot")) base->ESPcontrol(base->ESP_REBOOT);
	else if (parameters.equals("flash")) base->ESPcontrol(base->ESP_FLASH);
	else if (parameters.equals("debug")) {
		// TODO toggle esp debug
	}
}