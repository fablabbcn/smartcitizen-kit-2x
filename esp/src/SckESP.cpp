#include "SckESP.h"

void SckESP::setup() {

	// LED outputs
	pinMode(pinLED_LEFT, OUTPUT);
	pinMode(pinLED_RIGHT, OUTPUT);
	digitalWrite(pinLED_LEFT, HIGH);
	digitalWrite(pinLED_RIGHT, HIGH);

	Serial.begin(serialBaudrate);
	Serial.setDebugOutput(false);

}
void SckESP::update() {

}