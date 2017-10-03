#include "ReadSound.h"

//Setup must be executed before read
void ReadSound::setup() {

	reset();
	feedDOG();				// Start timer for watchdog

	debugFlag = false;
}

// Reset values to starting point
void ReadSound::reset() {
	debugOUT("Resetting readSound");

	for (uint8_t i=0; i<8; ++i) results.lines[i] = "";
	results.ok = false;
	results.lineIndex = 0;
}


dataSound ReadSound::read() {

	// para probar voy a intentar leer 000, 001, 010, 011, 100, 101, 110, 111

	// urban.setPot(POT4, );

	prevLevel = level;
	level = analogRead(S4);

	// mean = 

	// SerialUSB.println(tick);

	if (level - prevLevel ) out++;

	return results;
}

// Avoid the watchdog timer to trigger a restart()
void ReadSound::feedDOG() {

	watchDOG = millis();
}

// Check if we need a restart
// @return True if timeout has been reached
bool ReadSound::dogBite() {
	if (millis() - watchDOG > DOG_TIMEOUT) {
		debugOUT("watchdog timeout!!");
		reset();
		return true;
	}
	return false;
}


void ReadSound::debugOUT(const char *strOut, bool newLine) {

  if (debugFlag) {
    if (newLine) SerialUSB.println(strOut);
    else SerialUSB.print(strOut);
  }
}