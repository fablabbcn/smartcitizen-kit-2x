#include <AlphaDelta.h>

bool AlphaDelta::begin()
{

	if (!I2Cdetect(sht31Address) ||
			!(I2Cdetect(Slot1.electrode_A.resistor.address)) ||
			!(I2Cdetect(Slot1.electrode_W.resistor.address)) ||
			!(I2Cdetect(Slot2.electrode_A.resistor.address)) ||
			!(I2Cdetect(Slot2.electrode_W.resistor.address)) ||
			!(I2Cdetect(Slot3.electrode_A.resistor.address)) ||
			!(I2Cdetect(Slot3.electrode_W.resistor.address))) return false;

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

float AlphaDelta::getTemperature()
{
	sht31.update(true);
	return sht31.temperature;
}
float AlphaDelta::getHumidity()
{
	sht31.update(true);
	return sht31.humidity;
}
uint32_t AlphaDelta::getPot(Electrode wichElectrode)
{

	return ((255 - readI2C(wichElectrode.resistor.address, wichElectrode.resistor.channel)) * ohmsPerStep);
}
void AlphaDelta::setPot(Electrode wichElectrode, uint32_t value)
{

	int data=0x00;
	if (value>100000) value = 100000;
	data = 255 - (int)(value/ohmsPerStep);		// POT's are connected 'upside down' (255 - step)

	writeI2C(wichElectrode.resistor.address, 16, 192);        	// select WR (volatile) registers in POT
	writeI2C(wichElectrode.resistor.address, wichElectrode.resistor.channel, data);
}
uint8_t AlphaDelta::getPGAgain(MCP342X adc)
{
	uint8_t gainPGA = adc.getConfigRegShdw() & 0x3;
	return pow(2, gainPGA);
}
float AlphaDelta::getElectrodeGain(Electrode wichElectrode)
{

	return (((getPot(wichElectrode) + 85) / 10000.0f) + 1) * getPGAgain(wichElectrode.adc);
}
// Returns electrode value in mV
double AlphaDelta::getElectrode(Electrode wichElectrode)
{

	static int32_t result;

	// Gain can be changed before calling this funtion with: wichElectrode.gain = newGain (0->gain of 1, 1->gain of 2, 2->gain of 3 or 3->gain of 8)
	wichElectrode.adc.configure( MCP342X_MODE_ONESHOT | MCP342X_SIZE_18BIT | wichElectrode.gain);
	wichElectrode.adc.startConversion(wichElectrode.channel);
	wichElectrode.adc.getResult(&result);

	return (result * 0.015625) / getElectrodeGain(wichElectrode);
}
float AlphaDelta::getPPM(AlphaSensor wichSlot)
{
    switch(wichSlot.calData.GAS) {

        case ALPHA_CO: {
            // CO [ppm] = ((6.36 * WE - ZERO_CURR_W) - n*(6.36 * AE + ZERO_CURR_A)) / SENSITIVITY
            float resultCO = ((6.36 * getElectrode(wichSlot.electrode_W)) - wichSlot.calData.ZERO_CURR_W -
                  wichSlot.calData.ZERO_CURR_W/wichSlot.calData.ZERO_CURR_A * (6.36 * getElectrode(wichSlot.electrode_A) - wichSlot.calData.ZERO_CURR_A)) /
                  wichSlot.calData.SENSITIVITY[0];
	    resultCO =  max(0, resultCO);
	    return resultCO;
            break;

	} case ALPHA_NO2: {
            // NO2 [ppm] = ((6.36 * WE - ZERO_CURR_W) - n*(6.36 * AE - ZERO_CURR_A)) / SENSITIVITY
            // NO2 [ppb] = ((6.36 * WE - ZERO_CURR_W) - n*(6.36 * AE - ZERO_CURR_A)) / SENSITIVITY
            float resultNO2 = (((6.36 * getElectrode(wichSlot.electrode_W) - wichSlot.calData.ZERO_CURR_W) -
                wichSlot.calData.ZERO_CURR_W/wichSlot.calData.ZERO_CURR_A * (6.36 * getElectrode(wichSlot.electrode_A)-wichSlot.calData.ZERO_CURR_A)) /
                wichSlot.calData.SENSITIVITY[0]) * 1000;
	    resultNO2 =  max(0, resultNO2);
	    return resultNO2;
            break;

	} case ALPHA_NO2_O3: {
	    // O3 [ppm] =  ((6.36 * WE - ZERO_CURR_W) - n*(6.36 * AE - ZERO_CURR_A) - getPPM(NO2) * SENSITIVITY_NO2) / SENSITIVITY_O3
	    float resultO3 = ((6.36 * getElectrode(wichSlot.electrode_W) - wichSlot.calData.ZERO_CURR_W) -
		  ((wichSlot.calData.ZERO_CURR_W / wichSlot.calData.ZERO_CURR_A) * (6.36 * getElectrode(wichSlot.electrode_A) - wichSlot.calData.ZERO_CURR_A)) -
		  ((getPPM(Slot2) / 1000) * wichSlot.calData.SENSITIVITY[1])) /
		    wichSlot.calData.SENSITIVITY[0];
	    resultO3 =  max(0, resultO3);
	    return resultO3;
            break;
	}
    }
}
String AlphaDelta::getUID()
{

	char data[24];
	uint8_t eeaddr = 0xf8;
	sprintf(data, "%02x:", readByte(eeaddr++));
	for(uint8_t pos = 0; pos<7; pos++){
		sprintf(data, "%s:%02x", data, readByte(eeaddr++));
	}
	return String(data);
}
bool AlphaDelta::writeByte(uint8_t dataAddress, uint8_t data)
{
	auxWire.beginTransmission(eepromAddress);
	auxWire.write(dataAddress);
	auxWire.write(data);
	if (auxWire.endTransmission() == 0) return true;
	return false;
}
uint8_t AlphaDelta::readByte(uint8_t dataAddress)
{
	auxWire.beginTransmission(eepromAddress);
	auxWire.write(dataAddress);
	if (auxWire.endTransmission(false)) return 0;
	if(!auxWire.requestFrom(eepromAddress, 1)) return 0;
	return auxWire.read();
}

#ifdef deltaTest
void AlphaDelta::runTester(uint8_t wichSlot)
{

	Electrode wichElectrode_W;
	Electrode wichElectrode_A;

	switch(wichSlot) {
		case 1: {
				wichElectrode_W = Slot1.electrode_W;
				wichElectrode_A = Slot1.electrode_A;
				break;
			} case 2: {
				wichElectrode_W = Slot2.electrode_W;
				wichElectrode_A = Slot2.electrode_A;
				break;
			} case 3: {
				wichElectrode_W = Slot3.electrode_W;
				wichElectrode_A = Slot3.electrode_A;
				break;
			}
		default: break;
	}

	// Print headers
	SerialUSB.println("testW,readW,testA,readA");

	// Output from -1400 to +1400 nA
	for (int16_t i=-1400; i<1400; i++) {
		tester.setCurrent(tester.electrode_W, i);
		double currVoltW = getElectrode(wichElectrode_W);
		if (preVoltW != -99) if ((currVoltW - preVoltW) < threshold) maxErrorsW--;
		preVoltW = currVoltW;
		if (maxErrorsW == 0) SerialUSB.println("Working electrode fail !!!");

		tester.setCurrent(tester.electrode_A, i);
		double currVoltA = getElectrode(wichElectrode_A);
		if (preVoltA != -99) if ((currVoltA - preVoltA) < threshold) maxErrorsA--;
		preVoltA = currVoltA;
		if (maxErrorsA == 0) SerialUSB.println("Auxiliary electrode fail !!!");

		SerialUSB.print(tester.getCurrent(tester.electrode_W));
		SerialUSB.print(",");
		SerialUSB.print(currVoltW, 8);
		SerialUSB.print(",");
		SerialUSB.print(tester.getCurrent(tester.electrode_A));
		SerialUSB.print(",");
		SerialUSB.println(currVoltA, 8);
	}
}
void AlphaDelta::setTesterCurrent(int16_t wichCurrent, uint8_t wichSlot)
{

	Electrode wichElectrode_W;
	Electrode wichElectrode_A;

	switch(wichSlot) {
		case 1: {
				wichElectrode_W = Slot1.electrode_W;
				wichElectrode_A = Slot1.electrode_A;
				break;
			} case 2: {
				wichElectrode_W = Slot2.electrode_W;
				wichElectrode_A = Slot2.electrode_A;
				break;
			} case 3: {
				wichElectrode_W = Slot3.electrode_W;
				wichElectrode_A = Slot3.electrode_A;
				break;
			}
		default: break;
	}

	SerialUSB.print("Setting test current to: ");
	SerialUSB.println(wichCurrent);

	tester.setCurrent(tester.electrode_W, wichCurrent);
	tester.setCurrent(tester.electrode_A, wichCurrent);

	SerialUSB.print("Tester Electrode W: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_W));
	SerialUSB.print("Alphadelta ");
	SerialUSB.print(wichSlot);
	SerialUSB.print("W: ");
	SerialUSB.println(getElectrode(wichElectrode_W));

	SerialUSB.print("Tester Electrode A: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_A));
	SerialUSB.print("Alphadelta ");
	SerialUSB.print(wichSlot);
	SerialUSB.print("A: ");
	SerialUSB.println(getElectrode(wichElectrode_A));
}
bool AlphaDelta::autoTest()
{
	Electrode wichElectrode_W;
	Electrode wichElectrode_A;

	pinMode(pinBLUE, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinRED, OUTPUT);
	digitalWrite(pinGREEN, HIGH);
	digitalWrite(pinRED, HIGH);
	digitalWrite(pinBLUE, LOW);

	// Autoselect slot based on response (if none responds it fails)
	for (uint8_t i=1; i<4; i++) {
		switch(i) {
			case 1:	wichElectrode_W = Slot1.electrode_W; wichElectrode_A = Slot1.electrode_A; break;
			case 2:	wichElectrode_W = Slot2.electrode_W; wichElectrode_A = Slot2.electrode_A; break;
			case 3: wichElectrode_W = Slot3.electrode_W; wichElectrode_A = Slot3.electrode_A; break;
		}
		tester.setCurrent(tester.electrode_W, 0);
		double zeroVolt = getElectrode(wichElectrode_W);
		tester.setCurrent(tester.electrode_W, 500);
		double fiveVolt = getElectrode(wichElectrode_W);
		if ((fiveVolt - zeroVolt) > 5) {
			SerialUSB.println("Tesing slot " + String(i));
			break;
		}
	}
	uint8_t multiplier = 25;
	bool blueState = false;
	for (int16_t i=-1400; i<1400; i+=multiplier) {

		tester.setCurrent(tester.electrode_W, i);
		double currVoltW = getElectrode(wichElectrode_W);
		if (preVoltW != -99) if ((currVoltW - preVoltW) < threshold * multiplier) maxErrorsW--;
		preVoltW = currVoltW;
		if (maxErrorsW == 0) {
			SerialUSB.println("\r\nWorking electrode fail !!!");
			return false;
		}

		tester.setCurrent(tester.electrode_A, i);
		double currVoltA = getElectrode(wichElectrode_A);
		if (preVoltA != -99) if ((currVoltA - preVoltA) < threshold) maxErrorsA--;
		preVoltA = currVoltA;
		if (maxErrorsA == 0) {
			SerialUSB.println("\r\nAuxiliary electrode fail !!!");
			return false;
		}

		SerialUSB.print(".");
		digitalWrite(pinBLUE, blueState);
		blueState = !blueState;
	}
	if (maxErrorsW > 0 && maxErrorsA > 0) {
		SerialUSB.println("\r\nTest OK");
		return true;
	}
}
#endif

