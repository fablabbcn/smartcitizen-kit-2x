// #pragma once
//
// #include <Sensors.h>
// #include <myExternalLib.h> // Only if needed
//
// extern TwoWire auxWire;
//
// static SensorInfo sck_myDevice = 
// { 
// 	SENSOR_TYPE, 				// SensorType
// 	"myDevice",					// name
// 	BOARD_AUX, 					// DeviceLocation 
// 	{ 0x00, 0x00 }, 					// I2C address list (8 max)
// 	2, 							// Number of provided metrics
// 	{{ 
// 		 SENSOR_WHATEVER, 		// MetricType
// 		 "My metric",			// title
// 		 "MyMet", 				// short title 
// 		 "myUnit", 				// unit
// 		 158 					// platform id
// 	 },{
// 		 SENSOR_TEMP,
// 		 "Temperature",
// 		 "TEMP",
// 		 "C",
// 		 35
// 	 }}
// };
//
// class Ctrl_myDevice: public Sensor
// {
// 	public:
// 		bool start(byte address);
// 		bool stop();
// 		int8_t getReading(MetricType wichSensor, char *buffer);
// 		SensorInfo *info = &sck_myDevice;
//
// 	private:
// 		bool started = false;
// };
