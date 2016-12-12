#include "sckESP.h"


void SckESP::setup() {

	// LED outputs
	pinMode(LED_LEFT, OUTPUT);
	pinMode(LED_RIGHT, OUTPUT);
	digitalWrite(LED_LEFT, HIGH);
	digitalWrite(LED_RIGHT, HIGH);

	Serial.begin(115200);
	Serial.setDebugOutput(true);

};

void SckESP::start() {
	ledBlink(ledLeft, 350);
	ledBlink(ledRight, 350);
};


void espIn(String input) {

};


bool SckESP::saveConf() {

};

bool SckESP::loadConf() {

};

bool SckESP::mqttStart(String server) {

};

bool SckESP::mqttHellow() {

};

bool SckESP::mqttSend(String payload) {

};


/* 	---------------------------------------------
 	|	SmartCitizen Kit Wifi Leds management   |
 	---------------------------------------------
*/

void SckESP::ledSet(uint8_t wichLed, uint8_t value) {
	if(wichLed == ledLeft) Lblink.detach();
	else Rblink.detach();
	ledValue[wichLed] = value;
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
}

void SckESP::ledToggle(uint8_t wichLed) {
	ledValue[wichLed] = abs(ledValue[wichLed] - 1);
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
};

void SckESP::ledBlink(uint8_t wichLed, float rate) {
	if (wichLed == ledLeft) Lblink.attach_ms(rate, ledToggleLeft);
	else if (wichLed == ledRight) Rblink.attach_ms(rate, ledToggleRight);
};
