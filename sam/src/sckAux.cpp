#include "sckAux.h"

void auxSensor::setup(){

}


MCP3424 adc_Slot_1_2(0x68);
MCP3424 adc_Slot_3(0x69);

bool AlphaDelta::begin() {
	
	if (!sht31.begin(0x44)) return false;

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

  // byte rdata = 0xFF;
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