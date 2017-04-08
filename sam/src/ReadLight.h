#pragma once

#include <Arduino.h>
#include <Wire.h>

struct dataLight {
	bool ok = false;			// Bolean true if everything was OK
	bool commited = false;
	String lines[8];			// Array of strings to save the lines received
	int lineIndex = 0;
};

class ReadLight {
public:
	void setup();
	dataLight read();
	void reset();
	dataLight results;
	bool debugFlag = false;

private:

	uint8_t BH1730 = 0x29;			// Light sensor I2C address
	uint8_t TIME0 = 0xF0;

	// Variables to manage sensor readings
	uint32_t readyTimer = 0;
	uint32_t readyPause = ((2.8/1000.0) * 964 * (float)(256 - TIME0));
	uint16_t newReading = 0;		// Here we store the light sensor new readings
	uint16_t OldReading = 0;		// Variable for saving readings between loops
	uint16_t tolerance = 2;			// Threshold for considering a reading the same as other
	uint8_t readingRepetitions = 0;		// Whit this we validate readings, we need at least MIN_REP repetitions inside the tolerance
	const uint8_t MIN_REP = 3;

	// Variables to manage light values (just for calibration)
  	float newValue = 0;
  	float oldValue = 0;
		
	// Variables to manage levels (each level represents an octal char)
	uint8_t levelNum = 9;				// Number of different grey levels the sensor can read (the screen should use the same number)
	uint16_t levels[9];					// Array for saving the sensor reading for each level during calibration
	uint8_t newLevel = 0;				// Stores the new detected level
	uint8_t oldLevel = -1;				// Variable for storing previous detected level
	uint8_t lastGoodLevel = 0;			// For storing last used good level
	uint8_t levelRepetitions = 0;		// Whit this we validate levels, we need at least MIN_REP of the same value

	// String variable
	String octalString = "0";

	// Checksum variables
	uint16_t localCheckSum = 0;	// Stores the received text checksum for verification
	String sum = "";

	// Watchdog variables
	uint32_t watchDOG = 0;			// Variable for the watchdog timeout
	uint32_t DOG_TIMEOUT = 2000;	// If no valid char is received in this timeout we restart and calibrate again. in milliseconds

	// State variables
	bool calibrated = false;
	bool TransmittingText = false;	// This is true after receiving STX char (start text) and false after ETX char (end text)
	bool EOT = false;	// End of transmission, true when transmission ends or watchdog kicks in.
	bool ETX = false;

	bool calibrate();
	bool getLight();
	char getChar();
	bool getLevel();
	bool getRawLevel();
	bool checksum();
	void feedDOG();
	bool dogBite();
	void debugOUT(String debugText, bool newLine=true);

};
