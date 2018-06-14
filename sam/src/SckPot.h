#pragma once

#include <Arduino.h>
#include <Wire.h>


class SckPot {
public:
	
	SckPot(byte addr, byte chann) {
		address = addr;
		channel = chann;
    }
 
	uint32_t getValue();
	bool setValue(float value);

private:
	byte address;
	byte channel;
	const float ohmsPerStep = 392.1568;     // Resistor
};