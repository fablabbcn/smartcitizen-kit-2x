// #include "Sck_.h"
//
// bool Ctrl_myDevice::start(byte address)
// {
// 	if (!I2Cdetect(&auxWire, address)) return false;
// 	if (started) return true;
//
// 	devAddr = address;
//
// 	started = true;
// 	return true;
// }
//
// bool Ctrl_myDevice::stop()
// {
// 	started = false;
// 	return true;
// }
//
// int8_t Ctrl_myDevice::getReading(MetricType wichSensor, char *buffer) ;
// {
// 	float result = 0;
//
// 	switch(wichSensor)
// 	{
// 		case SENSOR_:
// 			
// 			result = 
// 			break;
//
// 		case SENSOR_:
// 			
// 			result = 
// 			break;
//
// 		default:
// 			return -1;
// 	}
//
// 	snprintf(buffer, sizeof(buffer), "%.*l", precision, result);
// 	return 0;
// }
//
