#include "GasesBoardTester.h"

bool testerGasesBoard::begin()
{
	setCurrent(electrode_W, 0);
	setCurrent(electrode_A, 0);
	return true;
}

void testerGasesBoard::setPot(Resistor wichPot, uint8_t wichValue)
{
	auxWire.beginTransmission(wichPot.address);
	auxWire.write(wichPot.SET);
	auxWire.write(wichValue);
	auxWire.endTransmission(wichPot.address);
}

uint8_t testerGasesBoard::readPot(Resistor wichPot)
{
	int data = 0;

	auxWire.beginTransmission(wichPot.address);
	auxWire.write(wichPot.READ);
	auxWire.endTransmission(wichPot.address);
	auxWire.requestFrom(wichPot.address, 2);

	unsigned long time = millis();
	while (!auxWire.available()) if ((millis() - time)>500) return 0x00;
	while (auxWire.available()) data = auxWire.read();
	return data;
}

void testerGasesBoard::setCurrent(Electrode wichElectrode, int16_t wichCurrent)
{
	int16_t currentIndex = wichCurrent + DELTA_CURRENT;
	uint8_t currentValues[2];
	uint16_t pause = 1;

	if (wichElectrode.type == ELECTRODE_W) {
		memcpy(currentValues, CURRENT_LOOKUP_W[currentIndex], 2);
		pause = abs(electrode_W.nA - wichCurrent);
		electrode_W.nA = wichCurrent;
	} else {
		memcpy(currentValues, CURRENT_LOOKUP_A[currentIndex], 2);
		pause = abs(electrode_A.nA - wichCurrent);
		electrode_A.nA = wichCurrent;
	}

	setPot(wichElectrode.pots[0], currentValues[0]);
	setPot(wichElectrode.pots[1], currentValues[1]);

	delay(pause);
}

int16_t testerGasesBoard::getCurrent(Electrode wichElectrode)
{
	uint8_t find0 = readPot(wichElectrode.pots[0]);
	uint8_t find1 = readPot(wichElectrode.pots[1]);

	uint16_t i = 0;

	if (wichElectrode.type == ELECTRODE_W) {
		while( (i < totalLevels_W) && (CURRENT_LOOKUP_W [i] [0]!= find0) ) i++;
		while( (i < totalLevels_W) && (CURRENT_LOOKUP_W [i] [1]!= find1) ) i++;
	} else if (wichElectrode.type == ELECTRODE_A) {
		while( (i < totalLevels_A) && (CURRENT_LOOKUP_A [i] [0]!= find0) ) i++;
		while( (i < totalLevels_A) && (CURRENT_LOOKUP_A [i] [1]!= find1) ) i++;
	}

	return i - DELTA_CURRENT;
}
