#pragma once

#include <SckAux.h>
#include "Adafruit_SHT31.h"		// To be replaced by internal lib
#include <MCP342X.h>

// AlphaDeltaTester
#ifdef deltaTest
#include <AlphaDeltaTester.h>
#endif

extern TwoWire auxWire;

struct Resistor
{
	byte address;
	byte channel;
};

struct Electrode
{
	MCP342X adc;
	byte channel;
	Resistor resistor;
	uint8_t gain;
};

enum ALPHA_GAS 
{
	ALPHA_CO, 
	ALPHA_NO2, 
	ALPHA_NO2_O3
};

struct CalibrationData
{
	ALPHA_GAS GAS;
	float ZERO_CURR_W;
	float ZERO_CURR_A;
	float SENSITIVITY[2];
};

struct AlphaSensor
{
	Electrode electrode_A;
	Electrode electrode_W;
	CalibrationData calData;
};

// Temperature and Humidity
class Sck_Aux_SHT31
{
	// Datasheet
	// https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/2_Humidity_Sensors/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf
	// This code is based on Adafruit SHT31 library, thanks! (https://github.com/adafruit/Adafruit_SHT31)
	private:
		// Commands
		uint8_t address = 0x44;
		const uint16_t SOFT_RESET = 0x30A2;
		const uint16_t SINGLE_SHOT_HIGH_REP = 0x2400;

		// TO be removed with state machine asynchronous
		uint32_t timeout = 20;	// Time in ms to wait for a reading

		void sendComm(uint16_t comm);
		uint8_t crc8(const uint8_t *data, int len);
		uint32_t lastUpdate = 0;
	public:
		float temperature;
		float humidity;
		bool begin();
		bool stop();
		bool update(bool wait=true);
};

class AlphaDelta
{
	public:

		// Init functions
		bool begin();
		bool alreadyStarted = false;

		// SHT31 Temperature and Humidity Sensor
		const byte sht31Address = 0x44;

		// Adafruit_SHT31 sht31 = Adafruit_SHT31();
		Sck_Aux_SHT31 sht31;

		float getTemperature();
		float getHumidity();


		// Alphasense Sensors (Slot 1,2 and 3)
		const uint8_t initGain = 0;
		const float ohmsPerStep = 392.1568;     // Resistor conversion constant in Ohms. (100,000 / 255)
		MCP342X ADC_1 = MCP342X(0x69);
		MCP342X ADC_2_3 = MCP342X(0x68);

		// Slot 1 sensor
		AlphaSensor Slot1 = {
			{ADC_1, MCP342X_CHANNEL_1, {0x55, 0x01}, initGain},		// Electrode A
			{ADC_1, MCP342X_CHANNEL_2, {0x55, 0x00}, initGain},		// Electrode W
			{ALPHA_CO, (-68.1), (-13.9), {601.9, 0}} // board U		// Gas type, zero current W, zero current A, sensitivity TODO move this to eeprom inside alpha board
			/* {ALPHA_CO, (-69.4), (-18.6), {493.1, 0}} // board 1 */
		};

		// Slot 2 sensor
		AlphaSensor Slot2 = {
			{ADC_2_3, MCP342X_CHANNEL_3, {0x56, 0x01}, initGain},		// Electrode A
			{ADC_2_3, MCP342X_CHANNEL_4, {0x56, 0x00}, initGain},		// Electrode W
			{ALPHA_NO2, 31.5, 17.7, {(-383.7), 0}} // Board U
			/* {ALPHA_NO2, 25.9, 15.4, {(-384.9), 0}} // Board 1 */
		};

		// Slot 3 sensor
		AlphaSensor Slot3 = {
			{ADC_2_3, MCP342X_CHANNEL_1, {0x54, 0x01}, initGain},		// Electrode A
			{ADC_2_3, MCP342X_CHANNEL_2, {0x54, 0x00}, initGain},		// Electrode W
			{ALPHA_NO2_O3, 23.01, 14.5, {(-446.36), (-506.96)}} // Board U
			/* {ALPHA_NO2_O3, 23.33, 19.86, {(-466.29), (-466.29)}} // Board 1 */

		};

		uint32_t getPot(Electrode wichElectrode);				// Return value in Ohms
		void setPot(Electrode wichElectrode, uint32_t value);	// Accept value in Ohms (0-100,000)
		uint8_t getPGAgain(MCP342X adc);
		float getElectrodeGain(Electrode wichElectrode);
		double getElectrode(Electrode wichElectrode);
		float getPPM(AlphaSensor wichSlot);
		String getUID();
		bool writeByte(uint8_t dataAddress, uint8_t data);
		uint8_t readByte(uint8_t dataAddress);

		#ifdef deltaTest
		testerAlphaDelta tester;
		void runTester(uint8_t wichSlot);
		void setTesterCurrent(int16_t wichCurrent, uint8_t wichSlot);
		#endif

	private:

		// EEPROM 24AA025
		const byte eepromAddress = 0x51;
};

