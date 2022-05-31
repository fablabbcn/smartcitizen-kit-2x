// #pragma once
//
// #include <Sensors.h>
//
// // Sparkfun VL6180x time of flight range finder
// #include <SparkFun_VL6180X.h>
//
// extern TwoWire auxWire;
//
// class Sck_VL6180: public SensorDevice
// {
// 	public:
// 		Sck_VL6180();
// 		bool start(byte address);
// 		bool stop();
// 		bool getReading(SensorType wichSensor);
//
// 	private:
// 		byte _address;
// 		bool started = false;
//
// 		// TODO review this to allow non standard I2C address
// 		VL6180x vl6180x = VL6180x(0x29);
//
// 		// TODO implement the change address function (https://github.com/fablabbcn/SparkFun_ToF_Range_Finder-VL6180_Arduino_Library/blob/master/src/SparkFun_VL6180X.cpp#L126)
// };
