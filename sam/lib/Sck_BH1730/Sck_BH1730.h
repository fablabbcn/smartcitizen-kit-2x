#pragma once

#include <Sensors.h>
#include <Wire.h>

// Datasheet
// https://www.mouser.es/datasheet/2/348/bh1730fvc-e-1018573.pdf
static SensorInfo info_bh1730 =
{
	SENSOR_BH1730, 		// SensorType
	"BH1730",			// Name
	{ 0x29 }, 			// I2C address list (8 max)
	1, 					// Number of provided measurements
	{ 14 }, 			// Legacy platform Id's list for corresponding measurements (0 if is not defined)
	{ &Light }			// Measurement list
};

class Ctrl_BH1730 : public Sensor
{
	public:
		bool start(TwoWire * _wire, byte address);
		bool stop();
		int8_t getReading(const Measurement * measurement, char * buff);
		SensorInfo * info = &info_bh1730;

	private:
		bool updateValues();
	
		// Config values
		uint8_t ITIME; 			// Integration Time (datasheet page 9)
		float ITIME_ms;
		const uint8_t ITIME_max = 252;
		const uint8_t ITIME_min = 1;
		const uint16_t goUp = 32768; 	// On the high part
		float Tmt; 			// Measurement time (datasheet page 9)
		const float Tint = 2.8; 	// Internal Clock Period (datasheet page 4 --> 2.8 typ -- 4.0 max)
		uint8_t Gain = 1;

		float DATA0; 			// Visible Light
		float DATA1; 			// Infrared Light

		bool started = false;
		bool debug = false;
};
