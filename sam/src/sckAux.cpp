#include "sckAux.h"

AlphaDelta			alphaDelta;
GrooveI2C_ADC		grooveI2C_ADC;
INA219				ina219;
Groove_OLED			groove_OLED;
WaterTemp_DS18B20 	waterTemp_DS18B20;
Atlas				atlasPH = Atlas(SENSOR_ATLAS_PH);
Atlas				atlasEC = Atlas(SENSOR_ATLAS_EC);
Atlas				atlasDO = Atlas(SENSOR_ATLAS_DO);


bool I2Cdetect(byte address) {

	Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
 
	if (error == 0) return true;
	else return false;
}

bool AuxBoards::begin(SensorType wichSensor) {
	
	switch (wichSensor) {

		case SENSOR_ALPHADELTA_AE1:
		case SENSOR_ALPHADELTA_WE1:
		case SENSOR_ALPHADELTA_AE2:
		case SENSOR_ALPHADELTA_WE2:
		case SENSOR_ALPHADELTA_AE3:
		case SENSOR_ALPHADELTA_WE3:
		case SENSOR_ALPHADELTA_HUMIDITY:
		case SENSOR_ALPHADELTA_TEMPERATURE: 	return alphaDelta.begin(); break;
		case SENSOR_GROOVE_I2C_ADC: 			return grooveI2C_ADC.begin(); break;
		case SENSOR_INA219_BUSVOLT: 		
		case SENSOR_INA219_SHUNT: 			
		case SENSOR_INA219_CURRENT: 		
		case SENSOR_INA219_LOADVOLT: 			return ina219.begin(); break;
		case SENSOR_GROOVE_OLED: 				return groove_OLED.begin(); break;
		case SENSOR_WATER_TEMP_DS18B20:			return waterTemp_DS18B20.begin(); break;
		case SENSOR_ATLAS_PH:					return atlasPH.begin();
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG: 				return atlasEC.begin(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 				return atlasDO.begin(); break;
		default: break;
	}

	return false;
}

float AuxBoards::getReading(SensorType wichSensor) {
	
	switch (wichSensor) {
		case SENSOR_ALPHADELTA_AE1: 		return alphaDelta.getElectrode(alphaDelta.AE_1); break;
		case SENSOR_ALPHADELTA_WE1: 		return alphaDelta.getElectrode(alphaDelta.WE_1); break;
		case SENSOR_ALPHADELTA_AE2: 		return alphaDelta.getElectrode(alphaDelta.AE_2); break;
		case SENSOR_ALPHADELTA_WE2: 		return alphaDelta.getElectrode(alphaDelta.WE_2); break;
		case SENSOR_ALPHADELTA_AE3: 		return alphaDelta.getElectrode(alphaDelta.AE_3); break;
		case SENSOR_ALPHADELTA_WE3: 		return alphaDelta.getElectrode(alphaDelta.WE_3); break;
		case SENSOR_ALPHADELTA_HUMIDITY: 	return alphaDelta.getHumidity(); break;
		case SENSOR_ALPHADELTA_TEMPERATURE: return alphaDelta.getTemperature(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.getReading(); break;
		case SENSOR_INA219_BUSVOLT: 		return ina219.getReading(ina219.BUS_VOLT); break;
		case SENSOR_INA219_SHUNT: 			return ina219.getReading(ina219.SHUNT_VOLT); break;
		case SENSOR_INA219_CURRENT: 		return ina219.getReading(ina219.CURRENT); break;
		case SENSOR_INA219_LOADVOLT: 		return ina219.getReading(ina219.LOAD_VOLT); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.getReading(); break;
		case SENSOR_ATLAS_PH:				return atlasPH.newReading; break;
		case SENSOR_ATLAS_EC:				return atlasEC.newReading; break;
		case SENSOR_ATLAS_EC_SG:			return atlasEC.newReadingB; break;
		case SENSOR_ATLAS_DO:				return atlasDO.newReading; break;
		case SENSOR_ATLAS_DO_SAT:			return atlasDO.newReadingB; break;
		default: break;
	}

	return -9999;
}

bool AuxBoards::getBusyState(SensorType wichSensor) {
	
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

String AuxBoards::control(SensorType wichSensor, String command) {
	switch(wichSensor) {
		case SENSOR_ALPHADELTA_AE1:
		case SENSOR_ALPHADELTA_WE1:
		case SENSOR_ALPHADELTA_AE2:
		case SENSOR_ALPHADELTA_WE2:
		case SENSOR_ALPHADELTA_AE3:
		case SENSOR_ALPHADELTA_WE3: {

			if (command.startsWith("set pot")) {

				Resistor wichPot;

				switch(wichSensor) {
					case SENSOR_ALPHADELTA_AE1: wichPot = alphaDelta.POT_AE1;
					case SENSOR_ALPHADELTA_WE1: wichPot = alphaDelta.POT_WE1;
					case SENSOR_ALPHADELTA_AE2: wichPot = alphaDelta.POT_AE2;
					case SENSOR_ALPHADELTA_WE2: wichPot = alphaDelta.POT_WE2;
					case SENSOR_ALPHADELTA_AE3: wichPot = alphaDelta.POT_AE3;
					case SENSOR_ALPHADELTA_WE3: wichPot = alphaDelta.POT_WE3;
					default: break;
				}
				
				command.replace("set pot", "");
				command.trim();
				int wichValue = command.toInt();
				alphaDelta.setPot(wichPot, wichValue);
				return String F("Setting pot to: ") + String(wichValue) + F(" Ohms\n\rActual value: ") + String(alphaDelta.getPot(wichPot)) + F(" Ohms");

			} else if (command.startsWith("help")) {
				return F("Available commands for this sensor:\n\r* set pot ");

			} else {
				return F("Unrecognized command!! please try again...");
			}
			
			break;

		} case SENSOR_ATLAS_PH:
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_SG:
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: {

			Atlas *thisAtlas;

			if (wichSensor == SENSOR_ATLAS_PH) thisAtlas = &atlasPH;
			else if (wichSensor == SENSOR_ATLAS_EC || wichSensor == SENSOR_ATLAS_EC_SG) thisAtlas = &atlasEC;
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

		} default: return "Unrecognized sensor!!!";
	}
	return "Unknown error on control command!!!";
}

void AuxBoards::print(SensorType wichSensor, String payload) {

	groove_OLED.print(payload);
}

void AuxBoards::displayReading(String title, String reading, String unit, String time) {

	groove_OLED.displayReading(title, reading, unit, time);
}

MCP3424 adc_Slot_1_2(0x68);
MCP3424 adc_Slot_3(0x69);

bool AlphaDelta::begin() {
	
	if (!I2Cdetect(sht31Address)) return false;

	sht31.begin();

	// TODO poner un checkeo para saber si respondieron los adc y si no retornar false
	adc_Slot_1_2.generalCall(GC_RESET);
	adc_Slot_3.generalCall(GC_RESET);

	Gain 		myGain			= GAINx8;		// Posible gains: GAINx1, GAINx2, GAINx4, GAINx8
	SampleRate 	mySampleRate 	= SR18B;		// Posible Samplerates: SR12B, SR14B, SR16B, SR18B

	adc_Slot_1_2.creg[CH1].bits = { myGain, mySampleRate, ONE_SHOT, CH1, 1 };
	adc_Slot_1_2.creg[CH2].bits = { myGain, mySampleRate, ONE_SHOT, CH2, 1 };
	adc_Slot_1_2.creg[CH3].bits = { myGain, mySampleRate, ONE_SHOT, CH3, 1 };
	adc_Slot_1_2.creg[CH4].bits = { myGain, mySampleRate, ONE_SHOT, CH4, 1 };

	adc_Slot_3.creg[CH1].bits = { myGain, mySampleRate, ONE_SHOT, CH1, 1 };
	adc_Slot_3.creg[CH2].bits = { myGain, mySampleRate, ONE_SHOT, CH2, 1 };

	return true;
}

float AlphaDelta::getTemperature() {

	return sht31.readTemperature();
}

float AlphaDelta::getHumidity() {

	return sht31.readHumidity();
}

double AlphaDelta::getElectrode(Electrodes wichElectrode) {

	double value;
	bool blocking = true;
	ConvStatus error;

	switch(wichElectrode) {
		case AE_1: error = adc_Slot_1_2.read(CH3, value, blocking); break;
		case WE_1: error = adc_Slot_1_2.read(CH4, value, blocking); break;
		case AE_2: error = adc_Slot_1_2.read(CH1, value, blocking); break;
		case WE_2: error = adc_Slot_1_2.read(CH2, value, blocking); break;
		case AE_3: error = adc_Slot_3.read(CH1, value, blocking); break;
		case WE_3: error = adc_Slot_3.read(CH2, value, blocking); break;
		default: error = R_TIMEOUT;
	}

	if (error != R_OK) return 1;
	else return value;
}

void AlphaDelta::setPot(Resistor wichPot, uint32_t value) {

	// Data to be writen
	int data=0x00;
	if (value>100000) value = 100000;
	data = (int)(value/ohmsPerStep);
	
	Wire.beginTransmission(wichPot.deviceAddress);
	Wire.write(wichPot.resistorAddress);
	Wire.write(data);
	Wire.endTransmission();
	delay(4);
}

uint32_t AlphaDelta::getPot(Resistor wichPot) {

  byte data = 0x0000;

  Wire.beginTransmission(wichPot.deviceAddress);
  Wire.write(wichPot.resistorAddress);
  Wire.endTransmission();
  Wire.requestFrom(wichPot.deviceAddress,1);
  
  // Wait for answer with a timeout
  uint16_t waitTimeout = 500;
  uint32_t time = millis();
  while (!Wire.available()) if ((millis() - time) > waitTimeout) return 0x00;
  data = Wire.read();
  return data*ohmsPerStep;
}

bool GrooveI2C_ADC::begin() {

	if (!I2Cdetect(deviceAddress)) return false;

	Wire.beginTransmission(deviceAddress);		// transmit to device
	Wire.write(REG_ADDR_CONFIG);				// Configuration Register
	Wire.write(0x20);
	Wire.endTransmission();
	return true;
}

float GrooveI2C_ADC::getReading() {

	uint32_t data = 0;

	Wire.beginTransmission(deviceAddress);		// transmit to device
	Wire.write(REG_ADDR_RESULT);				// get result
	Wire.endTransmission();

	Wire.requestFrom(deviceAddress, 2);			// request 2byte from device
	delay(1);

	if (Wire.available()<=2) {
		data = (Wire.read()&0x0f)<<8;
		data |= Wire.read();
	}

	return data * V_REF * 2.0 / 4096.0;
}

bool INA219::begin() {

	if (!I2Cdetect(deviceAddress)) return false;

	ada_ina219.begin();

	// By default the initialization will use the largest range (32V, 2A).  However
	// To use a slightly lower 32V, 1A range (higher precision on amps):
	//ada_ina219.setCalibration_32V_1A();

	// Or to use a lower 16V, 400mA range (higher precision on volts and amps):
	ada_ina219.setCalibration_16V_400mA();
	return true;
}

float INA219::getReading(typeOfReading wichReading) {

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

bool Groove_OLED::begin() {

	if (!I2Cdetect(deviceAddress)) return false;

	U8g2_oled.begin();

	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType thisSensor = static_cast<SensorType>(i);
		displayable[i].sensor = thisSensor;

		switch(thisSensor) {
			case SENSOR_BATTERY:
			case SENSOR_NOISE: {
				displayable[i].display = true;
				break;
				}
			default: displayable[i].display = false;
		}
	}
	return true;;
}

void Groove_OLED::print(String payload) {

	// uint8_t length = payload.length();
	char charPayload[payload.length()];
	payload.toCharArray(charPayload, payload.length()+1);

	U8g2_oled.firstPage();

	do {
		U8g2_oled.setFont(u8g2_font_ncenB14_tr);
		U8g2_oled.drawStr(0,24, charPayload);
	} while (U8g2_oled.nextPage());
}

void Groove_OLED::displayReading(String title, String reading, String unit, String time) {

	// Reading
	uint8_t ll = reading.length();
	char Creading[ll];
	reading.toCharArray(Creading, ll+1);

	// Unit
	ll = unit.length();
	char Cunit[ll];
	unit.toCharArray(Cunit, ll+1);

	// Date
	String date = time.substring(8,10) + "/" + time.substring(5,7) + "/" + time.substring(2,4);
	char Cdate[9];
	date.toCharArray(Cdate, 9);

	SerialUSB.println(Cdate);

	// Time
	String hours = time.substring(11,19);
	char Chour[9];
	hours.toCharArray(Chour, 9);

	// 2017-04-11T15:24:50Z

	U8g2_oled.firstPage();
	do {
		// Reading Left aligned
		U8g2_oled.setFont(u8g2_font_helvB24_tf);
		U8g2_oled.drawStr(0,40, Creading);

		U8g2_oled.setFont(u8g2_font_helvB14_tf);
		U8g2_oled.drawStr(0,62, Cunit);

		// Clock icon
		U8g2_oled.setFont(u8g2_font_unifont_t_symbols);
		U8g2_oled.drawGlyph(80, 70, 0x23f2);

		// Date
		U8g2_oled.setFont(u8g2_font_helvB10_tf);
		U8g2_oled.drawStr(96-U8g2_oled.getStrWidth(Cdate),83,Cdate);

		// Time
		U8g2_oled.drawStr(96-U8g2_oled.getStrWidth(Chour),96,Chour);

	} while (U8g2_oled.nextPage());
}

bool WaterTemp_DS18B20::begin() {

	if (!I2Cdetect(deviceAddress)) return false;

	Wire.begin();

	DS_bridge.reset();
	DS_bridge.wireReset();
	DS_bridge.wireSkip();
	DS_bridge.wireWriteByte(0x44);

	return true;
}

float WaterTemp_DS18B20::getReading() {
	
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

bool Atlas::begin() {

	if (!I2Cdetect(deviceAddress)) return false;

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

float Atlas::getReading() {

	return newReading;
}

bool Atlas::getBusyState() {

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

void Atlas::goToSleep() {

	Wire.beginTransmission(deviceAddress);
	Wire.write("Sleep");
	Wire.endTransmission();
}

bool Atlas::sendCommand(char* command) {

	uint8_t retrys = 5;

	for (uint8_t i=0; i<retrys; ++i) {

		Wire.beginTransmission(deviceAddress);
		Wire.write(command);

		Wire.requestFrom(deviceAddress, 1, true);
		uint8_t confirmed = Wire.read();
		Wire.endTransmission();

		if (confirmed == 1) {
			lastCommandSent = millis();
			return true;
		}

		delay(300);
	}
	return false;
}

bool Atlas::tempCompensation() {

		String stringData;
		char data[10];

		float temperature = waterTemp_DS18B20.getReading();

		if (temperature == 0) return false;

		sprintf(data,"T,%.2f",temperature);

		if (sendCommand(data)) return true;

		return false;
}

uint8_t Atlas::getResponse() {

	uint8_t code;
	
	Wire.requestFrom(deviceAddress, 20, 1);
	code = Wire.read();

	atlasResponse = "";
	
	switch (code) {
		case 0: 		// Undefined
		case 2:			// Error
		case 255:		// No data sent
		case 254: {		// Still procesing, not ready

			return code;
			break;

		} default : {
			
			while (Wire.available()) {
				char buff = Wire.read();
				atlasResponse += buff;
			}
			Wire.endTransmission();
			
			goToSleep();

			if (atlasResponse.length() > 0) {
				return 1;
			}
			
			return 2;
		}
    }
}
