#pragma once

#include <Wire.h>
#include <Arduino.h>

#include "Constants.h"
#include <Sensors.h>


class SckUrban {
public:
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
	float getCO();
	float getNO2();

	// Utility functions
	void writeI2C(byte deviceaddress, byte address, byte data );
	byte readI2C(int deviceaddress, byte address);
	void ADCini();
	void ADCoff();
private:
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