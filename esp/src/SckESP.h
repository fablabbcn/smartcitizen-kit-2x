#pragma once

#include <Arduino.h>
#include "Shared.h"

class SckESP {
private:

	// Led pins
	const uint8_t pinLED_RIGHT = 5; 	// GPIO5
	const uint8_t pinLED_LEFT = 4;	 	// GPIO4


public:
	void setup();
	void update();

	void espOut(String strOut);
	bool serialDebug = false;

};