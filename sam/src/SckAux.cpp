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
Moisture 		moistureChirp;
PMsensor		pmSensor = PMsensor();
PM_DallasTemp 		pmDallasTemp;
Sck_DallasTemp 		dallasTemp;
Sck_SHT31 		sht31 = Sck_SHT31(&auxWire);
Sck_Range 		range;
Sck_BME680 		bme680;
Sck_ADS1X15 		ads48;
Sck_ADS1X15 		ads49;
Sck_ADS1X15 		ads4A;
Sck_ADS1X15 		ads4B;

// Eeprom flash emulation to store I2C address
FlashStorage(eepromAuxData, EepromAuxData);

bool AuxBoards::start(SensorType wichSensor)
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
		case SENSOR_EXT_A_PN_10:
		case SENSOR_EXT_B_PM_1:
		case SENSOR_EXT_B_PM_25:
		case SENSOR_EXT_B_PM_10:
		case SENSOR_EXT_B_PN_03:
		case SENSOR_EXT_B_PN_05:
		case SENSOR_EXT_B_PN_1:
		case SENSOR_EXT_B_PN_25:
		case SENSOR_EXT_B_PN_5:
		case SENSOR_EXT_B_PN_10:
		case SENSOR_EXT_PM_1:
		case SENSOR_EXT_PM_25:
		case SENSOR_EXT_PM_10:
		case SENSOR_EXT_PN_03:
		case SENSOR_EXT_PN_05:
		case SENSOR_EXT_PN_1:
		case SENSOR_EXT_PN_25:
		case SENSOR_EXT_PN_5:
		case SENSOR_EXT_PN_10:			return pmSensor.start(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.start(); break;
		case SENSOR_DALLAS_TEMP: 		return dallasTemp.start(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM:
			if (sht31.start() && !gasBoard.start()) return true;
			else return false;
			break;
		case SENSOR_RANGE_DISTANCE: 		return range.start(); break;
		case SENSOR_RANGE_LIGHT: 		return range.start(); break;
		case SENSOR_BME680_TEMPERATURE:		return bme680.start(); break;
		case SENSOR_BME680_HUMIDITY:		return bme680.start(); break;
		case SENSOR_BME680_PRESSURE:		return bme680.start(); break;
		case SENSOR_BME680_VOCS:		return bme680.start(); break;
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
		case SENSOR_EXT_A_PN_10:
		case SENSOR_EXT_B_PM_1:
		case SENSOR_EXT_B_PM_25:
		case SENSOR_EXT_B_PM_10:
		case SENSOR_EXT_B_PN_03:
		case SENSOR_EXT_B_PN_05:
		case SENSOR_EXT_B_PN_1:
		case SENSOR_EXT_B_PN_25:
		case SENSOR_EXT_B_PN_5:
		case SENSOR_EXT_B_PN_10:
		case SENSOR_EXT_PM_1:
		case SENSOR_EXT_PM_25:
		case SENSOR_EXT_PM_10:
		case SENSOR_EXT_PN_03:
		case SENSOR_EXT_PN_05:
		case SENSOR_EXT_PN_1:
		case SENSOR_EXT_PN_25:
		case SENSOR_EXT_PN_5:
		case SENSOR_EXT_PN_10:			return pmSensor.stop(); break;
		case SENSOR_PM_DALLAS_TEMP: 		return pmDallasTemp.stop(); break;
		case SENSOR_DALLAS_TEMP: 		return dallasTemp.stop(); break;
		case SENSOR_SHT31_TEMP:
		case SENSOR_SHT31_HUM: 			return sht31.stop(); break;
		case SENSOR_RANGE_DISTANCE: 		return range.stop(); break;
		case SENSOR_RANGE_LIGHT: 		return range.stop(); break;
		case SENSOR_BME680_TEMPERATURE:		return bme680.stop(); break;
		case SENSOR_BME680_HUMIDITY:		return bme680.stop(); break;
		case SENSOR_BME680_PRESSURE:		return bme680.stop(); break;
		case SENSOR_BME680_VOCS:		return bme680.stop(); break;
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
		case SENSOR_GROVE_OLED: 		return groove_OLED.stop(); break;
		default: break;
	}

	return false;
}

void AuxBoards::getReading(OneSensor *wichSensor)
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
		case SENSOR_CHIRP_MOISTURE_RAW:		if (moistureChirp.getReading(SENSOR_CHIRP_MOISTURE_RAW)) { wichSensor->reading = String(moistureChirp.raw); return; } break;
		case SENSOR_CHIRP_MOISTURE:		if (moistureChirp.getReading(SENSOR_CHIRP_MOISTURE)) { wichSensor->reading = String(moistureChirp.moisture); return; } break;
		case SENSOR_CHIRP_TEMPERATURE:		if (moistureChirp.getReading(SENSOR_CHIRP_TEMPERATURE)) { wichSensor->reading = String(moistureChirp.temperature); return; } break;
		case SENSOR_CHIRP_LIGHT:		if (moistureChirp.getReading(SENSOR_CHIRP_LIGHT)) { wichSensor->reading = String(moistureChirp.light); return; } break;
		case SENSOR_EXT_A_PM_1:
		case SENSOR_EXT_A_PM_25:
		case SENSOR_EXT_A_PM_10:
		case SENSOR_EXT_A_PN_03:
		case SENSOR_EXT_A_PN_05:
		case SENSOR_EXT_A_PN_1:
		case SENSOR_EXT_A_PN_25:
		case SENSOR_EXT_A_PN_5:
		case SENSOR_EXT_A_PN_10:		wichSensor->reading = String(pmSensor.getReading(SLOT_A, wichSensor->type)); return;
		case SENSOR_EXT_B_PM_1:
		case SENSOR_EXT_B_PM_25:
		case SENSOR_EXT_B_PM_10:
		case SENSOR_EXT_B_PN_03:
		case SENSOR_EXT_B_PN_05:
		case SENSOR_EXT_B_PN_1:
		case SENSOR_EXT_B_PN_25:
		case SENSOR_EXT_B_PN_5:
		case SENSOR_EXT_B_PN_10: 		wichSensor->reading = String(pmSensor.getReading(SLOT_B, wichSensor->type)); return;
		case SENSOR_EXT_PM_1:
		case SENSOR_EXT_PM_25:
		case SENSOR_EXT_PM_10:
		case SENSOR_EXT_PN_03:
		case SENSOR_EXT_PN_05:
		case SENSOR_EXT_PN_1:
		case SENSOR_EXT_PN_25:
		case SENSOR_EXT_PN_5:
		case SENSOR_EXT_PN_10: 			wichSensor->reading = String(pmSensor.getReading(SLOT_AVG, wichSensor->type)); return;
		case SENSOR_PM_DALLAS_TEMP: 		wichSensor->reading = String(pmDallasTemp.getReading()); return;
		case SENSOR_DALLAS_TEMP: 		if (dallasTemp.getReading()) 			{ wichSensor->reading = String(dallasTemp.reading); return; } break;
		case SENSOR_SHT31_TEMP: 		if (sht31.getReading()) 				{ wichSensor->reading = String(sht31.temperature); return; } break;
		case SENSOR_SHT31_HUM: 			if (sht31.getReading()) 				{ wichSensor->reading = String(sht31.humidity); return; } break;
		case SENSOR_RANGE_DISTANCE: 		if (range.getReading(SENSOR_RANGE_DISTANCE)) 	{ wichSensor->reading = String(range.readingDistance); return; } break;
		case SENSOR_RANGE_LIGHT: 		if (range.getReading(SENSOR_RANGE_LIGHT)) 	{ wichSensor->reading = String(range.readingLight); return; } break;
		case SENSOR_BME680_TEMPERATURE:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.temperature); return; } break;
		case SENSOR_BME680_HUMIDITY:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.humidity); return; } break;
		case SENSOR_BME680_PRESSURE:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.pressure); return; } break;
		case SENSOR_BME680_VOCS:		if (bme680.getReading()) 			{ wichSensor->reading = String(bme680.VOCgas); return; } break;
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

		}
		case SENSOR_ATLAS_PH:
		case SENSOR_ATLAS_EC:
		case SENSOR_ATLAS_EC_TDS:
		case SENSOR_ATLAS_EC_SAL:
		case SENSOR_ATLAS_EC_SG:
		case SENSOR_ATLAS_DO:
		case SENSOR_ATLAS_DO_SAT: {

			Atlas *thisAtlas = &atlasPH;
			if (wichSensor == SENSOR_ATLAS_EC || wichSensor == SENSOR_ATLAS_EC_SG) thisAtlas = &atlasEC;
			else if (wichSensor == SENSOR_ATLAS_DO || wichSensor == SENSOR_ATLAS_DO_SAT) thisAtlas = &atlasDO;

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
			if (command.startsWith("com")) {

				command.replace("com", "");
				command.trim();
				thisAtlas->sendCommand((char*)command.c_str());

				uint8_t responseCode = thisAtlas->getResponse();
				if (responseCode == 254) delay(1000); responseCode = thisAtlas->getResponse();
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

					uint8_t error;
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

		} case SENSOR_EXT_PM_1:
		case SENSOR_EXT_PM_25:
		case SENSOR_EXT_PM_10: {

			if (command.startsWith("stop")) {

				if (pmSensor.stop()) return ("Stoping PM sensors...");
				else return ("Failed stoping PM sensor!!");

			} else if (command.startsWith("start")) {

				if (pmSensor.start()) return ("Starting PM sensors...");
				else return ("Failed starting PM sensor!!");

			}
			break;
		} default: return "Unrecognized sensor!!!"; break;
	}
	return "Unknown error on control command!!!";
}

void AuxBoards::print(char *payload)
{

	groove_OLED.print(payload);
}

void AuxBoards::updateDisplay(SckBase* base)
{
	groove_OLED.update(base);
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

void Groove_OLED::update(SckBase* base)
{
	if (millis() - lastUpdate < refreshRate) return;
	lastUpdate = millis();

	// Info bar
	drawBar(base);

	// Setup mode screen
	if (base->st.onSetup) drawSetup(base);

	// Error popup (at the end because it goes on top)
	drawError(base->st.error);
}

void Groove_OLED::drawBar(SckBase* base)
{
	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);

	// Clear the buffer are of the bar
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

	if (wichError != ERROR_NONE) {

		u8g2_oled.setFont(u8g2_font_7x13B_mr);
		uint8_t font_h = u8g2_oled.getMaxCharHeight();

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
		}

		// Print message
		u8g2_oled.drawStr(19, 125, errorMsg);
	}

	lastError = wichError;

	// Update display
	u8g2_oled.updateDisplayArea(0, 14, 16, 2);
}

void Groove_OLED::drawSetup(SckBase* base)
{
	// Clear buffer (except info bar)
	uint8_t *buffStart = u8g2_oled.getBufferPtr();
	memset(&buffStart[256], 0, 1792);

	uint8_t font_h = u8g2_oled.getMaxCharHeight();

	u8g2_oled.setFont(u8g2_font_nine_by_five_nbp_tr);
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

 	while ( !DS_bridge.wireSearch(addr)) {

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
		for ( int i = 0; i < 9; i++) data[i] = DS_bridge.wireReadByte();

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

	if (!I2Cdetect(&auxWire, deviceAddress)) return false;

	if (beginDone) return true;
	beginDone = true;

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

					if (PH || TEMP)	newReading[0] = atlasResponse.toFloat();
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
	if (waterTemp_DS18B20.detected) temperature = waterTemp_DS18B20.getReading();
	else if (atlasTEMP.detected) {

		if (millis() - atlasTEMP.lastUpdate > 10000) {
			while (atlasTEMP.getBusyState()) delay(2);
		}

		char data[10];
		temperature = atlasTEMP.newReading[0];
		sprintf(data,"T,%.2f",temperature);

		if (!sendCommand(data)) return false;
	}

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

	auxWire.requestFrom(deviceAddress, 20, 1);
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
	auxWire.write(PM_START);
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
	auxWire.write(PM_STOP);
	auxWire.endTransmission();

	started = false;
	return true;
}

float PMsensor::getReading(PMslot slot, SensorType wichSensor)
{
	if (millis() - lastReading > 1000 || lastSlot != slot) {
		// Ask for reading
		auxWire.beginTransmission(deviceAddress);
		switch (slot) {
			case SLOT_A: auxWire.write(GET_PMA); break;
			case SLOT_B: auxWire.write(GET_PMB); break;
			case SLOT_AVG: auxWire.write(GET_PM_AVG); break;
		}
		auxWire.endTransmission();
		delay(2);

		// Get the reading
		auxWire.requestFrom(deviceAddress, valuesSize);
		uint32_t time = millis();
		while (!auxWire.available()) if ((millis() - time)>500) return 0x00;

		bool isError = true;
		for (uint8_t i=0; i<valuesSize; i++) {
			values[i] = auxWire.read();
			if (values[i] != 255) isError = false;
		}

		if (isError) return -9999;

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
		lastSlot = slot;
	}


	switch(wichSensor) {
		case SENSOR_EXT_PM_1:
		case SENSOR_EXT_A_PM_1:
		case SENSOR_EXT_B_PM_1: return pm1; break;
		case SENSOR_EXT_PM_25:
		case SENSOR_EXT_A_PM_25:
		case SENSOR_EXT_B_PM_25: return pm25; break;
		case SENSOR_EXT_PM_10:
		case SENSOR_EXT_A_PM_10:
		case SENSOR_EXT_B_PM_10: return pm10; break;
		case SENSOR_EXT_PN_03:
		case SENSOR_EXT_A_PN_03:
		case SENSOR_EXT_B_PN_03: return pn03; break;
		case SENSOR_EXT_PN_05:
		case SENSOR_EXT_A_PN_05:
		case SENSOR_EXT_B_PN_05: return pn05; break;
		case SENSOR_EXT_PN_1:
		case SENSOR_EXT_A_PN_1:
		case SENSOR_EXT_B_PN_1: return pn1; break;
		case SENSOR_EXT_PN_25:
		case SENSOR_EXT_A_PN_25:
		case SENSOR_EXT_B_PN_25: return pn25; break;
		case SENSOR_EXT_PN_5:
		case SENSOR_EXT_A_PN_5:
		case SENSOR_EXT_B_PN_5: return pn5; break;
		case SENSOR_EXT_PN_10:
		case SENSOR_EXT_A_PN_10:
		case SENSOR_EXT_B_PN_10: return pn10; break;
		default:break;
	}

	return -9999;
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
	// Reset gain
	ads.setGain(GAIN_TWOTHIRDS);
	double voltage_range = 6.144;

	// Get value with full range
	int16_t value = ads.readADC_SingleEnded(wichChannel);

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
			voltage_range = 0.25;

		} else {

			// 16x gain, 0.125V
			ads.setGain(GAIN_SIXTEEN);
			voltage_range = 0.125;
		}

		// Get the value again
		value = ads.readADC_SingleEnded(wichChannel);
	}

	reading = (float)value / 32768 * voltage_range;
	return true;
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
