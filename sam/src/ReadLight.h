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
	dataLight results;
	void reset();
	bool debugFlag = false;

private:

	int BH1730 = 0x29;			// Light sensor I2C address
	uint8_t TIME0 = 0xFB;

	// Variables to manage sensor readings
	float newReading = 0;		// Here we store the light sensor new readings
	float OldReading = 0;		// Variable for saving readings between loops
	int repetition = 0;			// Whit this we validate readings, we need at least MIN_REP repetitions inside the tolerance
	int MIN_REP = 2;
	float tolerance = 0.20;		// Threshold for considering a reading the same as other

	// Variables to manage light values
  	float oldValue = 0;
  	float currentValue = 0;

	// Variables to manage screen color levels
	int levelNum = 9;			// Number of different grey levels the sensor can read (the screen should use the same number)

		///AQUI HAY QUE PONER levelnum en vez de 9 NO ME ACUERDO COMO CONSTINT
	float levels[9];			// Array for saving the sensor reading for each level during calibration
	int newLevel = 0;			// Stores the new detected level
	int oldLevel = 0;			// Variable for storing previous detected level
	float getValueTimer = 0;	// Timer for calling get light function when looking for new values
	float getValueRetry = (2.8/1000.0) * 964 * (float)(256 - TIME0);

	// String variable
	String newChar = "0";		
	String lightBuffer;
	String octalString = "0";

	// Checksum variables
	float localCheckSum = 0;	// Stores the received text checksum for verification
	String sum = "";

	// Watchdog variables
	float watchDOG = 0;			// Variable for the watchdog timeout
	float DOG_TIMEOUT = 2000;	// If no valid char is received in this timeout we restart and calibrate again. in milliseconds

	// State variables
	bool calibrated = false;
	bool TransmittingText = false;	// This is true after receiving STX char (start text) and false after ETX char (end text)
	bool EOT = false;	// End of transmission, true when transmission ends or watchdog kicks in.
	bool ETX = false;

	float getLight();
	float getValue();
	int getLevel();
	int closerLevel(float reading);
	bool calibrate();
	char getChar();
	int checksum();
	void feedDOG();
	bool dogBite();
	void debugOUT(String debugText, bool newLine=true);
};
