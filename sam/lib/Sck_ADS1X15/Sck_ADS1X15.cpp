#include "Sck_ADS1X15.h"


bool Ctrl_ADS1X15::start(byte address)
{
	if (!I2Cdetect(&auxWire, address)) return false;
	if (started) return true;

	devAddr = address;
	_ads.begin(devAddr);
	
	started = true;
	return true;
}

bool Ctrl_ADS1X15::stop()
{
	started = false;
	return true;
}

int8_t Ctrl_ADS1X15::getReading(MetricType wichSensor, char *buffer)
{
	uint8_t wichChannel;

	switch(wichSensor) {
		case SENSOR_ADS1X15_0:
			wichChannel = 0;
			break;
		case SENSOR_ADS1X15_1:
			wichChannel = 1;
			break;
		case SENSOR_ADS1X15_2:
			wichChannel = 2;
			break;
		case SENSOR_ADS1X15_3:
			wichChannel = 3;
			break;
		default:
			return -1;
	}

	// Reset gain
	_ads.setGain(GAIN_TWOTHIRDS);
	double voltage_range = 6.144;

	// Get value with full range
	uint16_t value = _ads.readADC_SingleEnded(wichChannel);

	// If value is under 4.096v increase the gain depending on voltage
	if (value < 21845) {
		if (value > 10922) {

			// 1x gain, 4.096V
			_ads.setGain(GAIN_ONE);
			voltage_range = 4.096;

		} else if (value > 5461) {

			// 2x gain, 2.048V
			_ads.setGain(GAIN_TWO);
			voltage_range = 2.048;

		} else if (value > 2730) {

			// 4x gain, 1.024V
			_ads.setGain(GAIN_FOUR);
			voltage_range = 1.024;

		} else if (value > 1365) {

			// 8x gain, 0.25V
			_ads.setGain(GAIN_EIGHT);
			voltage_range = 0.512;

		} else {

			// 16x gain, 0.125V
			_ads.setGain(GAIN_SIXTEEN);
			voltage_range = 0.256;
		}

		// Get the value again
		value = _ads.readADC_SingleEnded(wichChannel);
	}

	float result = (float)value / 32768 * voltage_range;
	snprintf(buffer, sizeof(buffer), "%.*l", precision, result);
	return 0;
}

#ifdef adsTest
void Ctrl_ADS1X15::setTesterCurrent(int16_t wichCurrent, uint8_t wichChannel)
{
	// Support both combinations of ADC channels:
	// wichChannel = 0 (default) -> WE in ADS_Ch0 and AE in ADS_Ch1
	// wichChannel = 1 			 -> WE in ADS_Ch2 and AE in ADS_Ch3
	if (wichChannel > 0) {
		adsChannelW = 2;
		adsChannelA = 3;
	}

	SerialUSB.print("Setting test current to: ");
	SerialUSB.println(wichCurrent);

	tester.setCurrent(tester.electrode_W, wichCurrent);
	tester.setCurrent(tester.electrode_A, wichCurrent);

	SerialUSB.print("Tester Electrode W: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_W));
	SerialUSB.print("ISB W:");
	this->getReading(adsChannelW);
	SerialUSB.println(this->reading);

	SerialUSB.print("Tester Electrode A: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_A));
	SerialUSB.print("ISB A: ");
	this->getReading(adsChannelA);
	SerialUSB.println(this->reading);

}

void Sck_ADS1X15::runTester(uint8_t wichChannel)
{
	// Support both combinations of ADC channels:
	// wichChannel = 0 (default) -> WE in ADS_Ch0 and AE in ADS_Ch1
	// wichChannel = 1 			 -> WE in ADS_Ch2 and AE in ADS_Ch3
	if (wichChannel > 0) {
		adsChannelW = 2;
		adsChannelA = 3;
	}

	// Print headers
	SerialUSB.println("testW,readW,testA,readA");

	// Output from -1400 to +1400 nA
	for (int16_t i=-1400; i<1400; i++) {
		tester.setCurrent(tester.electrode_W, i);
		double currVoltW = -10;

		if (this->getReading(adsChannelW))  currVoltW = this->reading;
		else SerialUSB.println("Error in Working electrode");

		// if (preVoltW != -99) if ((currVoltW - preVoltW) < threshold) maxErrorsW--;
		// preVoltW = currVoltW;
		// if (maxErrorsW == 0) SerialUSB.println("Working electrode fail !!!");

		tester.setCurrent(tester.electrode_A, i);
		double currVoltA = -10;

		if (this->getReading(adsChannelA)) currVoltA = this->reading;
		else SerialUSB.println("Error in Working electrode");

		// if (preVoltA != -99) if ((currVoltA - preVoltA) < threshold) maxErrorsA--;
		// preVoltA = currVoltA;
		// if (maxErrorsA == 0) SerialUSB.println("Auxiliary electrode fail !!!");

		SerialUSB.print(tester.getCurrent(tester.electrode_W));
		SerialUSB.print(",");
		SerialUSB.print(currVoltW, 8);
		SerialUSB.print(",");
		SerialUSB.print(tester.getCurrent(tester.electrode_A));
		SerialUSB.print(",");
		SerialUSB.println(currVoltA, 8);
	}
	SerialUSB.println("Run test finished!");
}
#endif

