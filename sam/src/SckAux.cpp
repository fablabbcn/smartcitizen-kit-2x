#include "SckAux.h"

GasesBoard		gasBoard;
GrooveI2C_ADC		grooveI2C_ADC;
INA219			ina219;
Groove_OLED		groove_OLED;
WaterTemp_DS18B20 	waterTemp_DS18B20;
Atlas			atlasPH = Atlas(SENSOR_ATLAS_PH);
Atlas			atlasEC = Atlas(SENSOR_ATLAS_EC);
Atlas			atlasDO = Atlas(SENSOR_ATLAS_DO);
Atlas 			atlasTEMP = Atlas(SENSOR_ATLAS_TEMPERATURE);
Atlas 			atlasORP = Atlas(SENSOR_ATLAS_ORP);
Moisture 		moistureChirp;
PMsensor		pmSensorA = PMsensor(SLOT_A);
PMsensor		pmSensorB = PMsensor(SLOT_B);
PM_DallasTemp 		pmDallasTemp;
Sck_DallasTemp 		dallasTemp;
Sck_SHT31 		sht31 = Sck_SHT31(&auxWire);
Sck_SHT31 		sht35 = Sck_SHT31(&auxWire, 0x45);
Sck_Range 		range;
Sck_BME680 		bme680;
Sck_GPS 		gps;
PM_Grove_GPS 		pmGroveGps;
XA111GPS 		xa1110gps;
NEOM8UGPS 		neoM8uGps;
Sck_ADS1X15 		ads48;
Sck_ADS1X15 		ads49;
Sck_ADS1X15 		ads4A;
Sck_ADS1X15 		ads4B;
Sck_SCD30 		scd30;

// Eeprom flash emulation to store I2C address
FlashStorage(eepromAuxData, EepromAuxData);

bool AuxBoards::start(SckBase *base, SensorType wichSensor)
{
	if (!dataLoaded) {
		data = eepromAuxData.read();
		dataLoaded = true;

		if (data.calibration.moistureCalDataValid) {
			moistureChirp.dryPoint = data.calibration.dryPoint;
			moistureChirp.wetPoint = data.calibration.wetPoint;
			moistureChirp.calibrated = true;
		}
	}

	switch (wichSensor) {

		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W:
		case SENSOR_GASESBOARD_HUMIDITY:
		case SENSOR_GASESBOARD_TEMPERATURE: 	return gasBoard.start(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.start(); break;
		case SENSOR_INA219_BUSVOLT:
		case SENSOR_INA219_SHUNT:
		case SENSOR_INA219_CURRENT:
		case SENSOR_INA219_LOADVOLT: 		return ina219.start(); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.start(); break;
		case SENSOR_ATLAS_TEMPERATURE: 		return atlasTEMP.start(); break;
		case SENSOR_ATLAS_PH:			return atlasPH.start();
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_TDS:
		case SENSOR_ATLAS_EC_SAL:
		case SENSOR_ATLAS_EC_SG: 		return atlasEC.start(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 		return atlasDO.start(); break;
		case SENSOR_ATLAS_ORP: 			return atlasORP.start(); break;
		case SENSOR_CHIRP_MOISTURE_RAW:
		case SENSOR_CHIRP_MOISTURE:
		case SENSOR_CHIRP_TEMPERATURE:
		case SENSOR_CHIRP_LIGHT:		return moistureChirp.start(); break;
		case SENSOR_EXT_A_PM_1:
		case SENSOR_EXT_A_PM_25:
		case SENSOR_EXT_A_PM_10:
		case SENSOR_EXT_A_PN_03:
		case SENSOR_EXT_A_PN_05:
		case SENSOR_EXT_A_PN_1:
		case SENSOR_EXT_A_PN_25:
		case SENSOR_EXT_A_PN_5:
		case SENSOR_EXT_A_PN_10: 		return pmSensorA.start(); break;
		case SENSOR_EXT_B_PM_1:
		case SENSOR_EXT_B_PM_25:
		case SENSOR_EXT_B_PM_10:
		case SENSOR_EXT_B_PN_03:
		case SENSOR_EXT_B_PN_05:
		case SENSOR_EXT_B_PN_1:
		case SENSOR_EXT_B_PN_25:
		case SENSOR_EXT_B_PN_5:
		case SENSOR_EXT_B_PN_10: 		return pmSensorB.start(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.start(); break;
		case SENSOR_DALLAS_TEMP: 		return dallasTemp.start(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM:
			if (sht31.start() && !gasBoard.start()) return true;
			else return false;
			break;
		case SENSOR_SHT35_TEMP:
		case SENSOR_SHT35_HUM: 			return sht35.start(); break;
		case SENSOR_RANGE_DISTANCE: 		return range.start(); break;
		case SENSOR_RANGE_LIGHT: 		return range.start(); break;
		case SENSOR_BME680_TEMPERATURE:		return bme680.start(); break;
		case SENSOR_BME680_HUMIDITY:		return bme680.start(); break;
		case SENSOR_BME680_PRESSURE:		return bme680.start(); break;
		case SENSOR_BME680_VOCS:		return bme680.start(); break;
		case SENSOR_GPS_FIX_QUALITY:
		case SENSOR_GPS_LATITUDE:
		case SENSOR_GPS_LONGITUDE:
		case SENSOR_GPS_ALTITUDE:
		case SENSOR_GPS_SPEED:
		case SENSOR_GPS_HDOP:
		case SENSOR_GPS_SATNUM:			return gps.start(); break;
		case SENSOR_ADS1X15_48_0:
		case SENSOR_ADS1X15_48_1:
		case SENSOR_ADS1X15_48_2:
		case SENSOR_ADS1X15_48_3: 		return ads48.start(0x48); break;
		case SENSOR_ADS1X15_49_0:
		case SENSOR_ADS1X15_49_1:
		case SENSOR_ADS1X15_49_2:
		case SENSOR_ADS1X15_49_3: 		return ads49.start(0x49); break;
		case SENSOR_ADS1X15_4A_0:
		case SENSOR_ADS1X15_4A_1:
		case SENSOR_ADS1X15_4A_2:
		case SENSOR_ADS1X15_4A_3: 		return ads4A.start(0x4A); break;
		case SENSOR_ADS1X15_4B_0:
		case SENSOR_ADS1X15_4B_1:
		case SENSOR_ADS1X15_4B_2:
		case SENSOR_ADS1X15_4B_3: 		return ads4B.start(0x4B); break;
		case SENSOR_SCD30_CO2: 			return scd30.start(base, SENSOR_SCD30_CO2); break;
		case SENSOR_SCD30_TEMP: 		return scd30.start(base, SENSOR_SCD30_TEMP); break;
		case SENSOR_SCD30_HUM: 			return scd30.start(base, SENSOR_SCD30_HUM); break;
		case SENSOR_GROVE_OLED: 		return groove_OLED.start(); break;
		default: break;
	}

	return false;
}

bool AuxBoards::stop(SensorType wichSensor)
{
	switch (wichSensor) {

		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W:
		case SENSOR_GASESBOARD_HUMIDITY:
		case SENSOR_GASESBOARD_TEMPERATURE: 	return gasBoard.stop(); break;
		case SENSOR_GROOVE_I2C_ADC: 		return grooveI2C_ADC.stop(); break;
		case SENSOR_INA219_BUSVOLT:
		case SENSOR_INA219_SHUNT:
		case SENSOR_INA219_CURRENT:
		case SENSOR_INA219_LOADVOLT: 		return ina219.stop(); break;
		case SENSOR_WATER_TEMP_DS18B20:		return waterTemp_DS18B20.stop(); break;
		case SENSOR_ATLAS_TEMPERATURE: 		return atlasTEMP.stop(); break;
		case SENSOR_ATLAS_PH:			return atlasPH.stop();
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_TDS:
		case SENSOR_ATLAS_EC_SAL:
		case SENSOR_ATLAS_EC_SG: 		return atlasEC.stop(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 		return atlasDO.stop(); break;
		case SENSOR_ATLAS_ORP: 		return atlasORP.stop(); break;
		case SENSOR_CHIRP_TEMPERATURE:
		case SENSOR_CHIRP_MOISTURE:		return moistureChirp.stop(); break;
		case SENSOR_EXT_A_PM_1:
		case SENSOR_EXT_A_PM_25:
		case SENSOR_EXT_A_PM_10:
		case SENSOR_EXT_A_PN_03:
		case SENSOR_EXT_A_PN_05:
		case SENSOR_EXT_A_PN_1:
		case SENSOR_EXT_A_PN_25:
		case SENSOR_EXT_A_PN_5:
		case SENSOR_EXT_A_PN_10: 		return pmSensorA.stop(); break;
		case SENSOR_EXT_B_PM_1:
		case SENSOR_EXT_B_PM_25:
		case SENSOR_EXT_B_PM_10:
		case SENSOR_EXT_B_PN_03:
		case SENSOR_EXT_B_PN_05:
		case SENSOR_EXT_B_PN_1:
		case SENSOR_EXT_B_PN_25:
		case SENSOR_EXT_B_PN_5:
		case SENSOR_EXT_B_PN_10: 		return pmSensorB.stop(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.stop(); break;
		case SENSOR_DALLAS_TEMP: 		return dallasTemp.stop(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM: 			return sht31.stop(); break;
		case SENSOR_SHT35_TEMP:
		case SENSOR_SHT35_HUM: 			return sht35.stop(); break;
		case SENSOR_RANGE_DISTANCE: 		return range.stop(); break;
		case SENSOR_RANGE_LIGHT: 		return range.stop(); break;
		case SENSOR_BME680_TEMPERATURE:		return bme680.stop(); break;
		case SENSOR_BME680_HUMIDITY:		return bme680.stop(); break;
		case SENSOR_BME680_PRESSURE:		return bme680.stop(); break;
		case SENSOR_BME680_VOCS:		return bme680.stop(); break;
		case SENSOR_GPS_FIX_QUALITY:
		case SENSOR_GPS_LATITUDE:
		case SENSOR_GPS_LONGITUDE:
		case SENSOR_GPS_ALTITUDE:
		case SENSOR_GPS_SPEED:
		case SENSOR_GPS_HDOP:
		case SENSOR_GPS_SATNUM:			return gps.stop(); break;
		case SENSOR_ADS1X15_48_0:
		case SENSOR_ADS1X15_48_1:
		case SENSOR_ADS1X15_48_2:
		case SENSOR_ADS1X15_48_3: 		return ads48.stop(); break;
		case SENSOR_ADS1X15_49_0:
		case SENSOR_ADS1X15_49_1:
		case SENSOR_ADS1X15_49_2:
		case SENSOR_ADS1X15_49_3: 		return ads49.stop(); break;
		case SENSOR_ADS1X15_4A_0:
		case SENSOR_ADS1X15_4A_1:
		case SENSOR_ADS1X15_4A_2:
		case SENSOR_ADS1X15_4A_3: 		return ads4A.stop(); break;
		case SENSOR_ADS1X15_4B_0:
		case SENSOR_ADS1X15_4B_1:
		case SENSOR_ADS1X15_4B_2:
		case SENSOR_ADS1X15_4B_3: 		return ads4B.stop(); break;
		case SENSOR_SCD30_CO2: 			return scd30.stop(SENSOR_SCD30_CO2); break;
		case SENSOR_SCD30_TEMP: 		return scd30.stop(SENSOR_SCD30_TEMP); break;
		case SENSOR_SCD30_HUM: 			return scd30.stop(SENSOR_SCD30_HUM); break;
		case SENSOR_GROVE_OLED: 		return groove_OLED.stop(); break;
		default: break;
	}

	return false;
}

void AuxBoards::getReading(SckBase *base, OneSensor *wichSensor)
{
	wichSensor->state = 0;
	switch (wichSensor->type) {
		case SENSOR_GASESBOARD_SLOT_1A:	 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot1.electrode_A)); return;
		case SENSOR_GASESBOARD_SLOT_1W: 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot1.electrode_W)); return;
		case SENSOR_GASESBOARD_SLOT_2A: 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot2.electrode_A)); return;
		case SENSOR_GASESBOARD_SLOT_2W: 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot2.electrode_W)); return;
		case SENSOR_GASESBOARD_SLOT_3A: 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot3.electrode_A)); return;
		case SENSOR_GASESBOARD_SLOT_3W: 	wichSensor->reading = String(gasBoard.getElectrode(gasBoard.Slot3.electrode_W)); return;
		case SENSOR_GASESBOARD_HUMIDITY: 	wichSensor->reading = String(gasBoard.getHumidity()); return;
		case SENSOR_GASESBOARD_TEMPERATURE: 	wichSensor->reading = String(gasBoard.getTemperature()); return;
		case SENSOR_GROOVE_I2C_ADC: 		wichSensor->reading = String(grooveI2C_ADC.getReading()); return;
		case SENSOR_INA219_BUSVOLT: 		wichSensor->reading = String(ina219.getReading(ina219.BUS_VOLT)); return;
		case SENSOR_INA219_SHUNT: 		wichSensor->reading = String(ina219.getReading(ina219.SHUNT_VOLT)); return;
		case SENSOR_INA219_CURRENT: 		wichSensor->reading = String(ina219.getReading(ina219.CURRENT)); return;
		case SENSOR_INA219_LOADVOLT: 		wichSensor->reading = String(ina219.getReading(ina219.LOAD_VOLT)); return;
		case SENSOR_WATER_TEMP_DS18B20:		wichSensor->reading = String(waterTemp_DS18B20.getReading()); return;
		case SENSOR_ATLAS_TEMPERATURE: 		if (atlasTEMP.getReading()) 	{ wichSensor->reading = String(atlasTEMP.newReading[0]); return; } break;
		case SENSOR_ATLAS_PH:			if (atlasPH.getReading()) 	{ wichSensor->reading = String(atlasPH.newReading[0]); return; } break;
		case SENSOR_ATLAS_EC:			if (atlasEC.getReading()) 	{ wichSensor->reading = String(atlasEC.newReading[0]); return; } break;
		case SENSOR_ATLAS_EC_TDS:		if (atlasEC.getReading()) 	{ wichSensor->reading = String(atlasEC.newReading[1]); return; } break;
		case SENSOR_ATLAS_EC_SAL:		if (atlasEC.getReading()) 	{ wichSensor->reading = String(atlasEC.newReading[2]); return; } break;
		case SENSOR_ATLAS_EC_SG:		if (atlasEC.getReading()) 	{ wichSensor->reading = String(atlasEC.newReading[3]); return; } break;
		case SENSOR_ATLAS_DO:			if (atlasDO.getReading()) 	{ wichSensor->reading = String(atlasDO.newReading[0]); return; } break;
		case SENSOR_ATLAS_DO_SAT:		if (atlasDO.getReading()) 	{ wichSensor->reading = String(atlasDO.newReading[1]); return; } break;
		case SENSOR_ATLAS_ORP:		if (atlasORP.getReading()) 	{ wichSensor->reading = String(atlasORP.newReading[0]); return; } break;
		case SENSOR_CHIRP_MOISTURE_RAW:		if (moistureChirp.getReading(SENSOR_CHIRP_MOISTURE_RAW)) { wichSensor->reading = String(moistureChirp.raw); return; } break;
		case SENSOR_CHIRP_MOISTURE:		if (moistureChirp.getReading(SENSOR_CHIRP_MOISTURE)) { wichSensor->reading = String(moistureChirp.moisture); return; } break;
		case SENSOR_CHIRP_TEMPERATURE:		if (moistureChirp.getReading(SENSOR_CHIRP_TEMPERATURE)) { wichSensor->reading = String(moistureChirp.temperature); return; } break;
		case SENSOR_CHIRP_LIGHT:		if (moistureChirp.getReading(SENSOR_CHIRP_LIGHT)) { wichSensor->reading = String(moistureChirp.light); return; } break;
		case SENSOR_EXT_A_PM_1: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pm1); return; } break;
		case SENSOR_EXT_A_PM_25: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pm25); return; } break;
		case SENSOR_EXT_A_PM_10: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pm10); return; } break;
		case SENSOR_EXT_A_PN_03: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn03); return; } break;
		case SENSOR_EXT_A_PN_05: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn05); return; } break;
		case SENSOR_EXT_A_PN_1: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn1); return; } break;
		case SENSOR_EXT_A_PN_25: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn25); return; } break;
		case SENSOR_EXT_A_PN_5: 		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn5); return; } break;
		case SENSOR_EXT_A_PN_10:		if (pmSensorA.update()) { wichSensor->reading = String(pmSensorA.pn10); return; } break;
		case SENSOR_EXT_B_PM_1: 		if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pm1); return; } break;
		case SENSOR_EXT_B_PM_25:                if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pm25); return; } break;
		case SENSOR_EXT_B_PM_10:                if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pm10); return; } break;
		case SENSOR_EXT_B_PN_03:                if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn03); return; } break;
		case SENSOR_EXT_B_PN_05:                if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn05); return; } break;
		case SENSOR_EXT_B_PN_1:                 if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn1); return; } break;
		case SENSOR_EXT_B_PN_25:                if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn25); return; } break;
		case SENSOR_EXT_B_PN_5:                 if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn5); return; } break;
		case SENSOR_EXT_B_PN_10: 		if (pmSensorB.update()) { wichSensor->reading = String(pmSensorB.pn10); return; } break;
		case SENSOR_PM_DALLAS_TEMP: 		wichSensor->reading = String(pmDallasTemp.getReading()); return;
		case SENSOR_DALLAS_TEMP: 		if (dallasTemp.getReading()) 			{ wichSensor->reading = String(dallasTemp.reading); return; } break;
		case SENSOR_SHT31_TEMP: 		if (sht31.getReading()) 				{ wichSensor->reading = String(sht31.temperature); return; } break;
		case SENSOR_SHT31_HUM: 			if (sht31.getReading()) 				{ wichSensor->reading = String(sht31.humidity); return; } break;
		case SENSOR_SHT35_TEMP: 		if (sht35.getReading()) 				{ wichSensor->reading = String(sht35.temperature); return; } break;
		case SENSOR_SHT35_HUM: 			if (sht35.getReading()) 				{ wichSensor->reading = String(sht35.humidity); return; } break;
		case SENSOR_RANGE_DISTANCE: 		if (range.getReading(SENSOR_RANGE_DISTANCE)) 	{ wichSensor->reading = String(range.readingDistance); return; } break;
		case SENSOR_RANGE_LIGHT: 		if (range.getReading(SENSOR_RANGE_LIGHT)) 	{ wichSensor->reading = String(range.readingLight); return; } break;
		case SENSOR_BME680_TEMPERATURE:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.temperature); return; } break;
		case SENSOR_BME680_HUMIDITY:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.humidity); return; } break;
		case SENSOR_BME680_PRESSURE:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.pressure); return; } break;
		case SENSOR_BME680_VOCS:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.VOCgas); return; } break;
		case SENSOR_GPS_FIX_QUALITY: 		if (gps.getReading(base, SENSOR_GPS_FIX_QUALITY)) 	{ wichSensor->reading = String(gps.r.fixQuality); return; } break;
		case SENSOR_GPS_LATITUDE: 		if (gps.getReading(base, SENSOR_GPS_LATITUDE)) 		{ wichSensor->reading = String(gps.r.latitude, 6); return; } break;
		case SENSOR_GPS_LONGITUDE: 		if (gps.getReading(base, SENSOR_GPS_LONGITUDE)) 	{ wichSensor->reading = String(gps.r.longitude, 6); return; } break;
		case SENSOR_GPS_ALTITUDE: 		if (gps.getReading(base, SENSOR_GPS_ALTITUDE)) 		{ wichSensor->reading = String(gps.r.altitude, 2); return; } break;
		case SENSOR_GPS_SPEED: 			if (gps.getReading(base, SENSOR_GPS_SPEED)) 		{ wichSensor->reading = String(gps.r.speed, 2); return; } break;
		case SENSOR_GPS_HDOP: 			if (gps.getReading(base, SENSOR_GPS_HDOP)) 		{ wichSensor->reading = String(gps.r.hdop, 2); return; } break;
		case SENSOR_GPS_SATNUM:			if (gps.getReading(base, SENSOR_GPS_SATNUM)) 		{ wichSensor->reading = String(gps.r.satellites); return; } break;
		case SENSOR_ADS1X15_48_0: 		if (ads48.getReading(0)) 			{ wichSensor->reading = String(ads48.reading, 6); return;} break;
		case SENSOR_ADS1X15_48_1: 		if (ads48.getReading(1)) 			{ wichSensor->reading = String(ads48.reading, 6); return;} break;
		case SENSOR_ADS1X15_48_2: 		if (ads48.getReading(2)) 			{ wichSensor->reading = String(ads48.reading, 6); return;} break;
		case SENSOR_ADS1X15_48_3: 		if (ads48.getReading(3)) 			{ wichSensor->reading = String(ads48.reading, 6); return;} break;
		case SENSOR_ADS1X15_49_0: 		if (ads49.getReading(0)) 			{ wichSensor->reading = String(ads49.reading, 6); return;} break;
		case SENSOR_ADS1X15_49_1: 		if (ads49.getReading(1)) 			{ wichSensor->reading = String(ads49.reading, 6); return;} break;
		case SENSOR_ADS1X15_49_2: 		if (ads49.getReading(2)) 			{ wichSensor->reading = String(ads49.reading, 6); return;} break;
		case SENSOR_ADS1X15_49_3: 		if (ads49.getReading(3)) 			{ wichSensor->reading = String(ads49.reading, 6); return;} break;
		case SENSOR_ADS1X15_4A_0: 		if (ads4A.getReading(0)) 			{ wichSensor->reading = String(ads4A.reading, 6); return;} break;
		case SENSOR_ADS1X15_4A_1: 		if (ads4A.getReading(1)) 			{ wichSensor->reading = String(ads4A.reading, 6); return;} break;
		case SENSOR_ADS1X15_4A_2: 		if (ads4A.getReading(2)) 			{ wichSensor->reading = String(ads4A.reading, 6); return;} break;
		case SENSOR_ADS1X15_4A_3: 		if (ads4A.getReading(3)) 			{ wichSensor->reading = String(ads4A.reading, 6); return;} break;
		case SENSOR_ADS1X15_4B_0: 		if (ads4B.getReading(0)) 			{ wichSensor->reading = String(ads4B.reading, 6); return;} break;
		case SENSOR_ADS1X15_4B_1: 		if (ads4B.getReading(1)) 			{ wichSensor->reading = String(ads4B.reading, 6); return;} break;
		case SENSOR_ADS1X15_4B_2: 		if (ads4B.getReading(2)) 			{ wichSensor->reading = String(ads4B.reading, 6); return;} break;
		case SENSOR_ADS1X15_4B_3: 		if (ads4B.getReading(3)) 			{ wichSensor->reading = String(ads4B.reading, 6); return;} break;
		case SENSOR_SCD30_CO2: 			if (scd30.getReading(SENSOR_SCD30_CO2)) { wichSensor->reading = String(scd30.co2); return; } break;
		case SENSOR_SCD30_TEMP: 		if (scd30.getReading(SENSOR_SCD30_TEMP)) { wichSensor->reading = String(scd30.temperature); return; } break;
		case SENSOR_SCD30_HUM: 			if (scd30.getReading(SENSOR_SCD30_HUM)) { wichSensor->reading = String(scd30.humidity); return; } break;
		default: break;
	}

	wichSensor->reading = "null";
	wichSensor->state = -1;
}

bool AuxBoards::getBusyState(SensorType wichSensor)
{

	switch(wichSensor) {
		case SENSOR_ATLAS_TEMPERATURE:  return atlasTEMP.getBusyState(); break;
		case SENSOR_ATLAS_PH: 		return atlasPH.getBusyState(); break;
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_TDS:
		case SENSOR_ATLAS_EC_SAL:
		case SENSOR_ATLAS_EC_SG: 	return atlasEC.getBusyState(); break;
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 	return atlasDO.getBusyState(); break;
		case SENSOR_ATLAS_ORP: 	return atlasORP.getBusyState(); break;
		default: return false; break;
	}
}

String AuxBoards::control(SensorType wichSensor, String command)
{
	switch(wichSensor) {
		case SENSOR_GASESBOARD_SLOT_1A:
		case SENSOR_GASESBOARD_SLOT_1W:
		case SENSOR_GASESBOARD_SLOT_2A:
		case SENSOR_GASESBOARD_SLOT_2W:
		case SENSOR_GASESBOARD_SLOT_3A:
		case SENSOR_GASESBOARD_SLOT_3W: {

			if (command.startsWith("set pot")) {

				Electrode wichElectrode;

				switch(wichSensor) {
					case SENSOR_GASESBOARD_SLOT_1A: wichElectrode = gasBoard.Slot1.electrode_A;
					case SENSOR_GASESBOARD_SLOT_1W: wichElectrode = gasBoard.Slot1.electrode_W;
					case SENSOR_GASESBOARD_SLOT_2A: wichElectrode = gasBoard.Slot2.electrode_A;
					case SENSOR_GASESBOARD_SLOT_2W: wichElectrode = gasBoard.Slot2.electrode_W;
					case SENSOR_GASESBOARD_SLOT_3A: wichElectrode = gasBoard.Slot3.electrode_A;
					case SENSOR_GASESBOARD_SLOT_3W: wichElectrode = gasBoard.Slot3.electrode_W;
					default: break;
				}

				command.replace("set pot", "");
				command.trim();
				int wichValue = command.toInt();
				gasBoard.setPot(wichElectrode, wichValue);
				return String F("Setting pot to: ") + String(wichValue) + F(" Ohms\n\rActual value: ") + String(gasBoard.getPot(wichElectrode)) + F(" Ohms");

			#ifdef gasesBoardTest
			} else if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get slot
				String slotSTR = String(command.charAt(0));
				uint8_t wichSlot = slotSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					gasBoard.setTesterCurrent(wichValue, wichSlot);

				} else if (command.startsWith("full")) {

					gasBoard.runTester(wichSlot);

				} else {
					return F("Unrecognized test command!!\r\nOptions:\r\ntest slot set value (slot: 1-3, value:-1400/+1400 nA)\r\ntest slot full (test the full cycle on slot (1-3))");
				}

				return F("\nTesting finished!");

			} else if (command.startsWith("autotest")) {

				return String(gasBoard.autoTest());
			#endif

			} else if (command.startsWith("help")) {
				return F("Available commands for this sensor:\n\r* set pot ");

			} else {
				return F("Unrecognized command!! please try again...");
			}

			break;

		} case SENSOR_ATLAS_PH:
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_TDS:
		case SENSOR_ATLAS_EC_SAL:
		case SENSOR_ATLAS_EC_SG:
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: 
		case SENSOR_ATLAS_ORP:
		case SENSOR_ATLAS_TEMPERATURE: {

			Atlas *thisAtlas = &atlasPH;
			if (wichSensor == SENSOR_ATLAS_EC || wichSensor == SENSOR_ATLAS_EC_SG || wichSensor == SENSOR_ATLAS_EC_TDS || wichSensor == SENSOR_ATLAS_EC_SAL) thisAtlas = &atlasEC;
			else if (wichSensor == SENSOR_ATLAS_DO || wichSensor == SENSOR_ATLAS_DO_SAT) thisAtlas = &atlasDO;
			else if (wichSensor == SENSOR_ATLAS_ORP) thisAtlas = &atlasORP;
			else if (wichSensor == SENSOR_ATLAS_TEMPERATURE) thisAtlas = &atlasTEMP;

			// 	 Calibration command options:
			// 		Atlas PH: (https://www.atlas-scientific.com/files/pH_EZO_Datasheet.pdf) page 52
			// 			* com cal,mid,7
			// 			* com cal,low,4
			// 			* com cal,high,10
			// 			* com cal,clear
			// 			* com cal,?
			// 		Atlas EC: (https://www.atlas-scientific.com/_files/_datasheets/_circuit/EC_EZO_Datasheet.pdf) page 55
			// 			* com cal,dry
			// 			* com cal,low,12880
			// 			* com cal,high,80000
			// 			* com cal,clear
			// 			* com cal,?
			// 		Atlas DO: (https://www.atlas-scientific.com/_files/_datasheets/_circuit/DO_EZO_Datasheet.pdf) page 52
			// 			* com cal
			// 			* com cal,0
			// 			* com cal,clear
			// 			* com cal,?
			//		Atlas ORP: (https://files.atlas-scientific.com/ORP_EZO_Datasheet.pdf) page x49
			//			* com cal,225 	-> calibrates the ORP circuit to a set value
			//			* com cal,clear -> delete calibration data
			//			* com cal,? 	-> device calibrated?
			//		Atlas TEMP: (https://files.atlas-scientific.com/https://files.atlas-scientific.com/EZO_RTD_Datasheet.pdf) page x53
			//			* com cal,25 	-> calibrates the TEMP circuit to a set value
			//			* com cal,clear -> delete calibration data
			//			* com cal,? 	-> device calibrated?
			if (command.startsWith("com")) {

				command.replace("com", "");
				command.trim();
				thisAtlas->sendCommand((char*)command.c_str());

				uint8_t responseCode = thisAtlas->getResponse();
				if (responseCode == 254) {
					delay(1000);
					responseCode = thisAtlas->getResponse();
				}
				if (responseCode == 1) return thisAtlas->atlasResponse;
				else return String(responseCode);

			}
			break;

		} case SENSOR_CHIRP_MOISTURE_RAW:
		case SENSOR_CHIRP_MOISTURE:
		case SENSOR_CHIRP_TEMPERATURE:
		case SENSOR_CHIRP_LIGHT: {

			if (command.startsWith("get ver")) {

				return String(moistureChirp.getVersion());

			} else if (command.startsWith("reset")) {

				for(uint8_t address = 1; address < 127; address++ ) {

					auxWire.beginTransmission(address);

					if (auxWire.endTransmission() == 0) {
						if (moistureChirp.resetAddress(address)) return F("Changed chirp address to default 0x20");
					}
				}
				return F("Can't find any chirp sensor...");

			} else if (command.startsWith("cal")) {

				command.replace("cal", "");
				command.trim();

				int space_index = command.indexOf(" ");

				if (space_index > 0) {

					String preDry = command.substring(0, space_index);
					String preWet = command.substring(space_index + 1);

					int32_t dryInt = preDry.toInt();
					int32_t wetInt = preWet.toInt();

					if ((dryInt == 0 && preDry != "0") || (wetInt == 0 && preWet != "0")) return F("Error reading values, please try again!");

					moistureChirp.dryPoint = data.calibration.dryPoint = dryInt;
					moistureChirp.wetPoint = data.calibration.wetPoint = wetInt;
					data.calibration.moistureCalDataValid = true;
					moistureChirp.calibrated = true;

					data.valid = true;
					eepromAuxData.write(data);
				}

				String response;
				if (moistureChirp.calibrated) {
					response += "Dry point: " + String(moistureChirp.dryPoint) + "\r\nWet point: " + String(moistureChirp.wetPoint);
				} else {
					response = F("Moisture sensor is NOT calibrated");
				}
				return response;


			} else if (command.startsWith("help") || command.length() == 0) return F("Available commands:\r\n* get ver\r\n* reset (connect only the chirp to aux)\r\n* cal dryPoint wetPoint");
			else return F("Unrecognized command!! please try again...");
			break;

		} case SENSOR_ADS1X15_48_0:
		case SENSOR_ADS1X15_48_1:
		case SENSOR_ADS1X15_48_2:
		case SENSOR_ADS1X15_48_3: {
#ifdef adsTest
			if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get channels
				String channelSTR = String(command.charAt(0));
				uint8_t wichChannel = channelSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					ads48.setTesterCurrent(wichValue, wichChannel);

				} else if (command.startsWith("full")) {

					ads48.runTester(wichChannel);

				} else {

					return F("Unrecognized test command!!\r\nOptions:\r\ntest set value (-1400/+1400 nA)\r\ntest slot full (test the full cycle)");
				}

				return F("\nCurrent set!");
			}
#endif
			break;
		} case SENSOR_ADS1X15_49_0:
		case SENSOR_ADS1X15_49_1:
		case SENSOR_ADS1X15_49_2:
		case SENSOR_ADS1X15_49_3: {
#ifdef adsTest
			if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get channels
				String channelSTR = String(command.charAt(0));
				uint8_t wichChannel = channelSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					ads49.setTesterCurrent(wichValue, wichChannel);

				} else if (command.startsWith("full")) {

					ads49.runTester(wichChannel);

				} else {

					return F("Unrecognized test command!!\r\nOptions:\r\ntest set value (-1400/+1400 nA)\r\ntest slot full (test the full cycle)");
				}

				return F("\nCurrent set!");
			}
#endif
			break;
		} case SENSOR_ADS1X15_4A_0:
		case SENSOR_ADS1X15_4A_1:
		case SENSOR_ADS1X15_4A_2:
		case SENSOR_ADS1X15_4A_3: {
#ifdef adsTest
			if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get channels
				String channelSTR = String(command.charAt(0));
				uint8_t wichChannel = channelSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					ads4A.setTesterCurrent(wichValue, wichChannel);

				} else if (command.startsWith("full")) {

					ads4A.runTester(wichChannel);

				} else {

					return F("Unrecognized test command!!\r\nOptions:\r\ntest set value (-1400/+1400 nA)\r\ntest slot full (test the full cycle)");
				}

				return F("\nCurrent set!");
			}
#endif
			break;
		} case SENSOR_ADS1X15_4B_0:
		case SENSOR_ADS1X15_4B_1:
		case SENSOR_ADS1X15_4B_2:
		case SENSOR_ADS1X15_4B_3: {
#ifdef adsTest
			if (command.startsWith("test")) {
				command.replace("test", "");
				command.trim();

				// Get channels
				String channelSTR = String(command.charAt(0));
				uint8_t wichChannel = channelSTR.toInt();

				command.remove(0,1);
				command.trim();

				if (command.startsWith("set")) {

					command.replace("set", "");
					command.trim();

					// Get value
					int wichValue = command.toInt();
					ads4B.setTesterCurrent(wichValue, wichChannel);

				} else if (command.startsWith("full")) {

					ads4B.runTester(wichChannel);

				} else {

					return F("Unrecognized test command!!\r\nOptions:\r\ntest set value (-1400/+1400 nA)\r\ntest slot full (test the full cycle)");
				}

				return F("\nCurrent set!");
			}
#endif
			break;
		}
		case SENSOR_SCD30_CO2:
		case SENSOR_SCD30_TEMP:
		case SENSOR_SCD30_HUM: {

			if (command.startsWith("interval")) {

				command.replace("interval", "");
				command.trim();

				uint16_t newInterval = command.toInt();
				scd30.interval(newInterval);

				return String F("Measuring Interval: ") + String(scd30.interval());

			} else if (command.startsWith("autocal")) {

				command.replace("autocal", "");
				command.trim();

				if (command.startsWith("on")) scd30.autoSelfCal(1);
				else if (command.startsWith("off")) scd30.autoSelfCal(0);

				return String F("Auto Self Calibration: ") + String(scd30.autoSelfCal() ? "on" : "off");

			} else if (command.startsWith("calfactor")) {

				command.replace("calfactor", "");
				command.trim();

				uint16_t newFactor = command.toInt();

				return String F("Forced Recalibration Factor: ") + String(scd30.forcedRecalFactor(newFactor));

			} else if (command.startsWith("caltemp")) {

				command.replace("caltemp", "");
				command.trim();

				float userTemp;
				bool off = false;

				if (command.startsWith("off")) off = true;
				else {

					if (command.length() > 0 && isDigit(command.charAt(0))) userTemp = command.toFloat();
					else return F("Wrong temperature value, try again.");
				}

				scd30.getReading(SENSOR_SCD30_TEMP);

				return String F("Current temperature: ") + String(scd30.temperature) + F(" C") + F("\r\nTemperature offset: ") + String(scd30.tempOffset(userTemp, off)) + F(" C");
			} else if (command.startsWith("pressure")) {

				return String F("Pressure compensation on last boot: ") + String(scd30.pressureCompensated ? "True" : "False");

			} else {
				return F("Wrong command!!\r\nOptions:\r\ninterval [2-1000 (seconds)]\r\nautocal [on/off]\r\ncalfactor [400-2000 (ppm)]\r\ncaltemp [newTemp/off]\r\npressure");
			}


		} default: return "Unrecognized sensor!!!"; break;
	}
	return "Unknown error on control command!!!";
}

void AuxBoards::print(char *payload)
{

	groove_OLED.print(payload);
}

void AuxBoards::updateDisplay(SckBase* base, bool force)
{
	groove_OLED.update(base, force);
}

bool AuxBoards::updateGPS()
{
	return gps.update();
}

void AuxBoards::plot(String value, const char *title, const char *unit)
{
	if (title != NULL && unit != NULL) groove_OLED.plot(value, title, unit);
	else groove_OLED.plot(value);
}

bool GrooveI2C_ADC::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);		// transmit to device
	auxWire.write(REG_ADDR_CONFIG);				// Configuration Register
	auxWire.write(0x20);
	auxWire.endTransmission();
	return true;
}

bool GrooveI2C_ADC::stop()
{

	return true;
}

float GrooveI2C_ADC::getReading()
{

	uint32_t data = 0;

	auxWire.beginTransmission(deviceAddress);		// transmit to device
	auxWire.write(REG_ADDR_RESULT);				// get result
	auxWire.endTransmission();

	auxWire.requestFrom(deviceAddress, 2);			// request 2byte from device
	delay(1);

	if (auxWire.available()<=2) {
		data = (auxWire.read()&0x0f)<<8;
		data |= auxWire.read();
	}

	return data * V_REF * 2.0 / 4096.0;
}

bool INA219::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	ada_ina219.begin(&auxWire);

	// By default the initialization will use the largest range (32V, 2A).
	ada_ina219.setCalibration_32V_1A();
	// ada_ina219.setCalibration_16V_400mA();

	return true;
}

bool INA219::stop()
{
	// TODO check if there is a way to minimize power consumption
	return true;
}

float INA219::getReading(typeOfReading wichReading)
{

	switch(wichReading) {
		case BUS_VOLT: {

			return ada_ina219.getBusVoltage_V();
			break;

		} case SHUNT_VOLT: {

			return ada_ina219.getShuntVoltage_mV();
			break;

		} case CURRENT: {

			return ada_ina219.getCurrent_mA();
			break;

		} case LOAD_VOLT: {

			float busvoltage 	= ada_ina219.getBusVoltage_V();
			float shuntvoltage 	= ada_ina219.getShuntVoltage_mV();

			return busvoltage + (shuntvoltage / 1000);
			break;

		}
	}

	return 0;
}

bool Groove_OLED::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	u8g2_oled.setBusClock(1000000); 	// 1000000 -> 68 ms for a full buffer redraw
	u8g2_oled.begin();
	u8g2_oled.drawXBM( 16, 16, 96, 96, scLogo);
	u8g2_oled.sendBuffer();
	currentLine = 1;
	delay(1000);
	u8g2_oled.clearDisplay();

	return true;;
}

bool Groove_OLED::stop()
{

	return true;
}

void Groove_OLED::print(char *payload)
{
	u8g2_oled.setFont(font);

	uint8_t thisLineChar = 0;
	uint8_t lineStart = 0;
	for (uint8_t i=0; i<strlen(payload); i++) {

		// If there is a newLine char
		if (payload[i] == 0xA || i == (strlen(payload) - 1)) {

			printLine(&payload[lineStart], thisLineChar + 1);
			lineStart += (thisLineChar + 1);
			thisLineChar = 0;

		// If line is full
		} else if (thisLineChar == (columns - 1)) {

			printLine(&payload[lineStart], thisLineChar + 1);
			lineStart += (thisLineChar + 1);
			thisLineChar = 0;

		// No new line yet
		} else thisLineChar ++;
	}

	u8g2_oled.sendBuffer();
}

void Groove_OLED::printLine(char *payload, uint8_t size)
{
	// Reject empty lines
	if (size < 1) return;
	if (payload[0] == 0xA || payload[0] == 0xD) return;

	// Clear screen if we are on the top
	if (currentLine == 1) {
		u8g2_oled.clearDisplay();
		u8g2_oled.home();

	// Slide screen one line up when bottom is reached
	} else if (currentLine > lines) {
		uint8_t *buffStart = u8g2_oled.getBufferPtr();
		memcpy(&buffStart[0], &buffStart[128], 1920);
		memset(&buffStart[1920], 0, 128);
		currentLine = lines;
	}

	// Print line
	char toPrint[size+1];
	snprintf(toPrint, sizeof(toPrint), payload);

	u8g2_oled.drawStr(0, currentLine * font_height, toPrint);
	u8g2_oled.drawStr(columns * font_width, currentLine * font_height, " "); 	// Clear incomplete chars at the end
	currentLine++;
}

void Groove_OLED::update(SckBase* base, bool force)
{
	if (millis() - lastUpdate < refreshRate && !force) return;
	lastUpdate = millis();

	if (base->config.debug.oled) return;

	// Info bar
	drawBar(base);

	if (base->st.error == ERROR_NONE) {

		// Setup mode screen
		if (base->st.onSetup) drawSetup(base);
		else displayReading(base);

	} else {

		// Error popup
		drawError(base->st.error);

	}

}

void Groove_OLED::drawBar(SckBase* base)
{
	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);

	// Clear the buffer area of the bar
	uint8_t *buffStart = u8g2_oled.getBufferPtr();
	memset(&buffStart[0], 0, 256);

	uint8_t font_h = u8g2_oled.getMaxCharHeight();

	// Print current mode on the left
	if (base->st.onSetup) u8g2_oled.drawStr(0, font_h, "SETUP");
	else if (base->st.onShell) u8g2_oled.drawStr(0, font_h, "SHELL");
	else if (base->st.mode == MODE_NET) u8g2_oled.drawStr(0, font_h, "WIFI");
	else if (base->st.mode == MODE_SD) u8g2_oled.drawStr(0, font_h, "SD");

	// Print "icon tray" from right to left
	uint8_t tray_x = 128;
	uint8_t sep = 4;

	if (base->battery.present) {

		// Battery percent
		char percent[5];
		snprintf(percent, sizeof(percent), "%u%%", base->battery.last_percent);
		tray_x -= u8g2_oled.getStrWidth(percent);
		u8g2_oled.drawStr(tray_x, font_h, percent);

		// Battery Icon
		tray_x -= (batt_charge_width + sep);
		if (base->led.chargeStatus == base->led.CHARGE_CHARGING) u8g2_oled.drawXBM(tray_x, (16 - batt_charge_height) / 2, batt_charge_width, batt_charge_height, batt_charge_bits);
		else if (base->battery.last_percent > 75) u8g2_oled.drawXBM(tray_x, (16 - batt_full_height) / 2, batt_full_width, batt_full_height, batt_full_bits);
		else if (base->battery.last_percent > 25) u8g2_oled.drawXBM(tray_x, (16 - batt_half_height) / 2, batt_half_width, batt_half_height, batt_half_bits);
		else u8g2_oled.drawXBM(tray_x, (16 - batt_empty_height) / 2, batt_empty_width, batt_empty_height, batt_empty_bits);

	}

	// AC icon
	if (base->charger.onUSB) {
		tray_x -= (AC_width + sep);
		u8g2_oled.drawXBM(tray_x, (16 - AC_height) / 2, AC_width, AC_height, AC_bits);
	}

	// Time sync icon
	if (base->st.timeStat.ok) {
		tray_x -= (clock_width + sep);
		u8g2_oled.drawXBM(tray_x, (16 - clock_height) / 2, clock_width, clock_height, clock_bits);
	}

	// Sdcard icon
	if (base->st.cardPresent) {
		tray_x -= (sdcard_width + sep);
		u8g2_oled.drawXBM(tray_x, (16 - sdcard_height) / 2, sdcard_width, sdcard_height, sdcard_bits);
	}

	// Wifi icon
	if (base->st.wifiStat.ok && base->st.espON) {
		tray_x -= (wifi_width + sep);
		u8g2_oled.drawXBM(tray_x, (16 - wifi_height) / 2, wifi_width, wifi_height, wifi_bits);
	}

	u8g2_oled.drawHLine(0, 15, 128);
	u8g2_oled.updateDisplayArea(0, 0, 16, 2);
}

void Groove_OLED::drawError(errorType wichError)
{
	if (lastError == wichError) return;

	// Clear error buffer area
	uint8_t *buffStart = u8g2_oled.getBufferPtr();
	memset(&buffStart[1792], 0, 256);

	u8g2_oled.setFont(u8g2_font_7x13B_mr);

	// Print a frame with an alert icon on the left
	u8g2_oled.drawFrame(0, 112, 128, 16);
	u8g2_oled.drawBox(0, 112, 16, 16);
	u8g2_oled.drawXBM(2, 114, error_width, error_height, error_bits);

	// Set message
	char errorMsg[18];
	switch(wichError) {
		case ERROR_SD:
			snprintf(errorMsg, sizeof(errorMsg), "NO SDCARD FOUND");
			break;
		case ERROR_SD_PUBLISH:
			snprintf(errorMsg, sizeof(errorMsg), "SDCARD ERROR");
			break;
		case ERROR_TIME:
			snprintf(errorMsg, sizeof(errorMsg), "TIME NOT SYNCED");
			break;
		case ERROR_NO_WIFI_CONFIG:
			snprintf(errorMsg, sizeof(errorMsg), "NO WIFI SET");
			break;
		case ERROR_AP:
			snprintf(errorMsg, sizeof(errorMsg), "WRONG WIFI SSID");
			break;
		case ERROR_PASS:
			snprintf(errorMsg, sizeof(errorMsg), "WRONG WIFI PASS");
			break;
		case ERROR_WIFI_UNKNOWN:
			snprintf(errorMsg, sizeof(errorMsg), "WIFI ERROR");
			break;
		case ERROR_MQTT:
			snprintf(errorMsg, sizeof(errorMsg), "MQTT ERROR");
			break;
		case ERROR_NO_TOKEN_CONFIG:
			snprintf(errorMsg, sizeof(errorMsg), "NO TOKEN SET");
			break;
		case ERROR_BATT:
			snprintf(errorMsg, sizeof(errorMsg), "LOW BATTERY");
			break;
		default:
			break;
	}

	// Print message
	u8g2_oled.drawStr(19, 125, errorMsg);

	lastError = wichError;

	// Update display
	u8g2_oled.updateDisplayArea(0, 14, 16, 2);
}

void Groove_OLED::drawSetup(SckBase* base)
{
	// Clear buffer (except info bar)
	uint8_t *buffStart = u8g2_oled.getBufferPtr();
	memset(&buffStart[256], 0, 1792);

	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);
	uint8_t font_h = u8g2_oled.getMaxCharHeight();

	char conn[] = "Connect to the Wi-Fi:";
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(conn)) / 2, font_h + 30, conn);

	u8g2_oled.setFont(u8g2_font_t0_16b_tf);
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(base->hostname)) / 2, font_h + 55, base->hostname);

	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);
	char conn2[] = "If no window opens,";
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(conn2)) / 2, font_h + 80, conn2);
	char conn3[] = "with your browser go to";
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(conn3)) / 2, font_h + 92, conn3);
	char conn4[] = "http://sck.me";
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(conn4)) / 2, font_h + 104, conn4);
	char conn5[] = "or 192.168.1.1";
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(conn5)) / 2, font_h + 116, conn5);

	u8g2_oled.updateDisplayArea(0, 2, 16, 14);
}

void Groove_OLED::displayReading(SckBase* base)
{
	if (base->rtc.getEpoch() - showStartTime <= showTime) return;

	SensorType sensorToShow = SENSOR_COUNT;
	uint8_t cycles = 0;

	// Find next sensor to show
	for (uint8_t i=lastShown+1; i<SENSOR_COUNT; i++) {

		SensorType thisSensor = static_cast<SensorType>(i);

		if (base->sensors[thisSensor].enabled &&
				base->sensors[thisSensor].oled_display &&
				base->sensors[thisSensor].type != SENSOR_GROVE_OLED && 		//Oled screen has nothing to show
				base->sensors[thisSensor].type != SENSOR_BATT_PERCENT) { 	// Battery is already shown on oled info-bar

			sensorToShow = thisSensor;
			break;
		}
		if (i == SENSOR_COUNT - 1) {
			i = 0;
			cycles++;
			if (cycles > 1) break; 	// Avoid getting stuck here if no sensor is enabled
		}
	}

	// Clear buffer (except info bar)
	uint8_t *buffStart = u8g2_oled.getBufferPtr();
	memset(&buffStart[256], 0, 1792);

	// Draw Title
	u8g2_oled.setFont(u8g2_font_t0_16b_tf);

	// Split in two lines if needed
	const char *sensorTitle = base->sensors[sensorToShow].title;
	uint8_t baseLine = 55 - u8g2_oled.getMaxCharHeight();
	if (u8g2_oled.getStrWidth(sensorTitle) > 128) {

		baseLine = 55;

		// Try splitting on first space
		char line1[20];
		char blank[] = " ";
		uint8_t splitPoint = strcspn(sensorTitle, blank);
		memcpy(line1, sensorTitle, splitPoint);
		line1[splitPoint + 1] = '\0';

		char *line2 = strchr(sensorTitle, ' ') + 1;

		// If some of the lines is to big split in half
		if (u8g2_oled.getStrWidth(line2) > 128 ||
			u8g2_oled.getStrWidth(line1) > 128) {

			// Split in half
			splitPoint = strlen(sensorTitle) / 2;
			memcpy(line1, sensorTitle, splitPoint);
			line1[splitPoint + 1] = '\0';
			u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(line1)) / 2, (baseLine - u8g2_oled.getMaxCharHeight()) - 2, line1);
			u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(&sensorTitle[splitPoint])) / 2, baseLine + u8g2_oled.getDescent(), &sensorTitle[splitPoint]);

		} else {

			// Split on space
			u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(line1)) / 2, (baseLine - u8g2_oled.getMaxCharHeight()) - 2, line1);
			u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(line2)) / 2, baseLine + u8g2_oled.getDescent(), line2);
		}

	} else u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(sensorTitle)) / 2, baseLine + u8g2_oled.getDescent(), sensorTitle);

	// Draw unit
	const char *sensorUnit = base->sensors[sensorToShow].unit;
	u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(sensorUnit)) / 2, 128 + u8g2_oled.getDescent(), sensorUnit);

	// Draw Value
	uint8_t vCenter = baseLine + ((128 - u8g2_oled.getMaxCharHeight() - baseLine) / 2);
	u8g2_oled.setFont(u8g2_font_fub30_tn);
	String value = base->sensors[sensorToShow].reading;
	if (base->sensors[sensorToShow].state != 0) value = "--";
	if (u8g2_oled.getStrWidth(value.c_str()) > 128) {
		u8g2_oled.setFont(u8g2_font_fub20_tn);
		u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(value.c_str())) / 2, vCenter + (u8g2_oled.getMaxCharHeight() / 2), value.c_str());
	} else {
		u8g2_oled.drawStr((128 - u8g2_oled.getStrWidth(value.c_str())) / 2, vCenter + (u8g2_oled.getMaxCharHeight() / 2), value.c_str());
	}


	u8g2_oled.updateDisplayArea(0, 2, 16, 14);

	lastShown = sensorToShow;
	showStartTime = base->rtc.getEpoch();
}

void Groove_OLED::plot(String value, const char *title, const char *unit)
{
	// TODO support negative values
	// TODO Auto adjust lower limit of Y scale

	// Fresh start
	if (title != NULL) {
		_unit = unit;
		_title = title;
		minY = 0;
		maxY = 100;
		u8g2_oled.clearDisplay();
		plotData.clear();
	}

	u8g2_oled.clearBuffer();

	// Title and unit
	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);
	u8g2_oled.drawStr(0, u8g2_oled.getMaxCharHeight() + u8g2_oled.getDescent(), _title);
	u8g2_oled.drawStr(0, (u8g2_oled.getMaxCharHeight() * 2) - 2, value.c_str());
	u8g2_oled.drawStr(u8g2_oled.getStrWidth(value.c_str()) + 10, (u8g2_oled.getMaxCharHeight() * 2) - 2, _unit);


	// Get the new value
	float Fvalue = value.toFloat();

	// If this value is grater than our limit adjust scale
	if (Fvalue > maxY) remap(Fvalue);

	// Store the new value scaled to screen size
	int8_t Ivalue = map(Fvalue, minY, maxY, screenMin, screenMax);
	plotData.add(Ivalue);
	if (plotData.size() > 128) plotData.remove(0); // Keep the size of the array limited to the screen size

	// Check if we need to reajust scale (Happens when big values get out of scope)
	float currentMaxY = 0;
	for (uint8_t i=0; i<plotData.size(); i++) {
		if (plotData.get(i) > currentMaxY) currentMaxY = plotData.get(i);
	}
	float bigCurrentMaxY = map(currentMaxY, screenMin, screenMax, minY, maxY);
	if (bigCurrentMaxY < (maxY - (maxY / 10)) && bigCurrentMaxY > 0) remap(bigCurrentMaxY);


	// Plot the data on the display
	uint8_t ii = plotData.size() - 1;
	for (uint8_t i=127; i>0; i--) {
		if (ii == 0) break;
		u8g2_oled.drawLine(i, 128 - plotData.get(ii), i - 1, 128 - plotData.get(ii - 1));
		ii--;
	}

	// Print Y top value
	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);
	char buff[8];
	snprintf(buff, sizeof(buff), "%.0f", maxY);
	u8g2_oled.drawStr(128 - u8g2_oled.getStrWidth(buff), 16 + u8g2_oled.getMaxCharHeight(), buff);
	u8g2_oled.drawHLine(118, 18 + u8g2_oled.getMaxCharHeight(), 10);

	// Print Y bottom value
	snprintf(buff, sizeof(buff), "%.0f", minY);
	u8g2_oled.drawStr(128 - u8g2_oled.getStrWidth(buff), 126, buff);
	u8g2_oled.drawHLine(118, 127, 10);

	u8g2_oled.sendBuffer();
}

void Groove_OLED::remap(float newMaxY)
{
	newMaxY += (newMaxY / 10);

	float fa = maxY / newMaxY;

	for (uint8_t i=0; i<plotData.size(); i++) {

		float remaped = plotData.get(i) * fa;

		// round it properly
		if (remaped > 0) remaped += 0.5;
		else remaped -= 0.5;

		plotData.set(i, (int)remaped);
	}

	maxY = newMaxY;
}

bool WaterTemp_DS18B20::start()
{

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.begin();

	DS_bridge.reset();
	DS_bridge.wireReset();
	DS_bridge.wireSkip();
	DS_bridge.wireWriteByte(0x44);

	detected = true;

	return true;
}

bool WaterTemp_DS18B20::stop()
{
	return true;
}

float WaterTemp_DS18B20::getReading()
{

	while (!DS_bridge.wireSearch(addr)) {

		DS_bridge.wireResetSearch();
		DS_bridge.wireReset();
		DS_bridge.selectChannel(0); 			// After reset need to set channel 0 because we are using the version with single channel (DS2482_100)
		DS_bridge.configure(conf);
		DS_bridge.wireSkip();
		DS_bridge.configure(conf); 				// Set bus on strong pull-up after next write, not only LSB nibble is required
		DS_bridge.wireWriteByte(0x44); 			// Convert temperature on all devices
		DS_bridge.configure(0x01);
	}

	//	Test if device is in reality the DS18B20 Water Temperature
	if (addr[0]==0x28) {

		// Read temperature data.
		DS_bridge.wireReset(); 				//DS_bridge.reset();
		DS_bridge.selectChannel(0); 		//necessary on -800
		DS_bridge.wireSelect(addr);
		DS_bridge.wireWriteByte(0xbe);      // Read Scratchpad command

		// We need to read 9 bytes
		for (int i=0; i<9; i++) data[i] = DS_bridge.wireReadByte();

		// Convert to decimal temperature
		int LowByte = data[0];
		int HighByte = data[1];
		int TReading = (HighByte << 8) + LowByte;
		int SignBit = TReading & 0x8000;

		// If Temperature is negative
		if (SignBit) TReading = (TReading ^ 0xffff) + 1;

		int Tc_100 = (double)TReading * 0.0625 * 10;

		// If the reading is negative make it efective
		if (SignBit) Tc_100 = 0 - Tc_100;

		return ((float)(Tc_100) / 10) + 1;
	}

	return 0;
}

bool Atlas::start()
{
	if (beginDone) return true;

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;


	// Protocol lock
	if (!sendCommand((char*)"Plock,1")) return false;
	delay(shortWait);

	// This actions are only for conductivity (EC) sensor
	if (EC) {

		// ----  Set parameters
		if (sendCommand((char*)"O,?")) { 	// Ask info about enabled parameters
			delay(shortWait);
			getResponse();
			if (!atlasResponse.equals("?O,EC,TDS,S,SG")) {
				SerialUSB.println("Enabling all metrics for EC Atlas");
				const char *parameters[4] = PROGMEM {"O,EC,1", "O,TDS,1", "O,S,1", "O,SG,1"};
				for (int i = 0; i<4; ++i) {
					if (!sendCommand((char*)parameters[i])) return false;
					delay(longWait);
				}
			}
		} else return false;

	} else if (DO) {

		// ---- Check if this is really a Atlas DO sensor (allows sharing I2C addres 0x61 with SCD30 CO2 sensor)
		if (sendCommand((char*)"i")) {
			delay(shortWait);
			getResponse();
			if (!atlasResponse.startsWith("?I,DO")) return false;
		} else return false;

		// ---- Set parameters
		if (sendCommand((char*)"O,?")) {
			delay(shortWait);
			getResponse();
			if (!atlasResponse.equals((char*)"?O,%,mg")) {
				if (!sendCommand((char*)"O,%,1")) return false;
				delay(shortWait);
				if (!sendCommand((char*)"O,mg,1")) return false;
				delay(shortWait);
			}
		} else return false;
	}

	beginDone = true;
	detected = true;

	goToSleep();

	return true;
}

bool Atlas::stop()
{
	return true;
}

bool Atlas::getReading()
{

	if (millis() - lastUpdate < 2000) return true;
	uint32_t started = millis();
	while (getBusyState()) {
		if (millis() - started > 5000) return false; 	// Timeout
		delay(2);
	}
	return true;
}

bool Atlas::getBusyState()
{

	if (millis() - lastUpdate < 2) return true;
	switch (state) {

		case REST: {

			// ORP doesn't need temp compensation so we jump directly to ask reading
			if (ORP) {
				if (millis() - lastCommandSent >= shortWait) {
					if (sendCommand((char*)"r")) state = ASKED_READING;
				}
			}

			if (TEMP) {
				state = TEMP_COMP_SENT;
				break;
			}

			if (tempCompensation()) state = TEMP_COMP_SENT;
			break;

		} case TEMP_COMP_SENT: {

			if (millis() - lastCommandSent >= shortWait) {
				if (sendCommand((char*)"r")) state = ASKED_READING;
			}
			break;

		} case ASKED_READING: {

			uint16_t customWait = longWait;
			if (TEMP) customWait = mediumWait;

			if (millis() - lastCommandSent >= customWait) {

				uint8_t code = getResponse();

				if (code == 254) {
					// Still working (wait a little more)
					lastCommandSent = lastCommandSent + 200;
					break;

				} else if (code == 1) {

					// Reading OK
					state = REST;

					if (PH || TEMP || ORP)	newReading[0] = atlasResponse.toFloat();
					if (EC || DO) {

						uint8_t readingNum = 2;
						if (EC) readingNum = 4;

						for (uint8_t i=0; i<readingNum; i++) {

							uint8_t endIdx = atlasResponse.indexOf(",");

							String newReadingStr;
							if (endIdx > 0) {
								newReadingStr = atlasResponse.substring(0, endIdx);
								atlasResponse.remove(0, endIdx+1);
							} else {
								newReadingStr = atlasResponse.substring(0);
							}

							newReading[i] = newReadingStr.toFloat();
						}
					}
					goToSleep();
					return false;
					break;

				} else {

					// Error
					state = REST;
					newReading[0] = 0;
					goToSleep();
					return false;
					break;
				}
			}
			break;
		}
	}

	lastUpdate = millis();
	return true;
}

void Atlas::goToSleep()
{

	auxWire.beginTransmission(deviceAddress);
	auxWire.write("Sleep");
	auxWire.endTransmission();
}

bool Atlas::sendCommand(char* command)
{

	uint8_t retrys = 5;

	for (uint8_t i=0; i<retrys; ++i) {

		auxWire.beginTransmission(deviceAddress);
		auxWire.write(command);

		auxWire.requestFrom(deviceAddress, 1, true);
		uint8_t confirmed = auxWire.read();
		auxWire.endTransmission();

		if (confirmed == 1) {
			lastCommandSent = millis();
			return true;
		}

		delay(300);
	}
	return false;
}

bool Atlas::tempCompensation()
{
	// Temperature comepnsation for PH, EC, and DO
	float temperature;

	if (!ORP) {
		if (waterTemp_DS18B20.detected) temperature = waterTemp_DS18B20.getReading();
		else if (atlasTEMP.detected) {

			if (millis() - atlasTEMP.lastUpdate > 10000) {
				while (atlasTEMP.getBusyState()) delay(2);
			}

			temperature = atlasTEMP.newReading[0];
		}
	}

    char data[10];
    sprintf(data,"T,%.2f",temperature);
    if (!sendCommand(data)) return false;

	// Salinity compensation only for DO
	if (DO && atlasEC.detected) {

		if (millis() - atlasEC.lastUpdate > 10000) {
			while (atlasEC.getBusyState()) delay(2);
		}

		char salData[20];
		float salinity = atlasEC.newReading[2];
		sprintf(salData,"S,%.2f,ppt",salinity);

		if (!sendCommand(salData)) return false;
	}

	return true;
}

uint8_t Atlas::getResponse()
{

	uint8_t code;

	auxWire.requestFrom(deviceAddress, 40, 1);
	uint32_t time = millis();
	while (!auxWire.available()) if ((millis() - time)>500) return 0x00;
	code = auxWire.read();

	atlasResponse = "";

	switch (code) {
		case 0: 		// Undefined
		case 2:			// Error
		case 255:		// No data sent
		case 254: {		// Still procesing, not ready

			return code;
			break;

		} default : {

			while (auxWire.available()) {
				char buff = auxWire.read();
				atlasResponse += buff;
			}
			auxWire.endTransmission();

			if (atlasResponse.length() > 0) {
				return 1;
			}

			return 2;
		}
	}
}

bool Moisture::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;
	if (alreadyStarted) return true;

	chirp.begin();

	alreadyStarted = true;

	detected = true;

	return true;
}

bool Moisture::stop()
{
	return true;
}

bool Moisture::getReading(SensorType wichSensor)
{
	uint32_t started = millis();
	while (chirp.isBusy()) {
		if (millis() - started > 5000) return 0; 	// Timeout
		delay(1);
	}

	switch(wichSensor) {
		case SENSOR_CHIRP_MOISTURE_RAW: {

			raw = chirp.getCapacitance();
			return true;

		} case SENSOR_CHIRP_MOISTURE: {

			if (!calibrated) return false;
			int32_t thisValue = chirp.getCapacitance();
			moisture = map(thisValue, dryPoint, wetPoint, 0.0, 100.0);
			return true;

		} case SENSOR_CHIRP_TEMPERATURE: {

			temperature = chirp.getTemperature() / 10.0;
			return true;

		} case SENSOR_CHIRP_LIGHT: {

			// From Arduino library documentation
			// The measurement gives 65535 in a dark room away form desk lamp - so more light, lower reading.

			light = chirp.getLight(false); 		// We are sending the reading from previous request
			chirp.startMeasureLight(); 		// Ask for next reading
			return true;

		} default: break;
	}

	return false;
}

uint8_t Moisture::getVersion()
{

	return chirp.getVersion();
}

void Moisture::sleep()
{

	chirp.sleep();
}

bool Moisture::resetAddress(int currentAddress)
{
	chirp.changeSensor(currentAddress, true);
	return chirp.setAddress(0x20, true);
}

bool PMsensor::start()
{
	if (started) return true;

	if (!I2Cdetect(&auxWire, deviceAddress) || failed) return false;

	auxWire.beginTransmission(deviceAddress);
	if (_slot == SLOT_A) auxWire.write(START_PMA);
	else if (_slot == SLOT_B) auxWire.write(START_PMB);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();

	if (result == 0) failed = true;
	else started = true;

	return result;
}

bool PMsensor::stop()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	if (_slot == SLOT_A) auxWire.write(STOP_PMA);
	else if (_slot == SLOT_B) auxWire.write(STOP_PMB);
	auxWire.endTransmission();

	started = false;
	return true;
}

bool PMsensor::update()
{
	// Only update if more than one second has past
	if (millis() - lastReading > 1000) {

		// Ask for readings
		auxWire.beginTransmission(deviceAddress);
		if (_slot == SLOT_A) auxWire.write(GET_PMA);
		else if (_slot == SLOT_B) auxWire.write(GET_PMB);
		auxWire.endTransmission();
		delay(2);

		// Get the readings
		auxWire.requestFrom(deviceAddress, valuesSize);
		uint32_t time = millis();
		while (!auxWire.available()) if ((millis() - time) > 500) return false;

		// Check for errors
		bool isError = true;
		for (uint8_t i=0; i<valuesSize; i++) {
			values[i] = auxWire.read();
			if (values[i] != 255) isError = false;
		}
		if (isError) return false;

		pm1 = (values[0]<<8) + values[1];
		pm25 = (values[2]<<8) + values[3];
		pm10 = (values[4]<<8) + values[5];
		pn03 = (values[6]<<8) + values[7];
		pn05 = (values[8]<<8) + values[9];
		pn1 = (values[10]<<8) + values[11];
		pn25 = (values[12]<<8) + values[13];
		pn5 = (values[14]<<8) + values[15];
		pn10 = (values[16]<<8) + values[17];

		lastReading = millis();
	}

	return true;
}

bool PM_DallasTemp::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(DALLASTEMP_START);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();
	return result;
}

bool PM_DallasTemp::stop()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(DALLASTEMP_STOP);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();
	return result;
}

float PM_DallasTemp::getReading()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(GET_DALLASTEMP);
	auxWire.endTransmission();

	// Get the reading
	auxWire.requestFrom(deviceAddress, 4);
	uint32_t start = millis();
	while (!auxWire.available()) if ((millis() - start)>500) return -9999;
	for (uint8_t i=0; i<4; i++) uRead.b[i] = auxWire.read();
	return uRead.fval;
}

TinyGPSPlus tinyGps;
TinyGPSCustom fixQuality(tinyGps, "GPGGA", 6);
TinyGPSCustom nfixQuality(tinyGps, "GNGGA", 6);

bool Sck_GPS::start()
{
	if (started) return true;

	if (neoM8uGps.start()) {
		gps_source = &neoM8uGps;
		started = true;
		return true;
	}

	if (xa1110gps.start()) {
		gps_source = &xa1110gps;
		started = true;
		return true;
	}

	if (pmGroveGps.start()) {
		gps_source = &pmGroveGps;
		started = true;
		return true;
	}

	return false;
}

bool Sck_GPS::stop()
{
	if (!started) return true;

	gps_source->stop();
	started = false;

	return true;
}

bool Sck_GPS::getReading(SckBase *base, SensorType wichSensor)
{
	// Use time from gps to set RTC if time is not set or older than 1 hour
	if (((millis() - base->lastTimeSync) > 3600000 || base->lastTimeSync == 0)) {

		if (gps_source->getReading(SENSOR_GPS_FIX_QUALITY, r)) {
			if (r.fixQuality > 0 && r.timeValid) {
				// Wait for some GPS readings after sync to be sure time is accurate
				if (fixCounter > 5) base->setTime(String(r.epochTime));
				else fixCounter++;
			}
		}

	} else {
		fixCounter = 0;
	}

	if (!gps_source->getReading(wichSensor, r)) return false;

	return true;
}

bool Sck_GPS::update()
{
	return gps_source->update();
}

bool PM_Grove_GPS::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(GROVEGPS_START);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceAddress, 1);

	bool result = auxWire.read();

	return result;
}

bool PM_Grove_GPS::stop()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	auxWire.beginTransmission(deviceAddress);
	auxWire.write(GROVEGPS_STOP);
	auxWire.endTransmission();

	return true;
}

bool PM_Grove_GPS::getReading(SensorType wichSensor, GpsReadings &r)
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	//  Only ask for readings if last one is older than
	if (millis() - lastReading < 500 && r.fixQuality > 0) return true;

	// Ask for reading
	auxWire.beginTransmission(deviceAddress);
	auxWire.write(GROVEGPS_GET);
	auxWire.endTransmission();

	// Get the reading
	auxWire.requestFrom(deviceAddress, DATA_LEN);
	uint32_t time = millis();
	while (!auxWire.available()) if ((millis() - time)>500) return false;

	for (uint8_t i=0; i<DATA_LEN; i++) data[i] = auxWire.read();

	// Fix quality
	memcpy(&r.fixQuality, &data[0], 1);

	// Time
	memcpy(&r.timeValid, &data[23], 1);
	if (r.timeValid) memcpy(&r.epochTime, &data[24], 4);
	// With this GPS wrong time is reported as Valid when no GPS fix
	// So if no fix we mark time as invalid
	if (r.fixQuality == 0) r.timeValid = false;

	// Location
	memcpy(&r.locationValid, &data[1], 1);
	if (r.locationValid) {

		// Latitude
		memcpy(&r.latitude, &data[2], 8);

		// Longitude
		memcpy(&r.longitude, &data[10], 8);

	} else if (wichSensor == SENSOR_GPS_LATITUDE ||	wichSensor == SENSOR_GPS_LONGITUDE) return false;

	// Altitude
	memcpy(&r.altitudeValid, &data[18], 1);
	if (r.altitudeValid) memcpy(&r.altitude, &data[19], 4);
	else if (wichSensor == SENSOR_GPS_ALTITUDE) return false;

	// Speed
	memcpy(&r.speedValid, &data[28], 1);
	if (r.speedValid) memcpy(&r.speed, &data[29], 4);
	else if (wichSensor == SENSOR_GPS_SPEED) return false;

	// Horizontal dilution of position
	memcpy(&r.hdopValid, &data[33], 1);
	if (r.hdopValid) memcpy(&r.hdop, &data[34], 4);
	else if (wichSensor == SENSOR_GPS_HDOP) return false;

	// Satellites
	memcpy(&r.satellitesValid, &data[38], 1);
	if (r.satellitesValid) memcpy(&r.satellites, &data[39], 1);
	else if (wichSensor == SENSOR_GPS_SATNUM) return false;

	lastReading = millis();

	return true;
}

bool PM_Grove_GPS::update()
{
	return true;
}

bool XA111GPS::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	if (!i2cGps.begin(auxWire)) return false;

	return true;
}

bool XA111GPS::stop()
{
	return true;
}

bool XA111GPS::getReading(SensorType wichSensor, GpsReadings &r)
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	//  Only ask for readings if last one is older than
	if (millis() - lastReading < 500) return true;

	// TODO
	// this was moved to update funtion, check if it works OK
	/* while (i2cGps.available()) tinyGps.encode(i2cGps.read()); */

	// Time
	r.timeValid = tinyGps.time.isValid();
	if (r.timeValid) {
		// Time (epoch) -> uint32 - 4
		struct tm tm; 				// http://www.nongnu.org/avr-libc/user-manual/structtm.html
		tm.tm_isdst = -1; 			// -1 means no data available
		tm.tm_yday = 0;
		tm.tm_wday = 0;
		tm.tm_year = tinyGps.date.year() - 1900; 	// tm struct expects years since 1900
		tm.tm_mon = tinyGps.date.month() - 1; 	// tm struct uses 0-11 months
		tm.tm_mday = tinyGps.date.day();
		tm.tm_hour = tinyGps.time.hour();
		tm.tm_min = tinyGps.time.minute();
		tm.tm_sec = tinyGps.time.second();
		r.epochTime = mktime(&tm);
	}

	// Fix Quality
	String fixQual = fixQuality.value();
	r.fixQuality = fixQual.toInt();
	if (r.fixQuality == 0) {
		fixQual = nfixQuality.value();
		r.fixQuality = fixQual.toInt();
	}

	// Location
	r.locationValid = tinyGps.location.isValid();
	if (r.locationValid) {

		// Latitude
		r.latitude = tinyGps.location.lat();

		// Longitude
		r.longitude = tinyGps.location.lng();

	} else if (wichSensor == SENSOR_GPS_LATITUDE ||	wichSensor == SENSOR_GPS_LONGITUDE) return false;

	// Altitude
	r.altitudeValid = tinyGps.altitude.isValid();
	if (r.altitudeValid) r.altitude = tinyGps.altitude.meters();
	else if (wichSensor == SENSOR_GPS_ALTITUDE) return false;

	// Speed
	r.speedValid = tinyGps.speed.isValid();
	if (r.speedValid) r.speed = tinyGps.speed.mps();
	else if (wichSensor == SENSOR_GPS_SPEED) return false;

	// Horizontal dilution of position
	r.hdopValid = tinyGps.hdop.isValid();
	if (r.hdopValid) r.hdop = tinyGps.hdop.value();
	else if (wichSensor == SENSOR_GPS_HDOP) return false;

	// Satellites
	r.satellitesValid = tinyGps.satellites.isValid();
	if (r.satellitesValid) r.satellites = tinyGps.satellites.value();
	else if (wichSensor == SENSOR_GPS_SATNUM) return false;

	lastReading = millis();

	// TODO use power save mode between readings if posible

	return true;
}

bool XA111GPS::update()
{
	// Test with the GPS if this make sense here
	while (i2cGps.available()) tinyGps.encode(i2cGps.read());

	return true;
}

bool NEOM8UGPS::start()
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	if (!ubloxGps.begin(auxWire)) return false;

	ubloxGps.setUART1Output(0); 		// Disable the UART1 port output
	ubloxGps.setUART2Output(0); 		// Disable Set the UART2 port output
	ubloxGps.setI2COutput(COM_TYPE_UBX); 	// Set the I2C port to output UBX only (turn off NMEA noise)
	ubloxGps.setNavigationFrequency(4);
	ubloxGps.setAutoPVT(true); 		// Tell the GPS to "send" each solution
	ubloxGps.saveConfiguration(); 		// Save the current settings to flash and BBR

	return true;
}

bool NEOM8UGPS::stop()
{
	// TODO
	// Lowpower mode
	return true;
}

bool NEOM8UGPS::getReading(SensorType wichSensor, GpsReadings &r)
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	switch(wichSensor) {

		case SENSOR_GPS_FIX_QUALITY:
		{

			// Time
			if (ubloxGps.getDateValid() && ubloxGps.getTimeValid()) {
				// Time (epoch) -> uint32 - 4
				struct tm tm; 					// http://www.nongnu.org/avr-libc/user-manual/structtm.html
				tm.tm_isdst = -1; 				// -1 means no data available
				tm.tm_yday = 0;
				tm.tm_wday = 0;
				tm.tm_year = ubloxGps.getYear() - 1900; 	// tm struct expects years since 1900
				tm.tm_mon = ubloxGps.getMonth() - 1; 		// tm struct uses 0-11 months
				tm.tm_mday = ubloxGps.getDay();
				tm.tm_hour = ubloxGps.getHour();
				tm.tm_min = ubloxGps.getMinute();
				tm.tm_sec = ubloxGps.getSecond();
				r.timeValid = true;
				r.epochTime = mktime(&tm);
			} else {
				r.timeValid = false;
			}

			uint8_t fixQual = ubloxGps.getFixType(); 		// Type of fix: 0=no, 3=3D, 4=GNSS+Deadreckoning */
			// TODO
			// Translate fix quality to NMEA standard
			r.fixQuality = fixQual;
			break;

		}
		case SENSOR_GPS_LATITUDE:
		case SENSOR_GPS_LONGITUDE:
		{
			// Location
			r.locationValid = true;
			// Latitude
			r.latitude = (float)ubloxGps.getLatitude() / 10000000.0;
			// Longitude
			r.longitude = (float)ubloxGps.getLongitude() / 10000000.0;
			break;
		}
		case  SENSOR_GPS_ALTITUDE:
		{
			// Altitude
			// TODO check if main sea level option (getAltitudeMSL()) is better for us
			r.altitudeValid = true;
			r.altitude = (float)ubloxGps.getAltitude() / 1000.0;
			break;
		}
		case SENSOR_GPS_SPEED:
		{
			// Speed
			r.speedValid = true;
			r.speed = (float)ubloxGps.getGroundSpeed() / 1000.0;
			break;
		}
		case SENSOR_GPS_HDOP:
		{
			// Horizontal dilution of position
			r.hdopValid = true;
			r.hdop = ubloxGps.getPDOP();
			break;
		}
		case SENSOR_GPS_SATNUM:
		{
			// Satellites
			r.satellitesValid = true;
			r.satellites = ubloxGps.getSIV();
			break;
		}
		default:
			break;
	}

	lastReading = millis();

	// TODO use power save mode between readings if posible

	return true;
}

bool NEOM8UGPS::update()
{
	ubloxGps.checkUblox();
	return ubloxGps.getPVT();
}

bool Sck_DallasTemp::start()
{
	pinPeripheral(pinAUX_WIRE_SCL, PIO_DIGITAL);
	OneWire oneWire = OneWire(pinAUX_WIRE_SCL);
	DallasTemperature _dallasTemp = DallasTemperature(&oneWire);

	_dallasTemp.begin();

	// If no device is found return false
	_dallasTemp.getAddress(_oneWireAddress, 0);

	_dallasTemp.setResolution(12);
	_dallasTemp.setWaitForConversion(true);

	if (!getReading()) return false;

	pinPeripheral(pinAUX_WIRE_SCL, PIO_SERCOM);

	return true;
}

bool Sck_DallasTemp::stop()
{

	return true;
}

bool Sck_DallasTemp::getReading()
{
	pinPeripheral(pinAUX_WIRE_SCL, PIO_DIGITAL);
	OneWire oneWire = OneWire(pinAUX_WIRE_SCL);
	DallasTemperature _dallasTemp = DallasTemperature(&oneWire);

	_dallasTemp.requestTemperatures();
	reading = _dallasTemp.getTempC(_oneWireAddress);
	if (reading <= DEVICE_DISCONNECTED_C) return false;

	pinPeripheral(pinAUX_WIRE_SCL, PIO_SERCOM);

	return true;
}

bool Sck_Range::start()
{
	if (alreadyStarted) return true;

	if(vl6180x.VL6180xInit() != 0) return false;

	vl6180x.VL6180xDefautSettings();

	alreadyStarted = true;

	return true;
}

bool Sck_Range::stop()
{
	alreadyStarted = false;
	return true;
}

bool Sck_Range::getReading(SensorType wichSensor)
{
	switch(wichSensor)
{
	case SENSOR_RANGE_DISTANCE:
		readingDistance = vl6180x.getDistance();
		break;
	case SENSOR_RANGE_LIGHT:
		readingLight = vl6180x.getAmbientLight(GAIN_1);
		break;
	default:
		return false;
}
	return true;
}

bool Sck_BME680::start()
{
	if (alreadyStarted) return true;

	if (!bme.begin(deviceAddress)) return false;

	alreadyStarted = true;
	return true;
}

bool Sck_BME680::stop()
{
	alreadyStarted = false;
	return true;
}

bool Sck_BME680::getReading()
{
	if (millis() - lastTime > minTime) {
		if (!bme.performReading()) return false;
		lastTime = millis();
	}

	temperature = bme.temperature;
	humidity = bme.humidity;
	pressure = bme.pressure / 1000;  // Converted to kPa
	VOCgas = bme.gas_resistance;

	return true;
}

bool Sck_ADS1X15::start(uint8_t address)
{
	if (!I2Cdetect(&auxWire, address)) return false;

	if (started) return true;

	ads.begin(address);
	started = true;
	return true;
}

bool Sck_ADS1X15::stop()
{
	started = false;
	return true;
}

bool Sck_ADS1X15::getReading(uint8_t wichChannel)
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	// Reset gain
	ads.setGain(GAIN_TWOTHIRDS);
	double voltage_range = 6.144;

	// Get value with full range
	uint16_t value = ads.readADC_SingleEnded(wichChannel);

	// If value is under 4.096v increase the gain depending on voltage
	if (value < 21845) {
		if (value > 10922) {

			// 1x gain, 4.096V
			ads.setGain(GAIN_ONE);
			voltage_range = 4.096;

		} else if (value > 5461) {

			// 2x gain, 2.048V
			ads.setGain(GAIN_TWO);
			voltage_range = 2.048;

		} else if (value > 2730) {

			// 4x gain, 1.024V
			ads.setGain(GAIN_FOUR);
			voltage_range = 1.024;

		} else if (value > 1365) {

			// 8x gain, 0.25V
			ads.setGain(GAIN_EIGHT);
			voltage_range = 0.512;

		} else {

			// 16x gain, 0.125V
			ads.setGain(GAIN_SIXTEEN);
			voltage_range = 0.256;
		}

		// Get the value again
		value = ads.readADC_SingleEnded(wichChannel);
	}

	reading = (float)value / 32768 * voltage_range;
	return true;
}

#ifdef adsTest
void Sck_ADS1X15::setTesterCurrent(int16_t wichCurrent, uint8_t wichChannel)
{
	// Support both combinations of ADC channels:
	// wichChannel = 0 (default) -> WE in ADS_Ch0 and AE in ADS_Ch1
	// wichChannel = 1 			 -> WE in ADS_Ch2 and AE in ADS_Ch3
	if (wichChannel > 0) {
		adsChannelW = 2;
		adsChannelA = 3;
	}

	SerialUSB.print("Setting test current to: ");
	SerialUSB.println(wichCurrent);

	tester.setCurrent(tester.electrode_W, wichCurrent);
	tester.setCurrent(tester.electrode_A, wichCurrent);

	SerialUSB.print("Tester Electrode W: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_W));
	SerialUSB.print("ISB W:");
	this->getReading(adsChannelW);
	SerialUSB.println(this->reading);

	SerialUSB.print("Tester Electrode A: ");
	SerialUSB.println(tester.getCurrent(tester.electrode_A));
	SerialUSB.print("ISB A: ");
	this->getReading(adsChannelA);
	SerialUSB.println(this->reading);

}

void Sck_ADS1X15::runTester(uint8_t wichChannel)
{
	// Support both combinations of ADC channels:
	// wichChannel = 0 (default) -> WE in ADS_Ch0 and AE in ADS_Ch1
	// wichChannel = 1 			 -> WE in ADS_Ch2 and AE in ADS_Ch3
	if (wichChannel > 0) {
		adsChannelW = 2;
		adsChannelA = 3;
	}

	// Print headers
	SerialUSB.println("testW,readW,testA,readA");

	// Output from -1400 to +1400 nA
	for (int16_t i=-1400; i<1400; i++) {
		tester.setCurrent(tester.electrode_W, i);
		double currVoltW = -10;

		if (this->getReading(adsChannelW))  currVoltW = this->reading;
		else SerialUSB.println("Error in Working electrode");

		// if (preVoltW != -99) if ((currVoltW - preVoltW) < threshold) maxErrorsW--;
		// preVoltW = currVoltW;
		// if (maxErrorsW == 0) SerialUSB.println("Working electrode fail !!!");

		tester.setCurrent(tester.electrode_A, i);
		double currVoltA = -10;

		if (this->getReading(adsChannelA)) currVoltA = this->reading;
		else SerialUSB.println("Error in Working electrode");

		// if (preVoltA != -99) if ((currVoltA - preVoltA) < threshold) maxErrorsA--;
		// preVoltA = currVoltA;
		// if (maxErrorsA == 0) SerialUSB.println("Auxiliary electrode fail !!!");

		SerialUSB.print(tester.getCurrent(tester.electrode_W));
		SerialUSB.print(",");
		SerialUSB.print(currVoltW, 8);
		SerialUSB.print(",");
		SerialUSB.print(tester.getCurrent(tester.electrode_A));
		SerialUSB.print(",");
		SerialUSB.println(currVoltA, 8);
	}
	SerialUSB.println("Run test finished!");
}
#endif

bool Sck_SCD30::start(SckBase *base, SensorType wichSensor)
{
	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	if (started) {
		// Mark this specific metric as enabled
		for (uint8_t i=0; i<3; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 1;
		return true;
	}

	if (_debug) sparkfun_scd30.enableDebugging(SerialUSB);

	// Without this delay sensor init fails sometimes
	delay(500);

	// Unset measbegin option to avoid begin() function to set measuring interval to default value of 2 seconds.
	if (!sparkfun_scd30.begin(auxWire, false, false)) return false;

	// Ambient pressure compensation
	OneSensor *pressureSensor = &base->sensors[SENSOR_PRESSURE];

	if (pressureSensor->enabled && base->getReading(pressureSensor)) {
		float pressureReading = pressureSensor->reading.toFloat();
		uint16_t pressureInMillibar = pressureReading * 10;

		if (pressureInMillibar > 700 && pressureInMillibar < 1200) {
			if (sparkfun_scd30.setAmbientPressure(pressureInMillibar)) {
				pressureCompensated = true;
			}
		}
	}

	// Start measuring with this function respects the saved interval
	if (!sparkfun_scd30.beginMeasuring()) return false;

	// Mark this specific metric as enabled
	for (uint8_t i=0; i<3; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 1;

	started = true;
	return true;
}

bool Sck_SCD30::stop(SensorType wichSensor)
{
	// Mark this specific metric as disabled
	for (uint8_t i=0; i<3; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 0;

	// Turn sensor off only if all 3 metrics are disabled
	for (uint8_t i=0; i<3; i++) {
		if (enabled[i][1] == 1) return false;
	}

	sparkfun_scd30.StopMeasurement();
	started = false;
	return true;
}

bool Sck_SCD30::getReading(SensorType wichSensor)
{
	switch (wichSensor)
	{
		case SENSOR_SCD30_CO2:
			co2 = sparkfun_scd30.getCO2();
			break;

		case SENSOR_SCD30_TEMP:
			temperature = sparkfun_scd30.getTemperature();
			break;

		case SENSOR_SCD30_HUM:
			humidity = sparkfun_scd30.getHumidity();
			break;

		default:
			return false;
	}

	return true;
}

uint16_t Sck_SCD30::interval(uint16_t newInterval)
{
	// Even if the sensor responds OK it doesn't seems to accept any value grater than 1000
	if (newInterval >= 2 && newInterval <= 1800) sparkfun_scd30.setMeasurementInterval(newInterval);

	uint16_t currentInterval;
	sparkfun_scd30.getMeasurementInterval(&currentInterval);

	// Restart measuring so we don't need to wait the current interval to finish (useful when you come from very long intervals)
	sparkfun_scd30.StopMeasurement();
	sparkfun_scd30.beginMeasuring();

	return currentInterval;
}

bool Sck_SCD30::autoSelfCal(int8_t value)
{
	// Value: 0 -> disable, 1 -> enable, any other -> get current setting

	if (value == 1)	sparkfun_scd30.setAutoSelfCalibration(true);
	else if (value == 0) sparkfun_scd30.setAutoSelfCalibration(false);

	return sparkfun_scd30.getAutoSelfCalibration();
}

uint16_t Sck_SCD30::forcedRecalFactor(uint16_t newFactor)
{
	if (newFactor >= 400 && newFactor <= 2000) {
		// Maybe not needed, but done for safety
		sparkfun_scd30.setAutoSelfCalibration(false);
		// Send command to SCD30
		sparkfun_scd30.setForcedRecalibrationFactor(newFactor);
	}
	uint16_t saved_value = 0;
	// Check saved value
	sparkfun_scd30.getForcedRecalibration(&saved_value);
	return saved_value;
}

float Sck_SCD30::tempOffset(float userTemp, bool off)
{
	// We expect from user the REAL temperature measured during calibration
	// We calculate the difference against the sensor measured temperature to set the correct offset. Please wait for sensor to stabilize temperatures before aplying an offset.
	// Temperature offset should always be positive (the sensor is generating heat)

	uint16_t currentOffsetTemp;
	sparkfun_scd30.getTemperatureOffset(&currentOffsetTemp);

	getReading(SENSOR_SCD30_TEMP);

	if (temperature > userTemp) sparkfun_scd30.setTemperatureOffset(temperature - userTemp);
	else if (off) sparkfun_scd30.setTemperatureOffset(0);

	sparkfun_scd30.getTemperatureOffset(&currentOffsetTemp);

	return currentOffsetTemp / 100.0;
}

void writeI2C(byte deviceaddress, byte instruction, byte data )
{
	auxWire.beginTransmission(deviceaddress);
	auxWire.write(instruction);
	auxWire.write(data);
	auxWire.endTransmission();
}

byte readI2C(byte deviceaddress, byte instruction)
{
	byte  data = 0x0000;
	auxWire.beginTransmission(deviceaddress);
	auxWire.write(instruction);
	auxWire.endTransmission();
	auxWire.requestFrom(deviceaddress,1);
	unsigned long time = millis();
	while (!auxWire.available()) if ((millis() - time)>500) return 0x00;
	data = auxWire.read();
	return data;
}
