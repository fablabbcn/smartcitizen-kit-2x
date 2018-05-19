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

	// return sht31.readTemperature();
	sht31.update(true);
	return sht31.temperature;
}
float AlphaDelta::getHumidity()
{

	// return sht31.readHumidity();
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

	return (result * 0.015625) / getPGAgain(wichElectrode.adc);
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

	// Output from -1440 to +1400 nA
	for (int16_t i=-1400; i<1400; i++) {
		tester.setCurrent(tester.electrode_W, i);
		SerialUSB.print(tester.getCurrent(tester.electrode_W));
		SerialUSB.print(",");
		SerialUSB.print(getElectrode(wichElectrode_W));
		SerialUSB.print(",");
		tester.setCurrent(tester.electrode_A, i);
		SerialUSB.print(tester.getCurrent(tester.electrode_A));
		SerialUSB.print(",");
		SerialUSB.println(getElectrode(wichElectrode_A));

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
#endif

// SHT31 (Temperature and Humidity)
bool Sck_Aux_SHT31::begin()
{

	auxWire.begin();

	// Send reset command
	sendComm(SOFT_RESET);

	update();

	return true;
}
bool Sck_Aux_SHT31::stop()
{

	// It will go to idle state by itself after 1ms
	return true;
}
bool Sck_Aux_SHT31::update(bool wait)
{

	// If last update was less than 2 sec ago dont do it again
	if (millis() - lastUpdate < 2000) return true;

	uint8_t readbuffer[6];
	sendComm(SINGLE_SHOT_HIGH_REP);

	auxWire.requestFrom(address, (uint8_t)6);

	// Wait for answer (datasheet says 15ms is the max)
	uint32_t started = millis();
	while(auxWire.available() != 6) {
		if (millis() - started > timeout) return 0;
	}

	// Read response
	for (uint8_t i=0; i<6; i++) readbuffer[i] = auxWire.read();

	uint16_t ST, SRH;
	ST = readbuffer[0];
	ST <<= 8;
	ST |= readbuffer[1];

	// Check Temperature crc
	if (readbuffer[2] != crc8(readbuffer, 2)) return false;

	SRH = readbuffer[3];
	SRH <<= 8;
	SRH |= readbuffer[4];

	// check Humidity crc
	if (readbuffer[5] != crc8(readbuffer+3, 2)) return false;

	double temp = ST;
	temp *= 175;
	temp /= 0xffff;
	temp = -45 + temp;
	temperature = (float)temp;

	double shum = SRH;
	shum *= 100;
	shum /= 0xFFFF;
	humidity = (float)shum;

	lastUpdate = millis();

	return true;
}
void Sck_Aux_SHT31::sendComm(uint16_t comm)
{
	auxWire.beginTransmission(address);
	auxWire.write(comm >> 8);
	auxWire.write(comm & 0xFF);
	auxWire.endTransmission();
}
uint8_t Sck_Aux_SHT31::crc8(const uint8_t *data, int len)
{

	/* CRC-8 formula from page 14 of SHT spec pdf */

	/* Test data 0xBE, 0xEF should yield 0x92 */

	/* Initialization data 0xFF */
	/* Polynomial 0x31 (x8 + x5 +x4 +1) */
	/* Final XOR 0x00 */

	const uint8_t POLYNOMIAL(0x31);
	uint8_t crc(0xFF);

	for ( int j = len; j; --j ) {
		crc ^= *data++;
		for ( int i = 8; i; --i ) {
			crc = ( crc & 0x80 )
				? (crc << 1) ^ POLYNOMIAL
				: (crc << 1);
		}
	}
	return crc;
}
