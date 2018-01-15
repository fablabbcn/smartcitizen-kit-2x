#pragma once

#include <Wire.h>
#include <Arduino.h>

#include "Adafruit_SHT31.h"

#include <Sensors.h>

class SckUrban {
private:
	struct Resistor {
		byte deviceAddress;
		byte resistorAddress;
	};

	// Temp and humidity
	// const byte sht31Address = ??;
	Adafruit_SHT31 sht31 = Adafruit_SHT31();
	
	// Carbon Monoxide
	const uint8_t pinPWM_HEATER_CO = 4;		// PA8
	const uint8_t pinREAD_CO = 25;			// PB3

	// Nitrogen Dioxide
	const uint8_t pinPWM_HEATER_NO2 = 3;	// PA9
	const uint8_t pinREAD_NO2 = 19;			// PB2

public:
	bool setup();

	float getTemperature();
	float getHumidity();
};


// I2C internal bus
// PA23 SCL_A
// PA22 SDA_A