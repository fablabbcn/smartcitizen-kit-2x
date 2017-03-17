#pragma once

#define kr				392.1568    //Constante de conversion a resistencia de potenciometrosen ohmios

//TEMP
#define SHT21_DIR       0x40    // SHT21 Temperature and humidity I2C address
#define BH1730_DIR 		0x29	// Light sensor I2C adress

#include "sckBase.h"
#include <Wire.h>
#include <Arduino.h>

class SckUrban {
public:
	void setup();
	//void setup();

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
	float getLight();

	// Utility functions
	void writeI2C(byte deviceaddress, byte address, byte data );
	byte readI2C(int deviceaddress, byte address);
	void ADCini();
	void ADCoff();

};


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