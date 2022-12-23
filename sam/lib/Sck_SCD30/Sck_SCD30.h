#pragma once

#include <Sensors.h>
#include <Wire.h>

// Sparkfun library for SCD30 CO2 sensor
#include <SparkFun_SCD30_Arduino_Library.h>

static SensorInfo info_scd30 =
{
	SENSOR_SCD30,
	"SCD30",
	{ 0x61 },
	3,
	{ 158, 35, 67 },
	{ &Co2, &Temperature, &Humidity }
};

class Ctrl_SCD30: public Sensor
{
	public:
		bool start(TwoWire * _wire, byte address);
		bool stop();
		int8_t getReading(const Measurement * measurement, char * buff);
		SensorInfo * info = &info_scd30;
		SensorConfig * config;

		uint16_t interval(uint16_t newInterval=0);
		bool autoSelfCal(int8_t value=-1);
		uint16_t forcedRecalFactor(uint16_t newFactor=0);
		float tempOffset(float userTemp, bool off=false);

		// bool pressureCompensated = false;

	private:
		bool started = false;
		bool debug = false;

		// uint16_t measInterval = 2; 	// "2-1800 seconds"
		SCD30 sparkfun_scd30;
};

