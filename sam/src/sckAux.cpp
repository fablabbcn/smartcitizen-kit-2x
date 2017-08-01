#include "sckAux.h"

AlphaDelta			alphaDelta;
GrooveI2C_ADC		grooveI2C_ADC;
INA219				ina219;
Groove_OLED			groove_OLED;

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
		case SENSOR_ALPHADELTA_TEMPERATURE: {
			return alphaDelta.begin(); break;
		} case SENSOR_GROOVE_I2C_ADC: 			return grooveI2C_ADC.begin(); break;
		case SENSOR_INA219_BUSVOLT: 		
		case SENSOR_INA219_SHUNT: 			
		case SENSOR_INA219_CURRENT: 		
		case SENSOR_INA219_LOADVOLT: {
			return ina219.begin(); break;
		} case SENSOR_GROOVE_OLED: 				return groove_OLED.begin(); break;
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
		}
	}
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
