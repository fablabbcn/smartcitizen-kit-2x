#pragma once

#include <Sensors.h>

// Adafruit library for ADS1x15 12/16 bits ADC
#include <Adafruit_ADS1015.h>

// ADS Tester (Only supported in version 2.0 and greater)
// This function is only for internal purposes
// #define adsTest 	// Uncomment for enabling tester board support (remember selecting board lookup table in GasesBoardTester.h)

#ifdef adsTest
#include "GasesBoardTester.h"
#endif

static SensorInfo info_ads1x15 =
{
	SENSOR_ADS1X15,                                                 // SensorType
	"ADS1X15",                                                      // Name
	{ 0x48, 0x49, 0x4A, 0x4B },                                     // I2C address list (8 max)
	4,                                                              // Number of provided measurements
	{ 133, 134, 135, 136 },                                         // Legacy platform Id's list for corresponding measurements (0 if is not defined)
	{ &Voltage_ch0, &Voltage_ch1, &Voltage_ch2, &Voltage_ch3 }      // Measurement list
};

class Ctrl_ADS1X15: public Sensor
{
	public:

		Ctrl_ADS1X15(TwoWire * _wire);
		bool start(TwoWire * _wire, byte address);
		bool stop();
		int8_t getReading(const Measurement * measurement, char * buff);
		SensorInfo * info = &info_ads1x15;

		#ifdef adsTest
		uint8_t adsChannelW = 0; // Default channel for WE
		uint8_t adsChannelA = 1; // Default channel for AE
		void setTesterCurrent(int16_t wichCurrent, uint8_t wichChannel);
		// double preVoltA = -99;
		// double preVoltW = -99;
		// double threshold = 0.05;
		// uint8_t maxErrorsA = 5;
		// uint8_t maxErrorsW = 5;
		void runTester(uint8_t wichChannel);
		testerGasesBoard tester;
		#endif

	private:
		bool started = false;
		float VOLTAGE = 3.3;
		Adafruit_ADS1115 * _ads;// = Adafruit_ADS1115(&auxWire);

	// TODO
	// Test ADS1015
};

