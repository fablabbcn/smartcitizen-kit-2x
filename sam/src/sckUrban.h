#pragma once

#include <Wire.h>
#include <Arduino.h>

#include "Constants.h"
#include <Sensors.h>



class SckUrban {
public:

	struct Resistor {
		byte deviceAddress;
		byte resistorAddress;
	};

	void setup();

	float getReading(SensorType wichSensor);

	// Noise
	float getNoise();
	void gainChange(uint8_t value);
	float getsound();
	void writeResistorRaw(byte resistor, int value);
	int gain_step = 0;
	float sounddB = 0;
	
	// Temp and humidity
	uint16_t readSHT(uint8_t type);
	float getHumidity();
	float getTemperature();
	
	// Light
	float getLight();

	// Gases
	Resistor POT_CO_LOAD_RESISTOR = {POT3, 0x00};
	Resistor POT_NO2_LOAD_RESISTOR = {POT3, 0x01};
	Resistor POT_CO_HEATER = {POT2, 0x00};
	Resistor POT_NO2_HEATER = {POT2, 0x01};

	const float Rc0 = 10; // Ohm Resistencia en el sensor CO sensor
	const float Rc1 = 39; // Ohm Resistencia en el sensor NO2 sensor

	void GasSetup();
	void GasOFF();
	void GasON();
	float getCO();
	float getNO2();

	// Utility functions
	void setPot(Resistor wichPot, uint32_t value);
	uint32_t getPot(Resistor wichPot);


	void writeI2C(byte deviceaddress, byte address, byte data );
	byte readI2C(int deviceaddress, byte address);
	void ADCini();
	void ADCoff();


	// TO BE PORTED
	void MICSini();
	void currentHeat(byte device, int current);
	float average(int anaPin);
	float readResistor(byte resistor );
	void writeResistor(byte resistor, float value );
	float readRs(byte device);
	float readVH(byte device);
	void writeVH(byte device, long voltage );
	float readMICS(byte device);
	void getMICS(float* __RsCO, float* __RsNO2);
	void getMICS_CO(float* __RsCO);
	void getMICS_NO2(float* __RsNO2);

private:
};

/* MICS notes

	* RED (reducing gases) sensor resistance decreases in the presence of CO and hydrocarbons.
		* The nominal power for the RED sensor is PH = 76 mW
		* The resulting temperature of the sensing layer for RED is about 340 °C (In air at 20 °C)

    * OX (oxidising gases) sensor resistance increases in the presence of NO2
    	* The nominal power for the OX sensor is PH = 43 mW.
		* The resulting temperature of the sensing layer for RED is about 220 °C (In air at 20 °C)

	* Pinout:
		A Rh1 OX 		(NO2 heater S3)
		B Rs1 OX 		(NO2 sensor S1)
		C Rh1 RED 		(CO heater S2)
		D Rs1 RED 		(CO sensor S0)
		E NC 
		F Rh2 RED 		(CO heater) controlado por ADJ_0 (debe ser channel 0 del POT2)
		G Rs2 RED 		V_MICS
		H Rh2 OX 		(NO2 heater) controlado por ADJ_1 (debe ser channel 1 del POT2)
		J Rs2 OX 		V_MICS
		K NC

	* Heaters
		* IO0	Controls CO Sensor Heather (LOW = off)
		* IO1	Controls NO2 Sensor Heater (LOW = off)

	* Load sensor resistors (must be 820 ohms at the lowest in order not to damage the sensitive layer)
		* POT3 channel 0 is CO sensitive load load resitor (S0R)
		* POT3 channel 1 is NO2 sensitive load load resitor (S1R)
	
	* Steps for measuring:
		1. Preheat	(30 sg at higher voltages than operational)
		2. Take reading 

	http://www.sgxsensortech.com/content/uploads/2014/08/AN-0172-SGX-Metal-Oxide-Gas-Sensors-V1.pdf
*/


/*
NOTAS

-- Por I2C tenemos:
	* El potenciometro digital
	* Humidity
	* Temperature
	* UV
	* Light

-- Por pines analogicos
	* CO Sensor
	* NO Sensor
	* CO Current
	* NO Current
	* Sound Sensor

*/

