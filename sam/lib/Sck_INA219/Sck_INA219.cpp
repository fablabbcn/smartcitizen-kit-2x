// #include "Sck_INA219.h"
//
// Sck_INA219::Sck_INA219()
// {
// 	providedList.add(SENSOR_INA219_BUSVOLT);
// 	providedList.add(SENSOR_INA219_SHUNT);
// 	providedList.add(SENSOR_INA219_CURRENT);
// 	providedList.add(SENSOR_INA219_LOADVOLT );
// 	addressList.add(0x41);
// }
//
// bool Sck_INA219::start(byte address)
// {
// 	if (!I2Cdetect(&auxWire, address)) return false;
// 	if (started) return true;
//
// 	_address = address;
//
// 	// TODO check if we can change INA address on the fly
//
// 	ada_ina219.begin(&auxWire);
//
// 	// TODO implement change of range via the control interface
// 	// By default the initialization will use the largest range (32V, 2A).
// 	ada_ina219.setCalibration_32V_1A();
// 	// ada_ina219.setCalibration_16V_400mA();
// 	
// 	started = true;
//
// 	return true;
// }
//
// bool Sck_INA219::stop()
// {
// 	started = false;
// 	// TODO check if there is a way to minimize power consumption
// 	return true;
// }
//
// bool Sck_INA219::getReading(SensorType	wichSensor)
// {
// 	float result;
//
// 	switch(wichSensor) {
// 		case SENSOR_INA219_BUSVOLT: {
//
// 			result = ada_ina219.getBusVoltage_V();
// 			break;
//
// 		} case SENSOR_INA219_SHUNT: {
//
// 			result = ada_ina219.getShuntVoltage_mV();
// 			break;
//
// 		} case SENSOR_INA219_CURRENT: {
//
// 			result = ada_ina219.getCurrent_mA();
// 			break;
//
// 		} case SENSOR_INA219_LOADVOLT: {
//
// 			float busvoltage 	= ada_ina219.getBusVoltage_V();
// 			float shuntvoltage 	= ada_ina219.getShuntVoltage_mV();
//
// 			result = busvoltage + (shuntvoltage / 1000);
// 			break;
//
// 		} default: {
//
// 			return false;
// 		}
// 	}
// 	
// 	readingList.add(wichSensor, result);
// 	return true;
// }
