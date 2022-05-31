#pragma once

#include <Sensors.h>
#include <Wire.h>

// Datasheet
// https://www.mouser.es/datasheet/2/348/bh1730fvc-e-1018573.pdf

static DeviceInfo sck_bh1730 = 
{ 
	DEVICE_BH1730, 				// DeviceType
	"BH1730",					// name
	{ 0x29 }, 					// I2C address list (8 max)
	1, 							// Number of provided metrics
	{{ 
		 SENSOR_LIGHT, 			// MetricType
		 "Light",				// name
		 "LIGHT", 				// short name 
		 "Lux", 				// unit
		 2,						// Precision (number of decimal digits)
		 14 					// platform id
	 }}
};

class Ctrl_BH1730 : public Device
{
	public:
		bool start(TwoWire * _wire, byte address);
		bool stop();
		int8_t getReading(Metric metric, char * buff);
		DeviceInfo * info = &sck_bh1730;

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
};
