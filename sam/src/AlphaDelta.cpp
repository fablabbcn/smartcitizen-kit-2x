#include <AlphaDelta.h>

bool AlphaDelta::begin() {

	if (!I2Cdetect(sht31Address)) return false;

	if (alreadyStarted) return true;
	alreadyStarted = true;

	sht31.begin();

	// Set all potentiometers to 0
	setPot(Slot1.electrode_A, 0);
	setPot(Slot1.electrode_W, 0);
	setPot(Slot2.electrode_A, 0);
	setPot(Slot2.electrode_W, 0);
	setPot(Slot3.electrode_A, 0);
	setPot(Slot3.electrode_W, 0);

	return true;
}

float AlphaDelta::getTemperature() {

	return sht31.readTemperature();
}

float AlphaDelta::getHumidity() {

	return sht31.readHumidity();
}

uint32_t AlphaDelta::getPot(Electrode wichElectrode) {

	return ((255 - readI2C(wichElectrode.resistor.address, wichElectrode.resistor.channel)) * ohmsPerStep);
}

void AlphaDelta::setPot(Electrode wichElectrode, uint32_t value) {

	int data=0x00;
	if (value>100000) value = 100000;
	data = 255 - (int)(value/ohmsPerStep);		// POT's are connected 'upside down' (255 - step)
	
	writeI2C(wichElectrode.resistor.address, 16, 192);        	// select WR (volatile) registers in POT
	writeI2C(wichElectrode.resistor.address, wichElectrode.resistor.channel, data);
}

uint8_t AlphaDelta::getPGAgain(MCP342X adc) {
	uint8_t gainPGA = adc.getConfigRegShdw() & 0x3;
	return pow(2, gainPGA);
}

float AlphaDelta::getElectrodeGain(Electrode wichElectrode) {

	return (((getPot(wichElectrode) + 85) / 10000.0f) + 1) * getPGAgain(wichElectrode.adc);
}

// Returns electrode value in mV
double AlphaDelta::getElectrode(Electrode wichElectrode) {

	static int32_t result;

	// Gain can be changed before calling this funtion with: wichElectrode.gain = newGain (0->gain of 1, 1->gain of 2, 2->gain of 3 or 3->gain 0f 8)
	wichElectrode.adc.configure( MCP342X_MODE_ONESHOT | MCP342X_SIZE_18BIT | wichElectrode.gain);	
	wichElectrode.adc.startConversion(wichElectrode.channel);
	wichElectrode.adc.getResult(&result);

	return (result * 0.015625) / getPGAgain(wichElectrode.adc);
}

