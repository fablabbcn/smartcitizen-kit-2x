#include "Sensors.h"
#include "Utils.h"

bool I2Cdetect(TwoWire *_Wire, byte address)
{
	_Wire->beginTransmission(address);
	byte error = _Wire->endTransmission();

	if (error == 0) return true;
	else return false;
}

// #include <Sck_SCD30.h>
#include <Sck_BH1730.h>
// #include <Sck_ADS1X15.h>
// #include "Sck_INA219.h"
// #include "Sck_VL6180.h"

Sensors::Sensors()
{
	// sensorCatalog[SENSOR_ADS1X15] = &info_ads1x15;
	// sensorCatalog[SENSOR_SCD30] = &info_scd30;
	sensorCatalog[SENSOR_BH1730] = &info_bh1730;
}
void Sensors::detect()
{
	sckPrintfln("Detecting sensors...");

	// Iterate over known I2C buses (defined on headers file)
	for (uint8_t busIndex=0; busIndex<i2cBuses_num; busIndex++) {

		sckPrintfln("\n%s I2C bus", i2cBuses[busIndex].name);

		// TCA9548 Multiplexer can be configured with 8 different I2C address from 0x70 to 0x77 (https://www.ti.com/lit/ds/symlink/tca9548a.pdf page 16)
		// Sensors or I2C hubs in the same level as a multiplexer are not supported, you should use the multiplexer channels instead.
		// Nested multiplexers are not supported
		for (uint8_t mux=0; mux<MUX_MAX_NUM; mux++) {

			byte muxAddress = mux + MUX_BASE_ADDRESS;
			
			// Check if a multiplexer is present on this address
			if (I2Cdetect(i2cBuses[busIndex]._wire, muxAddress)) {
				muxWasPresent = true;

				// Iterate over 8 mux channels
				for (uint8_t chann=0; chann<8; chann++) {
					
					selectMuxChannel(i2cBuses[busIndex]._wire, muxAddress, chann);
					scanBus(busIndex, muxAddress, chann);
				}

				// Disable all channels so we can use other muxes on the same bus
				selectMuxChannel(i2cBuses[busIndex]._wire, muxAddress, -1);
			}
		}

		if (!muxWasPresent) {
			scanBus(busIndex);
		}
	}

	sckPrintfln("\nTotal active sensors: %u (%u measurements)\n", total, getTotalMeasurNum());
}
void Sensors::selectMuxChannel(TwoWire * _wire, byte muxAddr, int8_t channel)
{
	if (muxAddr == 0) return;
	if (channel > 7) return;

	_wire->beginTransmission(muxAddr);
	if (channel >= 0) _wire->write(1 << channel);
	else _wire->write(0);
	_wire->endTransmission();  
}
void Sensors::scanBus(uint8_t busIndex, byte muxAddr, uint8_t chann)
{
	// Iterate over known sensors
	for (uint8_t sen=0; sen<SENSOR_COUNT; sen++) {

		// Iterate over 8 posible address  on sensors addressList
		for(uint8_t senAd=0; senAd<8; senAd++) {
			
			uint8_t thisAddress = sensorCatalog[sen]->addressList[senAd];

			// Check if the listed address is valid
			if (thisAddress > 0x08 && thisAddress < 0x78) {
				
				// Is the sensor present?
				if (I2Cdetect(i2cBuses[busIndex]._wire, thisAddress)) {

					// Try to create the sensor
					if (newSensor(i2cBuses[busIndex]._wire, sensorCatalog[sen], thisAddress)) {
						
						// ---- Print msgs
						if (muxAddr > 0) sckPrintf(" |   |  ");
						sckPrintfln(" |-- %s 0x%x", sensorCatalog[sen]->name, thisAddress);
										
						for (uint8_t m=0; m<sensorCatalog[sen]->measurementNum; m++) {
							if (muxAddr > 0) sckPrintf(" |   |  ");
							sckPrintfln("     |-- %s", sensorCatalog[sen]->measurementList[m]->name);
						}
						// ----

						// Set the mux and channel properties on the sensor
						if (muxAddr > 0) {
							activeList[total-1]->muxAddr = muxAddr;
							activeList[total-1]->channel = chann;
							activeList[total-1]->busIndex = busIndex;
						}
					}
				}
			}
		}
	}
}
bool Sensors::newSensor(TwoWire * _wire, SensorInfo *info, byte address)
{
	Sensor *_control;

	switch(info->id)
	{
		case SENSOR_BH1730:
			{
				_control = new Ctrl_BH1730();
				break;
			}
		// case SENSOR_ADS1X15:
		// 	{
		// 		_control = new Ctrl_ADS1X15(_wire);
		// 		break;
		// 	}
		// case SENSOR_SCD30:
		// 	{
		// 		_control = new Ctrl_SCD30();
		// 		break;
		// 	}
		default:
			return false;
			break;
	}

	if (_control->start(_wire, address)) {
		_control->info = info;
		_control->initConfig();
		activeList[total] = _control;
		total++;
		return true;
	}

	return false;
}
int8_t Sensors::getReading(Sensor * sensor, const Measurement * measurement)
{
	selectMuxChannel(sensor->wireBus, sensor->muxAddr, sensor->channel);
	int8_t seconds = sensor->getReading(measurement, readingBuff);
	return seconds;
}
uint8_t Sensors::getTotalMeasurNum()
{
	uint8_t totalM = 0;

	for (uint8_t i=0; i<total; i++) {
		totalM += activeList[i]->info->measurementNum;
	}

	return totalM;
}

void Sensor::initConfig()
{
	// TODO resolver como hago por que no sé el numero de measurements en el measurementConfig
 // 	SensorConfig _config = {
	// 	false,
	// 	{}
	// };

	for (uint8_t meas=0; meas<info->measurementNum; meas++) {

		Serial.println("A");
		// byte 0
		// bit 0:7   - 8 -> SensorID
		//
		// byte 1
		// bit 0:7  - 8 -> MeasurementID
		//
		// byte 2
		// |     7    |     6    |   5   |   4   |   3   |     2       |  1  |   0   |
	    // |----------|----------|-----------------------|-------------|-------------|
		// | Reserved | Reserved |     Mux addr diff     | Mux present | I2C bus idx |
		//
		// bit 0:1 - 2 -> I2C bus index
		// bit 2   - 1 -> Multiplexer present
		// bit 3:5 - 3 -> This + MUX_BASE_ADDRESS = mux i2c address
		// bit 6:7 - 2 -> Reserved
		//
		// byte 3
		// |     7    |     6    |   5   |   4   |   3   |   2   |  1   |   0   |
	    // |----------|----------|-----------------------|----------------------|
		// | Reserved | Reserved |  Sensor I2C addr idx  |     Mux channel      |
		//
		// bit 0:2 - 3 -> Multiplexer channel (0-7)
		// bit 3:5 - 3 -> Sensor I2C address index
		// bit 6:7 - 2 -> Reserved

		// config->measurements[meas].hash[0] = info->id;
		Serial.println("B");
		// config->measurements[meas].hash[1] = info->measurementList[meas]->id;
		Serial.println("C");
		byte two = ((muxAddr - MUX_BASE_ADDRESS) << 3);
		Serial.println("D");
		if (muxAddr > 0) two = two & (1 << 2);
		Serial.println("E");
		two = two & busIndex;

		byte three = 

		Serial.println(two, BIN);

		// TODO terminar la construccion del HASH
	}
}
// struct SensorConfig
// {
// 	// This has default values, but can be changed by the user
// 	bool debug = false;
// 	MeasurementConfig measurements[];
// };
// struct MeasurementConfig
// {
// 	// TODO decide if it worth the case encoding this, base64 (5 chars) or something smaller? or keep it on hex (8 chars)
// 	byte measurementHash[4];
// 								// bit 0:7   - 8 -> SensorID
// 								// bit 8:15  - 8 -> MeasurementID
// 								// bit 16:17 - 2 -> I2C bus index
// 								// bit 18 	 - 1 -> Multiplexer present
// 								// bit 19:21 - 3 -> This + MUX_BASE_ADDRESS = mux i2c address
// 								// bit 22:24 - 3 -> Multiplexer channel (0-7)
// 								// bit 25:27  - 3 -> Sensor I2C address index
// 	bool enabled = true;
// 	// uint8_t priority = 100;
// 	uint8_t intervals = 1;
// 	uint8_t precision = 2;
// 	// bool oled_display -> Move this to OLED Device (default show all and create a disable list to add manually disabled ones)
// };
