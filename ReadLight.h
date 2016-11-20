#ifndef READ_LIGHT_H
#define READ_LIGHT_H

#include <Arduino.h>
#include <Wire.h>

enum debugLog { STARTING, CALIBRATED, STX_CHAR, NEW_LINE, ETX_CHAR, CHECKSUM_OK, CHECKSUM_ERROR, EOT_CHAR, WATCHDOG_TIMEOUT };

const int MAX_LOG = 20;

struct dataLight {
	bool ok = false;			// Bolean true if everything was OK
	String lines[8];			// Array of strings to save the lines received
	int lineIndex = 0;
	debugLog log[MAX_LOG];			// Keeps 20 messages of debug log
	int logIndex = 0;
	bool commited = false;
};

class ReadLight {
public:
	void setup();
	dataLight read();

private:

	dataLight results;

	int BH1730 = 0x29;			// Light sensor I2C address
	uint8_t TIME0 = 0xFB;			//

	// Variables to manage sensor readings
	float newReading = 0;		// Here we store the light sensor new readings
	float OldReading = 0;		// Variable for saving readings between loops
	int repetition = 0;			// Whit this we validate readings, we need at least MIN_REP repetitions inside the tolerance
	int MIN_REP = 2;
	float tolerance = 0.20;		// Threshold for considering a reading the same as other

	// Variables to manage screen color levels
	int levelNum = 9;			// Number of different grey levels the sensor can read (the screen should use the same number)

		///AQUI HAY QUE PONER levelnum en vez de 9 NO ME ACUERDO COMO CONSTINT
	float levels[9];			// Array for saving the sensor reading for each level during calibration
	int newLevel = 0;			// Stores the new detected level
	int oldLevel = 0;			// Variable for storing previous detected level

	// String variable
	String newChar = "0";		
	String buffer;

	// Chacksum variables
	float localCheckSum = 0;	// Stores the received text checksum for verification

	// Watchdog variables
	float watchDOG = 0;			// Variable for the watchdog timeout
	float DOG_TIMEOUT = 3000;	// If no valid char is received in this timeout we restart and calibrate again.

	// State variables
	bool TransmittingText = false;	// This is true after receiving STX char (start text) and false after ETX char (end text)
	bool EOT = false;	// End of transmission, true when transmission ends or watchdog kicks in.

	float getLight();
	float getValue();
	int getLevel(float value);
	char getChar();
	bool calibrate();
	bool checksum();
	void feedDOG();
	bool dogBite();
	void log(debugLog message);
};


#endif