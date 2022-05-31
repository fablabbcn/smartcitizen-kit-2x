// #include "Sck_VL6180.h"
//
// Sck_VL6180::Sck_VL6180()
// {
// 	providedList.add(SENSOR_RANGE_LIGHT);
// 	providedList.add(SENSOR_RANGE_DISTANCE );
// 	addressList.add(0x29);
// }
//
// bool Sck_VL6180::start(byte address)
// {
// 	if (!I2Cdetect(&auxWire, address)) return false;
// 	if (started) return true;
// 	
// 	_address = address;
//
// 	if(vl6180x.VL6180xInit() != 0) return false;
//
// 	vl6180x.VL6180xDefautSettings();
//
// 	started = true;
// 	return true;
// }
//
// bool Sck_VL6180::stop()
// {
// 	started = false;
// 	return true;
// }
//
// bool Sck_VL6180::getReading(SensorType wichSensor)
// {
// 	float result = 0;
//
// 	switch(wichSensor)
// 	{
// 		case SENSOR_RANGE_DISTANCE:
// 			
// 			result = vl6180x.getDistance();
// 			break;
//
// 		case SENSOR_RANGE_LIGHT:
// 			
// 			result = vl6180x.getAmbientLight(GAIN_1);
// 			break;
//
// 		default:
// 			return false;
// 	}
//
// 	readingList.add(wichSensor, result);
// 	return true;
// }
//
