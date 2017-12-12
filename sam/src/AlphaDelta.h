#pragma once

#include <SckAux.h>
#include "Adafruit_SHT31.h"
#include <MCP342X.h>

struct Resistor {
	byte address;
	byte channel;
};

struct Electrode {
	MCP342X adc;
	byte channel;
	Resistor resistor;
	uint8_t gain;
};

struct AlphaSensor {
	Electrode electrode_A;
	Electrode electrode_W;
};

class AlphaDelta {
	public:

		// Init functions
		bool begin();
		bool alreadyStarted = false;
		
		// SHT31 Temperature and Humidity Sensor
		const byte sht31Address = 0x44;

		Adafruit_SHT31 sht31 = Adafruit_SHT31();
		
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
			{ADC_1, MCP342X_CHANNEL_2, {0x55, 0x00}, initGain}		// Electrode W
		};

		// Slot 2 sensor
		AlphaSensor Slot2 = {
			{ADC_2_3, MCP342X_CHANNEL_3, {0x56, 0x01}, initGain},		// Electrode A
			{ADC_2_3, MCP342X_CHANNEL_4, {0x56, 0x00}, initGain}		// Electrode W
		};

		// Slot 3 sensor
		AlphaSensor Slot3 = {
			{ADC_2_3, MCP342X_CHANNEL_1, {0x54, 0x01}, initGain},		// Electrode A
			{ADC_2_3, MCP342X_CHANNEL_2, {0x54, 0x00}, initGain}		// Electrode W
		};

		uint32_t getPot(Electrode wichElectrode);				// Return value in Ohms
		void setPot(Electrode wichElectrode, uint32_t value);	// Accept value in Ohms (0-100,000)
		uint8_t getPGAgain(MCP342X adc);
		float getElectrodeGain(Electrode wichElectrode);
		double getElectrode(Electrode wichElectrode);

	private:
};