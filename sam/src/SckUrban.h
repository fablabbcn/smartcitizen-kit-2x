#pragma once

#include <Wire.h>
#include <Arduino.h>

#include "Adafruit_SHT31.h"

#include <Sensors.h>
#include "Pins.h"

class SckUrban {
private:
	struct Resistor {
		byte deviceAddress;
		byte resistorAddress;
	};

	// Temp and humidity
	// const byte sht31Address = ??;
	Adafruit_SHT31 sht31 = Adafruit_SHT31();
	


public:
	bool setup();

	float getTemperature();
	float getHumidity();
};


// I2C internal bus
// PA23 SCL_A
// PA22 SDA_A