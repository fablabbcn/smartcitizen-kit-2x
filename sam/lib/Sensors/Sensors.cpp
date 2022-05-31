#include "Sensors.h"
#include "Utils.h"

MetricType AllSensors::getTypeFromString(String strIn)
{

	MetricType wichSensor = SENSOR_COUNT;
	uint8_t maxWordsFound = 0;

	// Get sensor type
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		MetricType thisSensor = static_cast<MetricType>(i);

		// Makes comparison lower case and not strict
		String titleCompare = list[thisSensor].title;
		titleCompare.toLowerCase();
		strIn.toLowerCase();

		// How many words match in Sensor title
		uint8_t matchedWords = countMatchedWords(titleCompare, strIn);

		if (matchedWords > maxWordsFound) {
			maxWordsFound = matchedWords;
			wichSensor = thisSensor;
		}

	}
	return wichSensor;
}
uint8_t AllSensors::countMatchedWords(String baseString, String input)
{
	
	uint8_t foundedCount = 0;
	String word;
	
	while (input.length() > 0) {

		// Get next word
		if (input.indexOf(" ") > -1) word = input.substring(0, input.indexOf(" "));
		// Or there is only one left
		else word = input;

		// If we found one
		if (baseString.indexOf(word) > -1) foundedCount += 1;
		// If next word is not part of the title we asume the rest of the input is a command or something else
		else break;

		// remove what we tested
		input.replace(word, "");
		input.trim();
	}

	return foundedCount;
}
String AllSensors::removeSensorName(String strIn)
{
	MetricType wichSensor = getTypeFromString(strIn);

	// Makes comparison lower case and not strict
	String titleCompare = list[wichSensor].title;
	titleCompare.toLowerCase();
	strIn.toLowerCase();

	uint8_t wordsToRemove = countMatchedWords(titleCompare, strIn);
	for (uint8_t i=0; i<wordsToRemove; i++) {
		if (strIn.indexOf(" ") > 0) strIn.remove(0, strIn.indexOf(" ")+1);
		else strIn.remove(0, strIn.length());
	}

	return strIn;
}
MetricType AllSensors::sensorsPriorized(uint8_t index)
{
	if (!sorted) {
		uint8_t sensorCount = 0;
		for (uint8_t i=0; i<251; i++) {
			for (uint8_t ii=0; ii<SENSOR_COUNT; ii++) {
				MetricType thisSensorType = static_cast<MetricType>(ii);
				if (list[thisSensorType].priority == i) {
					prioSortedList[sensorCount] = thisSensorType;
					sensorCount++;
				}
			}
		}
		sorted = true;
	}
	return prioSortedList[index];
}



bool I2Cdetect(TwoWire *_Wire, byte address)
{
	_Wire->beginTransmission(address);
	byte error = _Wire->endTransmission();

	if (error == 0) return true;
	else return false;
}


#include <Sck_SCD30.h>
#include <Sck_BH1730.h>
// #include <Sck_ADS1X15.h>
// #include "Sck_INA219.h"
// #include "Sck_VL6180.h"

Sensors::Sensors()
{

	// deviceCatalog[DEVICE_ADS1X15] = &sck_ads1x15;
	deviceCatalog[DEVICE_SCD30] = &sck_scd30;
	deviceCatalog[DEVICE_BH1730] = &sck_bh1730;
}
void Sensors::detect()
{
	// ---- Print msgs
	sckPrintfln("Detecting sensors...");

	uint8_t totalMetrics = 0;

	// Iterate over known I2C buses (defined on headers file)
	for (uint8_t bus=0; bus<2; bus++) {

		// ---- Print msgs
		sckPrintfln("\n%s I2C bus", buses[bus].name);

		// TCA9548 Multiplexer can be configured with 8 different I2C address from 0x70 to 0x77 (https://www.ti.com/lit/ds/symlink/tca9548a.pdf page 16)
		// I2C hubs in the same level as a multiplexer are not supported, you should use the multiplexer channels instead.
		for (int8_t mux=0; mux<8; mux++) {

			// TODO aqui hay un problema
			// cuando escaneo el mux -1 encuentro los sensores que esten en el channel 0... y luego los vuelvo a encontrar cuando escaneo los 8 canales
			uint8_t channels; 

			// On the first try (0) we scan the bus even if there is no multiplexer
			if (mux == 0) channels = 1;
			else channels = 0; 			// After one bus scan we only rescan if there is a multiplexer present

			// Search for multiplexers Address 0x70-0x77 (8 channels per multiplexer)
			if (I2Cdetect(buses[bus]._wire, mux + MUX_BASE_ADDRESS))  {
				channels = 8; 

				// ---- Print msgs
				sckPrintfln(" |-- Multiplexer 0x%X", mux + MUX_BASE_ADDRESS);
			}

			for (uint8_t chann=0; chann<channels; chann++) {

				if (mux >= 0) selectMuxChannel(buses[bus]._wire, mux + MUX_BASE_ADDRESS, chann);

				// Iterate over possible I2C addresses (0x00 - 0x07 and 0x78 - 0x7F are reserved I2C addresses)
				for (byte ad=0x08; ad<0x78; ad++) {

					if (I2Cdetect(buses[bus]._wire, ad)) {

						// Iterate over all known devices
						for (uint8_t dev=0; dev<DEVICE_COUNT; dev++) {

							// Iterate over the I2C address list of this device
							for(uint8_t devAd = 0; devAd < sizeof(deviceCatalog[dev]->addressList); devAd++) {

								// If this device has this address
								if (deviceCatalog[dev]->addressList[devAd] == ad) {

									// ---- Print msgs
									if (channels > 1) sckPrintfln(" |   |-- Channel %u", chann);
									
									// Create device
									if (newDevice(buses[bus]._wire, deviceCatalog[dev], ad))  {

										totalMetrics += deviceCatalog[dev]->providedNum;

										// ---- Print msgs
										if (channels > 1) sckPrintf(" |   |  ");
										sckPrintfln(" |-- %s 0x%x", deviceCatalog[dev]->name, ad);
										
										for (uint8_t m=0; m<deviceCatalog[dev]->providedNum; m++) {
											if (channels > 1) sckPrintf(" |   |  ");
											sckPrintfln("     |-- %s", deviceCatalog[dev]->providedList[m].name);
										}
										// ----

										// Set the mux and channel properties on the device
										if (channels > 1) {
											activeList[total-1]->muxAddr = mux + MUX_BASE_ADDRESS;
											activeList[total-1]->channel = chann;
										}

									}
								}
							}
						}
					}
				}
			}
		}
	}

	sckPrintfln("\nTotal active devices: %u (%i metrics)\n", total, totalMetrics);
}
void Sensors::selectMuxChannel(TwoWire * _wire, byte muxAddr, uint8_t channel)
{
	if (muxAddr == 0) return;
	if (channel > 7) return;

	_wire->beginTransmission(muxAddr);
	_wire->write(1 << channel);
	_wire->endTransmission();  
}
bool Sensors::newDevice(TwoWire * _wire, DeviceInfo *info, byte address)
{
	Device *_control;

	switch(info->type)
	{
		case DEVICE_BH1730:
			{
				_control = new Ctrl_BH1730();
				break;
			}
		// case DEVICE_ADS1X15: 
		// 	{ 
		// 		_control = new Ctrl_ADS1X15();
		// 		break;
		// 	}
		case DEVICE_SCD30:
			{
				_control = new Ctrl_SCD30();
				break;
			}
	}

	if (_control->start(_wire, address)) {
		_control->info = info;
		activeList[total] = _control;
		total++;
		return true;
	}

	return false;
}
int8_t Sensors::getReading(Device * dev, Metric metric)
{
	selectMuxChannel(dev->wireBus, dev->muxAddr, dev->channel);
	int8_t seconds = dev->getReading(metric, readingBuff);
	return seconds;
}
