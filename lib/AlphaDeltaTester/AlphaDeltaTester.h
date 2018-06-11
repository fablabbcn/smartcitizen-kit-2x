#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "currentLookUp.h"

extern TwoWire auxWire;

class testerAlphaDelta {
public:

	struct Resistor {
		byte address;
		byte SET;
		byte READ;
	};

	enum ElectrodeType {ELECTRODE_W, ELECTRODE_A};

	struct Electrode {
		Resistor pots[2];
		ElectrodeType type;
		int16_t nA;
	};

	uint16_t totalLevels_W = sizeof CURRENT_LOOKUP_W / 2;
	uint16_t totalLevels_A = sizeof CURRENT_LOOKUP_A / 2;

	Electrode electrode_W = {{{0x28, 0x00, 0x0C}, {0x28, 0x010, 0x1C}}, ELECTRODE_W};
	Electrode electrode_A = {{{0x2A, 0x00, 0x0C}, {0x2A, 0x010, 0x1C}}, ELECTRODE_A};

	bool begin();
	void setCurrent(Electrode wichElectrode, int16_t wichCurrent);
	int16_t getCurrent(Electrode wichElectrode);

private:

	const uint16_t DELTA_CURRENT = 1400;		// nA threshold (+/- 1400)
	void setPot(Resistor wichPot, uint8_t wichValue);
	uint8_t readPot(Resistor wichPot);
};
