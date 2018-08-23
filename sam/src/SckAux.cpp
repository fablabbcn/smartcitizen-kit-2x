#include "SckAux.h"

GasesBoard		gasBoard;
GrooveI2C_ADC		grooveI2C_ADC;
INA219			ina219;
Groove_OLED		groove_OLED;
WaterTemp_DS18B20 	waterTemp_DS18B20;
Atlas			atlasPH = Atlas(SENSOR_ATLAS_PH);
Atlas			atlasEC = Atlas(SENSOR_ATLAS_EC);
Atlas			atlasDO = Atlas(SENSOR_ATLAS_DO);
PMsensor		pmSensor = PMsensor(SLOT_AVG);
PM_DallasTemp 		pmDallasTemp;
Sck_SHT31 		sht31 = Sck_SHT31(&auxWire);

// Eeprom flash emulation to store I2C address
// FlashStorage(eepromAuxI2Caddress, Configuration);

bool AuxBoards::start(SensorType wichSensor)
{

	switch (wichSensor) {

		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W:
		case SENSOR_GASESBOARD_SLOT_1_CAL:
		case SENSOR_GASESBOARD_SLOT_2_CAL:
		case SENSOR_GASESBOARD_SLOT_3_CAL:
		case SENSOR_GASESBOARD_HUMIDITY:
		case SENSOR_GASESBOARD_TEMPERATURE: 	return gasBoard.start(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.start(); break;
		case SENSOR_INA219_BUSVOLT:
		case SENSOR_INA219_SHUNT:
		case SENSOR_INA219_CURRENT:
		case SENSOR_INA219_LOADVOLT: 		return ina219.start(); break;
		case SENSOR_GROOVE_OLED: 		return groove_OLED.start(); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.start(); break;
		case SENSOR_ATLAS_PH:			return atlasPH.start();
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG: 		return atlasEC.start(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 		return atlasDO.start(); break;
		case SENSOR_PM_1:
		case SENSOR_PM_25:
		case SENSOR_PM_10:			return pmSensor.start(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.start(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM:
			if (sht31.start() && !gasBoard.start()) return true;
			else return false;
			break;
		default: break;
	}

	return false;
}

bool AuxBoards::stop(SensorType wichSensor)
{
	switch (wichSensor) {

		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W:
		case SENSOR_GASESBOARD_SLOT_1_CAL:
		case SENSOR_GASESBOARD_SLOT_2_CAL:
		case SENSOR_GASESBOARD_SLOT_3_CAL:
		case SENSOR_GASESBOARD_HUMIDITY:
		case SENSOR_GASESBOARD_TEMPERATURE: 	return gasBoard.stop(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.stop(); break;
		case SENSOR_INA219_BUSVOLT:
		case SENSOR_INA219_SHUNT:
		case SENSOR_INA219_CURRENT:
		case SENSOR_INA219_LOADVOLT: 		return ina219.stop(); break;
		case SENSOR_GROOVE_OLED: 		return groove_OLED.stop(); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.stop(); break;
		case SENSOR_ATLAS_PH:			return atlasPH.stop();
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG: 		return atlasEC.stop(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 		return atlasDO.stop(); break;
		case SENSOR_PM_1:
		case SENSOR_PM_25:
		case SENSOR_PM_10:			return pmSensor.stop(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.stop(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM: 			return sht31.stop(); break;
		default: break;
	}

	return false;
}

float AuxBoards::getReading(SensorType wichSensor)
{

	switch (wichSensor) {
		case SENSOR_GASESBOARD_SLOT_1A:	 	return gasBoard.getElectrode(gasBoard.Slot1.electrode_A); break;
		case SENSOR_GASESBOARD_SLOT_1W: 	return gasBoard.getElectrode(gasBoard.Slot1.electrode_W); break;
		case SENSOR_GASESBOARD_SLOT_2A: 	return gasBoard.getElectrode(gasBoard.Slot2.electrode_A); break;
		case SENSOR_GASESBOARD_SLOT_2W: 	return gasBoard.getElectrode(gasBoard.Slot2.electrode_W); break;
		case SENSOR_GASESBOARD_SLOT_3A: 	return gasBoard.getElectrode(gasBoard.Slot3.electrode_A); break;
		case SENSOR_GASESBOARD_SLOT_3W: 	return gasBoard.getElectrode(gasBoard.Slot3.electrode_W); break;
		case SENSOR_GASESBOARD_SLOT_1_CAL: 	return gasBoard.getPPM(gasBoard.Slot1); break;
		case SENSOR_GASESBOARD_SLOT_2_CAL: 	return gasBoard.getPPM(gasBoard.Slot2); break;
		case SENSOR_GASESBOARD_SLOT_3_CAL: 	return gasBoard.getPPM(gasBoard.Slot3); break;
		case SENSOR_GASESBOARD_HUMIDITY: 	return gasBoard.getHumidity(); break;
		case SENSOR_GASESBOARD_TEMPERATURE: return gasBoard.getTemperature(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.getReading(); break;
		case SENSOR_INA219_BUSVOLT: 		return ina219.getReading(ina219.BUS_VOLT); break;
		case SENSOR_INA219_SHUNT: 			return ina219.getReading(ina219.SHUNT_VOLT); break;
		case SENSOR_INA219_CURRENT: 		return ina219.getReading(ina219.CURRENT); break;
		case SENSOR_INA219_LOADVOLT: 		return ina219.getReading(ina219.LOAD_VOLT); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.getReading(); break;
		case SENSOR_ATLAS_PH:			return atlasPH.newReading; break;
		case SENSOR_ATLAS_EC:			return atlasEC.newReading; break;
		case SENSOR_ATLAS_EC_SG:		return atlasEC.newReadingB; break;
		case SENSOR_ATLAS_DO:			return atlasDO.newReading; break;
		case SENSOR_ATLAS_DO_SAT:		return atlasDO.newReadingB; break;
		case SENSOR_PM_1:			return pmSensor.getReading(1); break;
		case SENSOR_PM_25:			return pmSensor.getReading(25); break;
		case SENSOR_PM_10:			return pmSensor.getReading(10); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.getReading(); break;
		case SENSOR_SHT31_TEMP: 		if (sht31.update(true)) return sht31.temperature; break;
		case SENSOR_SHT31_HUM: 			if (sht31.update(true)) return sht31.humidity; break;
		default: break;
	}

	return -9999;
}

bool AuxBoards::getBusyState(SensorType wichSensor)
{

	switch(wichSensor) {
		case SENSOR_GROOVE_OLED:	return true; break;
		case SENSOR_ATLAS_PH: 		return atlasPH.getBusyState(); break;
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG: 	return atlasEC.getBusyState(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 	return atlasDO.getBusyState(); break;
		default: return false; break;
	}
}

String AuxBoards::control(SensorType wichSensor, String command)
{
	switch(wichSensor) {
		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W: {

			if (command.startsWith("set pot")) {

				Electrode wichElectrode;

				switch(wichSensor) {
					case SENSOR_GASESBOARD_SLOT_1A: wichElectrode = gasBoard.Slot1.electrode_A;
					case SENSOR_GASESBOARD_SLOT_1W: wichElectrode = gasBoard.Slot1.electrode_W;
					case SENSOR_GASESBOARD_SLOT_2A: wichElectrode = gasBoard.Slot2.electrode_A;
					case SENSOR_GASESBOARD_SLOT_2W: wichElectrode = gasBoard.Slot2.electrode_W;
					case SENSOR_GASESBOARD_SLOT_3A: wichElectrode = gasBoard.Slot3.electrode_A;
					case SENSOR_GASESBOARD_SLOT_3W: wichElectrode = gasBoard.Slot3.electrode_W;
					default: break;
				}

				command.replace("set pot", "");
				command.trim();
				int wichValue = command.toInt();
				gasBoard.setPot(wichElectrode, wichValue);
				return String F("Setting pot to: ") + String(wichValue) + F(" Ohms\n\rActual value: ") + String(gasBoard.getPot(wichElectrode)) + F(" Ohms");

			#ifdef gasesBoardTest
			} else if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get slot
				String slotSTR = String(command.charAt(0));
				uint8_t wichSlot = slotSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					gasBoard.setTesterCurrent(wichValue, wichSlot);

				} else if (command.startsWith("full")) {

					gasBoard.runTester(wichSlot);

				} else {
					return F("Unrecognized test command!!\r\nOptions:\r\ntest slot set value (slot: 1-3, value:-1400/+1400 nA)\r\ntest slot full (test the full cycle on slot (1-3))");
				}

				return F("\nTesting finished!");

			} else if (command.startsWith("autotest")) {

				return String(gasBoard.autoTest());
			#endif

			} else if (command.startsWith("help")) {
				return F("Available commands for this sensor:\n\r* set pot ");

			} else {
				return F("Unrecognized command!! please try again...");
			}

			break;

		}
		case SENSOR_ATLAS_PH:
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG:
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: {

			Atlas *thisAtlas = &atlasPH;
			if (wichSensor == SENSOR_ATLAS_EC || wichSensor == SENSOR_ATLAS_EC_SG) thisAtlas = &atlasEC;
			else if (wichSensor == SENSOR_ATLAS_DO || wichSensor == SENSOR_ATLAS_DO_SAT) thisAtlas = &atlasDO;

			// 	 Calibration command options:
			// 		Atlas PH: (https://www.atlas-scientific.com/_files/_datasheets/_circuit/pH_EZO_datasheet.pdf) page 50
			// 			* set cal,[mid,low,high] 7.00
			// 			* set cal,clear
			// 		Atlas EC: (https://www.atlas-scientific.com/_files/_datasheets/_circuit/EC_EZO_Datasheet.pdf) page 52
			// 			* set cal,[dry,clear,84]
			// 			* set cal,low,1413
			// 			* set cal,high,12,880
			// 		Atlas DO: (https://www.atlas-scientific.com/_files/_datasheets/_circuit/DO_EZO_Datasheet.pdf) page 50
			// 			* set cal
			// 			* set cal,0
			// 			* set cal,clear
			if (command.startsWith("com")) {

				command.replace("com", "");
				command.trim();
				thisAtlas->sendCommand((char*)command.c_str());

				uint8_t responseCode = thisAtlas->getResponse();
				if (responseCode == 254) delay(1000); responseCode = thisAtlas->getResponse();
				if (responseCode == 1) return thisAtlas->atlasResponse;
				else return String(responseCode);

			}
			break;

		}
		case SENSOR_PM_1:
		case SENSOR_PM_25:
		case SENSOR_PM_10: {
			if (command.startsWith("stop")) {
				
				if (pmSensor.stop()) return ("Stoping PM sensors...");
				else return ("Failed stoping PM sensor!!");

			} else if (command.startsWith("start")) {
			    
				if (pmSensor.start()) return ("Starting PM sensors...");
				else return ("Failed starting PM sensor!!");
			    
			} 
			break;
		} default: return "Unrecognized sensor!!!"; break;
	}
	return "Unknown error on control command!!!";
}

void AuxBoards::print(SensorType wichSensor, String payload)
{

	groove_OLED.print(payload);
}

void AuxBoards::displayReading(String title, String reading, String unit, String time)
{

	groove_OLED.displayReading(title, reading, unit, time);
}

bool GrooveI2C_ADC::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);		// transmit to device
	auxWire.write(REG_ADDR_CONFIG);				// Configuration Register
	auxWire.write(0x20);
	auxWire.endTransmission();
	return true;
}
bool GrooveI2C_ADC::stop()
{

	return true;
}

float GrooveI2C_ADC::getReading()
{

	uint32_t data = 0;

	auxWire.beginTransmission(deviceAddress);		// transmit to device
	auxWire.write(REG_ADDR_RESULT);				// get result
	auxWire.endTransmission();

	auxWire.requestFrom(deviceAddress, 2);			// request 2byte from device
	delay(1);

	if (auxWire.available()<=2) {
		data = (auxWire.read()&0x0f)<<8;
		data |= auxWire.read();
	}

	return data * V_REF * 2.0 / 4096.0;
}

bool INA219::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	ada_ina219.begin();

	// By default the initialization will use the largest range (32V, 2A).  However
	// To use a slightly lower 32V, 1A range (higher precision on amps):
	//ada_ina219.setCalibration_32V_1A();

	// Or to use a lower 16V, 400mA range (higher precision on volts and amps):
	ada_ina219.setCalibration_16V_400mA();
	return true;
}

bool INA219::stop()
{
	// TODO check if there is a way to minimize power consumption
	return true;
}

float INA219::getReading(typeOfReading wichReading)
{

	switch(wichReading) {
		case BUS_VOLT: {

			return ada_ina219.getBusVoltage_V();
			break;

		} case SHUNT_VOLT: {

			return ada_ina219.getShuntVoltage_mV();
			break;

		} case CURRENT: {

			return ada_ina219.getCurrent_mA();
			break;

		} case LOAD_VOLT: {

			float busvoltage 	= ada_ina219.getBusVoltage_V();
			float shuntvoltage 	= ada_ina219.getShuntVoltage_mV();

			return busvoltage + (shuntvoltage / 1000);
			break;

		}
	}

	return 0;
}

bool Groove_OLED::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	U8g2_oled.begin();
	U8g2_oled.clearDisplay();

	U8g2_oled.firstPage();
	do { U8g2_oled.drawXBM( 0, 0, 96, 96, scLogo); } while (U8g2_oled.nextPage());

	return true;;
}

bool Groove_OLED::stop()
{

	return true;
}

void Groove_OLED::print(String payload)
{

	// uint8_t length = payload.length();
	char charPayload[payload.length()];
	payload.toCharArray(charPayload, payload.length()+1);

	U8g2_oled.firstPage();

	do {
		U8g2_oled.setFont(u8g2_font_ncenB14_tr);
		U8g2_oled.drawStr(0,24, charPayload);
	} while (U8g2_oled.nextPage());
}

void Groove_OLED::displayReading(String title, String reading, String unit, String time)
{

	String date;
	String hour;

	if (time.toInt() != 0) {
		date = time.substring(8,10) + "/" + time.substring(5,7) + "/" + time.substring(2,4);
		hour = time.substring(11,16);
	}

	U8g2_oled.firstPage();
	do {

		// Title
		U8g2_oled.setFont(u8g2_font_helvB10_tf);
		if (U8g2_oled.getStrWidth(title.c_str()) > 96 && title.indexOf(" ") > -1) {

			String first = title.substring(0, title.indexOf(" "));
			String second = title.substring(title.indexOf(" ")+1);

			U8g2_oled.drawStr((96-U8g2_oled.getStrWidth(first.c_str()))/2,11, first.c_str());
			U8g2_oled.drawStr((96-U8g2_oled.getStrWidth(second.c_str()))/2,23, second.c_str());

		} else U8g2_oled.drawStr((96-U8g2_oled.getStrWidth(title.c_str()))/2,11, title.c_str());

		// Reading
		U8g2_oled.setFont(u8g2_font_helvB24_tf);
		if (U8g2_oled.getStrWidth(reading.c_str()) > 96) U8g2_oled.setFont(u8g2_font_helvB18_tf);
		U8g2_oled.drawStr((96-U8g2_oled.getStrWidth(reading.c_str()))/2, 55,  reading.c_str());

		// Unit
		U8g2_oled.setFont(u8g2_font_helvB12_tf);
		U8g2_oled.drawStr((96-U8g2_oled.getStrWidth(unit.c_str()))/2,75, unit.c_str());

		if (time.toInt() != 0) {

			// Date
			U8g2_oled.setFont(u8g2_font_helvB10_tf);
			U8g2_oled.drawStr(0,96,date.c_str());

			// Time
			U8g2_oled.drawStr(96-U8g2_oled.getStrWidth(hour.c_str()),96,hour.c_str());
			U8g2_oled.drawStr(96-U8g2_oled.getStrWidth(hour.c_str()),96,hour.c_str());
		}

	} while (U8g2_oled.nextPage());
}

bool WaterTemp_DS18B20::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.begin();

	DS_bridge.reset();
	DS_bridge.wireReset();
	DS_bridge.wireSkip();
	DS_bridge.wireWriteByte(0x44);

	return true;
}

bool WaterTemp_DS18B20::stop()
{
	return true;
}

float WaterTemp_DS18B20::getReading()
{

 	while ( !DS_bridge.wireSearch(addr)) {

		DS_bridge.wireResetSearch();
		DS_bridge.wireReset();
		DS_bridge.selectChannel(0); 			// After reset need to set channel 0 because we are using the version with single channel (DS2482_100)
		DS_bridge.configure(conf);
 		DS_bridge.wireSkip();
 		DS_bridge.configure(conf); 				// Set bus on strong pull-up after next write, not only LSB nibble is required
 		DS_bridge.wireWriteByte(0x44); 			// Convert temperature on all devices
 		DS_bridge.configure(0x01);

	}

	//	Test if device is in reality the DS18B20 Water Temperature
	if (addr[0]==0x28) {

		// Read temperature data.
		DS_bridge.wireReset(); 				//DS_bridge.reset();
		DS_bridge.selectChannel(0); 		//necessary on -800
		DS_bridge.wireSelect(addr);
		DS_bridge.wireWriteByte(0xbe);      // Read Scratchpad command

		// We need to read 9 bytes
		for ( int i = 0; i < 9; i++) data[i] = DS_bridge.wireReadByte();

		// Convert to decimal temperature
		int LowByte = data[0];
		int HighByte = data[1];
		int TReading = (HighByte << 8) + LowByte;
		int SignBit = TReading & 0x8000;

		// If Temperature is negative
		if (SignBit) TReading = (TReading ^ 0xffff) + 1;

		int Tc_100 = (double)TReading * 0.0625 * 10;

		// If the reading is negative make it efective
		if (SignBit) Tc_100 = 0 - Tc_100;

		return ((float)(Tc_100) / 10) + 1;
	}

	return 0;
}

bool Atlas::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	if (beginDone) return true;
	beginDone = true;

	// Protocol lock
	if (!sendCommand((char*)"Plock,1")) return false;
	delay(shortWait);

	// This actions are only for conductivity (EC) sensor
	if (EC) {

		// Set probe
		if (!sendCommand((char*)"K,1.0")) return false;
		delay(shortWait);

		// ----  Set parameters
		if (sendCommand((char*)"O,?")) {
			delay(shortWait);
			if (!atlasResponse.equals("?O,EC,SG")) {
				const char *parameters[4] = PROGMEM {"O,EC,1", "O,TDS,0", "O,S,0", "O,SG,1"};
				for (int i = 0; i<4; ++i) {
					if (!sendCommand((char*)parameters[i])) return false;
					delay(longWait);
				}
			}
		} else return false;

	} else if (DO) {

		// ---- Set parameters
		if (sendCommand((char*)"O,?")) {
			delay(shortWait);
			if (!atlasResponse.equals((char*)"?O,%,mg")) {
				if (!sendCommand((char*)"O,%,1")) return false;
				delay(shortWait);
				if (!sendCommand((char*)"O,mg,1")) return false;
				delay(shortWait);
			}
		} else return false;
	}

	goToSleep();

	return true;
}

bool Atlas::stop()
{
	return true;
}

float Atlas::getReading()
{

	return newReading;
}

bool Atlas::getBusyState()
{

	switch (state) {

		case REST: {
			if (tempCompensation()) state = TEMP_COMP_SENT;
			break;

		} case TEMP_COMP_SENT: {
			if (millis() - lastCommandSent >= shortWait) {
				if (sendCommand((char*)"r")) state = ASKED_READING;
			}
			break;

		} case ASKED_READING: {
			if (millis() - lastCommandSent >= longWait) {

				uint8_t code = getResponse();

				if (code == 254) {
					// Still working (wait a little more)
					lastCommandSent = lastCommandSent + 200;
					break;

				} else if (code == 1) {

					// Reading OK
					state = REST;

					if (PH)	newReading = atlasResponse.toFloat();
					if (EC || DO) {
						String first = atlasResponse.substring(0, atlasResponse.indexOf(","));
						String second = atlasResponse.substring(atlasResponse.indexOf(",")+1);
						newReading = first.toFloat();
						newReadingB = second.toFloat();
					}
					goToSleep();
					return false;
					break;

				} else {

					// Error
					state = REST;
					newReading = 0;
					goToSleep();
					return false;
					break;
				}
			}
			break;
		}
	}

	return true;
}

void Atlas::goToSleep()
{

	auxWire.beginTransmission(deviceAddress);
	auxWire.write("Sleep");
	auxWire.endTransmission();
}

bool Atlas::sendCommand(char* command)
{

	uint8_t retrys = 5;

	for (uint8_t i=0; i<retrys; ++i) {

		auxWire.beginTransmission(deviceAddress);
		auxWire.write(command);

		auxWire.requestFrom(deviceAddress, 1, true);
		uint8_t confirmed = auxWire.read();
		auxWire.endTransmission();

		if (confirmed == 1) {
			lastCommandSent = millis();
			return true;
		}

		delay(300);
	}
	return false;
}

bool Atlas::tempCompensation()
{

	String stringData;
	char data[10];

	float temperature = waterTemp_DS18B20.getReading();

	if (temperature == 0) return false;

	sprintf(data,"T,%.2f",temperature);

	if (sendCommand(data)) return true;

	return false;
}

uint8_t Atlas::getResponse()
{

	uint8_t code;

	auxWire.requestFrom(deviceAddress, 20, 1);
	code = auxWire.read();

	atlasResponse = "";

	switch (code) {
		case 0: 		// Undefined
		case 2:			// Error
		case 255:		// No data sent
		case 254: {		// Still procesing, not ready

			return code;
			break;

		} default : {

			while (auxWire.available()) {
				char buff = auxWire.read();
				atlasResponse += buff;
			}
			auxWire.endTransmission();

			goToSleep();

			if (atlasResponse.length() > 0) {
				return 1;
			}

			return 2;
		}
    }
}

bool PMsensor::start()
{
	if (started) return true;
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(PM_START);
	auxWire.endTransmission();
	started = true;
	return true;
}

bool PMsensor::stop()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(PM_STOP);
	auxWire.endTransmission();
	started = false;
	return true;
}

float PMsensor::getReading(uint8_t wichReading)
{
	// Ask for reading
	auxWire.beginTransmission(deviceAddress);
	switch (slot) {
		case SLOT_A: auxWire.write(GET_PMA); break;
		case SLOT_B: auxWire.write(GET_PMB); break;
		case SLOT_AVG: auxWire.write(GET_PM_AVG); break;
	}
	auxWire.endTransmission();
	delay(2);

	// Get the reading
	auxWire.requestFrom(deviceAddress, 6);
	uint32_t time = millis();
	while (!auxWire.available()) if ((millis() - time)>500) return 0x00;
	for (uint8_t i=0; i<6; i++) {
		values[i] = auxWire.read();
	}

	readingPM1 = (values[0]<<8) + values[1];
	readingPM25 = (values[2]<<8) + values[3];
	readingPM10 = (values[4]<<8) + values[5];

	switch(wichReading) {
		case 1: return readingPM1; break;
		case 25: return readingPM25; break;
		case 10: return readingPM10; break;
	}

	return -9999;
}

bool PM_DallasTemp::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(DALLASTEMP_START);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();
	return result;
}
bool PM_DallasTemp::stop()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(DALLASTEMP_STOP);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();
	return result;
}
float PM_DallasTemp::getReading()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(GET_DALLASTEMP);
	auxWire.endTransmission();

	// Get the reading
	auxWire.requestFrom(deviceAddress, 4);
	uint32_t start = millis();
	while (!auxWire.available()) if ((millis() - start)>500) return -9999;
	for (uint8_t i=0; i<4; i++) uRead.b[i] = auxWire.read();
	return uRead.fval;
}


void writeI2C(byte deviceaddress, byte instruction, byte data )
{
  auxWire.beginTransmission(deviceaddress);
  auxWire.write(instruction);
  auxWire.write(data);
  auxWire.endTransmission();
}

byte readI2C(byte deviceaddress, byte instruction)
{
  byte  data = 0x0000;
  auxWire.beginTransmission(deviceaddress);
  auxWire.write(instruction);
  auxWire.endTransmission();
  auxWire.requestFrom(deviceaddress,1);
  unsigned long time = millis();
  while (!auxWire.available()) if ((millis() - time)>500) return 0x00;
  data = auxWire.read();
  return data;
}
