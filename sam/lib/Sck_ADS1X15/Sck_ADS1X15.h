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

extern TwoWire auxWire;

static DeviceInfo sck_ads1x15 =
{
	DEVICE_ADS1X15,				// DeviceType
	"ADS1X15",					// name
	BOARD_AUX, 					// DeviceLocation
	{ 0x48, 0x49, 0x4A, 0x4B }, // I2C address list (8 max)
	4, 							// Number of provided metrics
	{{
		 SENSOR_ADS1X15_0, 		// MetricType
		 "Channel 0", 			// title
		 "CH0", 				// short title
		 "V", 					// unit
		 133 					// platform id
	 },{
		 SENSOR_ADS1X15_1, 		// MetricType
		 "Channel 1", 			// title
		 "CH1", 				// short title
		 "V", 					// unit
		 134 					// platform id
	 },{
		 SENSOR_ADS1X15_2, 		// MetricType
		 "Channel 2", 			// title
		 "CH2", 				// short title
		 "V", 					// unit
		 135 					// platform id
	 },{
		 SENSOR_ADS1X15_3, 		// MetricType
		 "Channel 3", 			// title
		 "CH3", 				// short title
		 "V", 					// unit
		 136 					// platform id
	 }}
};

class Ctrl_ADS1X15: public Device
{
	public:

		bool start(byte address);
		bool stop();
		int8_t getReading(MetricType wichSensor, char *buffer);
		DeviceInfo *info = &sck_ads1x15;

		// Override default precision
		uint8_t precision = 6;

		// bool getReading(SensorType wichSensor);

		// bool getReading(uint8_t wichChannel);
		// float reading;

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
		Adafruit_ADS1115 _ads = Adafruit_ADS1115(&auxWire);

	// TODO
	// Test ADS1015
};

