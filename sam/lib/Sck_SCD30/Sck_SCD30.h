#pragma once

#include <Sensors.h>
#include <Wire.h>

// Sparkfun library for SCD30 CO2 sensor
#include <SparkFun_SCD30_Arduino_Library.h>

static DeviceInfo sck_scd30 =
{
	DEVICE_SCD30, 				// DeviceType
	"SCD30", 					// name
	{ 0x61 }, 					// I2C address list (8 max)
	3, 							// Number of provided metrics
	{{
		 SENSOR_SCD30_CO2, 		// MetricType
		 "Carbon Dioxide",		// name
		 "CO2", 				// short name
		 "ppm", 				// unit
		 2,						// Precision (number of decimal digits)
		 158 					// platform id
	 },{
		 SENSOR_SCD30_TEMP,
		 "Temperature",
		 "TEMP",
		 "C",	
		 2,
		 35
	 },{
		 SENSOR_SCD30_HUM, 
		 "Humidity",
		 "HUM",
		 "%",
		 2,
		 67
	 }}
};

class Ctrl_SCD30: public Device
{
	public:
		bool start(TwoWire * _wire, byte address);
		bool stop();
		int8_t getReading(Metric metric, char * buff);
		DeviceInfo * info = &sck_scd30;

		uint16_t interval(uint16_t newInterval=0);
		bool autoSelfCal(int8_t value=-1);
		uint16_t forcedRecalFactor(uint16_t newFactor=0);
		float tempOffset(float userTemp, bool off=false);

		uint16_t co2 = 0;
		float temperature = 0;
		float humidity = 0;

		bool pressureCompensated = false;

	private:
		bool started = false;

		uint8_t enabled[3][2] = { {SENSOR_SCD30_CO2, 0}, {SENSOR_SCD30_TEMP, 0}, {SENSOR_SCD30_HUM, 0} };
		uint16_t measInterval = 2; 	// "2-1800 seconds"
		SCD30 sparkfun_scd30;
};

