#pragma once

#include <Arduino.h>

enum SensorLocation
{
	BOARD_BASE,
	BOARD_URBAN,
	BOARD_AUX
};

enum SensorType
{

	// Base Sensors
	SENSOR_BATT_PERCENT,
	SENSOR_BATT_VOLTAGE,
	SENSOR_BATT_CHARGE_RATE,
	SENSOR_BATT_POWER,
	SENSOR_SDCARD,

	// Urban Sensors
	SENSOR_LIGHT,
	SENSOR_TEMPERATURE,
	SENSOR_HUMIDITY,
	SENSOR_CO_RESISTANCE,
	SENSOR_CO_HEAT_VOLT,
	SENSOR_CO_HEAT_TIME,
	SENSOR_NO2_RESISTANCE,
	SENSOR_NO2_HEAT_VOLT,
	SENSOR_NO2_HEAT_TIME,
	SENSOR_NO2_LOAD_RESISTANCE,
	SENSOR_NOISE_DBA,
	SENSOR_NOISE_DBC,
	SENSOR_NOISE_DBZ,
	SENSOR_NOISE_FFT, 
	SENSOR_ALTITUDE,
	SENSOR_PRESSURE,
	SENSOR_PRESSURE_TEMP,
	SENSOR_PARTICLE_RED,
	SENSOR_PARTICLE_GREEN,
	SENSOR_PARTICLE_IR,
	SENSOR_PARTICLE_TEMPERATURE,
	SENSOR_PM_1,
	SENSOR_PM_25,
	SENSOR_PM_10,
	SENSOR_PN_03,
	SENSOR_PN_05,
	SENSOR_PN_1,
	SENSOR_PN_25,
	SENSOR_PN_5,
	SENSOR_PN_10,

	// I2C Auxiliary Sensors
	SENSOR_GASESBOARD_SLOT_1A,
	SENSOR_GASESBOARD_SLOT_1W,
	SENSOR_GASESBOARD_SLOT_2A,
	SENSOR_GASESBOARD_SLOT_2W,
	SENSOR_GASESBOARD_SLOT_3A,
	SENSOR_GASESBOARD_SLOT_3W,
	SENSOR_GASESBOARD_TEMPERATURE,
	SENSOR_GASESBOARD_HUMIDITY,

	SENSOR_GROOVE_I2C_ADC,

	SENSOR_INA219_BUSVOLT,
	SENSOR_INA219_SHUNT,
	SENSOR_INA219_CURRENT,
	SENSOR_INA219_LOADVOLT,

	SENSOR_WATER_TEMP_DS18B20,
	SENSOR_ATLAS_TEMPERATURE,
	SENSOR_ATLAS_PH,
	SENSOR_ATLAS_EC,
	SENSOR_ATLAS_EC_SG,
	SENSOR_ATLAS_DO,
	SENSOR_ATLAS_DO_SAT,

	SENSOR_CHIRP_MOISTURE,
	SENSOR_CHIRP_TEMPERATURE,

	SENSOR_EXT_PM_1,
	SENSOR_EXT_PM_25,
	SENSOR_EXT_PM_10,
	SENSOR_EXT_PN_03,
	SENSOR_EXT_PN_05,
	SENSOR_EXT_PN_1,
	SENSOR_EXT_PN_25,
	SENSOR_EXT_PN_5,
	SENSOR_EXT_PN_10,
	SENSOR_EXT_A_PM_1,
	SENSOR_EXT_A_PM_25,
	SENSOR_EXT_A_PM_10,
	SENSOR_EXT_A_PN_03,
	SENSOR_EXT_A_PN_05,
	SENSOR_EXT_A_PN_1,
	SENSOR_EXT_A_PN_25,
	SENSOR_EXT_A_PN_5,
	SENSOR_EXT_A_PN_10,
	SENSOR_EXT_B_PM_1,
	SENSOR_EXT_B_PM_25,
	SENSOR_EXT_B_PM_10,
	SENSOR_EXT_B_PN_03,
	SENSOR_EXT_B_PN_05,
	SENSOR_EXT_B_PN_1,
	SENSOR_EXT_B_PN_25,
	SENSOR_EXT_B_PN_5,
	SENSOR_EXT_B_PN_10,

	SENSOR_PM_DALLAS_TEMP,
	SENSOR_DALLAS_TEMP,
	
	SENSOR_SHT31_TEMP,
	SENSOR_SHT31_HUM,

	SENSOR_RANGE_LIGHT,
	SENSOR_RANGE_DISTANCE,

	SENSOR_BME680_TEMPERATURE,
	SENSOR_BME680_HUMIDITY,
	SENSOR_BME680_PRESSURE,
	SENSOR_BME680_VOCS,

	SENSOR_CCS811_VOCS,
	SENSOR_CCS811_ECO2,

	// Actuators (This is temp)
	SENSOR_GROOVE_OLED,

	SENSOR_COUNT
};

class OneSensor
{
	public:
		SensorLocation location;
		SensorType type;
		const char *shortTitle;
		const char *title;
		const char *unit;
		String reading;
		uint32_t lastReadingTime;
		bool valid;
		bool controllable;
		uint8_t id;
		uint8_t everyNint; 	 	// Read this sensor every N intervals (default 1)
		bool enabled;
		bool defaultEnabled;
		bool busy;
		uint8_t priority;

		OneSensor(SensorLocation nLocation, uint8_t nPriority, SensorType nType, const char *nShortTitle, const char *nTitle, uint8_t nId=0, bool nEnabled=false, bool nControllable=false, uint8_t nEveryNint=1, const char *nUnit="") {
			location = nLocation;
			priority = nPriority; 		// 0-250, 0:Max priority -> 250:Min priority
			type = nType;
			shortTitle = nShortTitle;
			title = nTitle;
			unit = nUnit;
			reading = "none";
			lastReadingTime = 0;
			valid = false;
			controllable = nControllable;
			id = nId;
			everyNint = nEveryNint;
			enabled = nEnabled; 
			defaultEnabled = nEnabled;
			busy = false;
		}
};

class AllSensors
{
	public:


		OneSensor list[SENSOR_COUNT+1] {

			//	SensorLocation 	priority	SensorType 				shortTitle		title 						id		enabled		controllable	everyNintervals		unit

			// Base Sensors
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_PERCENT,			"BATT",			"Battery", 					10,		true,		false,		1,			"%"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_VOLTAGE,			"BATT_VOLT",		"Battery voltage",				0,		false,		false,		1,			"V"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_CHARGE_RATE,		"BATT_CHG_RATE",	"Battery charge rate",				97,		false,		false,		1,			"mA"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_POWER,			"BATT_POWER",		"Battery power rate",				0,		false,		false,		1,			"mW"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_SDCARD,				"SDCARD",		"SDcard present", 				0,		false,		false,		1,			"Present"},

			// Urban Sensors
			OneSensor { BOARD_URBAN, 	100,	SENSOR_LIGHT, 				"LIGHT",		"Light", 					14,		true,		false,		1,			"Lux"},
			OneSensor { BOARD_URBAN, 	0,	SENSOR_TEMPERATURE, 			"TEMP",			"Temperature", 					55,		true,		false,		1,			"C"},
			OneSensor { BOARD_URBAN, 	0,	SENSOR_HUMIDITY,			"HUM",			"Humidity", 					56,		true,		false,		1,			"%"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_CO_RESISTANCE,			"CO_MICS_RAW",		"Carbon monoxide resistance", 			16,		false,		true,		1,			"kOhm"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_CO_HEAT_VOLT, 			"CO_MICS_VHEAT",	"Carbon monoxide heat voltage",			0,		false,		false,		1,			"V"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_CO_HEAT_TIME, 			"CO_MICS_THEAT",	"Carbon monoxide heat time",			0,		false,		false,		1,			"sec"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_NO2_RESISTANCE,			"NO2_MICS_RAW",		"Nitrogen dioxide resistance",			15,		false,		true,		1,			"kOhm"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_NO2_HEAT_VOLT, 			"NO2_MICS_VHEAT",	"Nitrogen dioxide heat voltage",		0,		false,		false,		1,			"V"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_NO2_HEAT_TIME, 			"NO2_MICS_THEAT",	"Nitrogen dioxide heat time",			0,		false,		false,		1,			"sec"},
			OneSensor { BOARD_URBAN, 	200,	SENSOR_NO2_LOAD_RESISTANCE, 		"NO2_MICS_RLOAD",	"Nitrogen dioxide load resistance",		0,		false,		false,		1,			"Ohms"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBA, 			"NOISE_A",		"Noise dBA", 					53,		true,		true,		1,			"dBA"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBC, 			"NOISE_B",		"Noise dBC", 					0,		false,		true,		1,			"dBC"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBZ, 			"NOISE_Z",		"Noise dBZ", 					0,		false,		true,		1,			"dB"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_FFT, 			"NOISE_FFT",		"Noise FFT", 					0,		false,		true,		1,			},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_ALTITUDE, 			"ALT", 			"Altitude", 					0,		false,		false,		1,			"M"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PRESSURE, 			"PRESS",		"Barometric pressure",				58,		true,		false,		1,			"kPa"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PRESSURE_TEMP,			"PRESS_TEMP",		"Pressure internal temperature", 		0,		false,		false,		1,			"C"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PARTICLE_RED, 			"DUST_RED",		"Dust particle Red Channel",	 		0,		false,		false,		1,			},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PARTICLE_GREEN,			"DUST_GREEN",		"Dust particle Green Channel",	 		0,		false,		false,		1,			},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PARTICLE_IR,			"DUST_IR",		"Dust particle InfraRed Channel",	 	0,		false,		false,		1,			},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PARTICLE_TEMPERATURE,		"DUST_TEMP",		"Dust particle internal temperature",		0,		false,		false,		1,			"C"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_1,				"PM_1",			"PM 1.0",					89,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_25,				"PM_25",		"PM 2.5",					87,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_10,				"PM_10",		"PM 10.0",					88,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_03,				"PN_03",		"PN 0.3",					0,		true,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_05,				"PN_05",		"PN 0.5",					0,		true,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_1,				"PN_1",			"PN 1.0",					0,		true,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_25,				"PN_25",		"PN 2.5",					0,		true,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_5,				"PN_5",			"PN 5.0",					0,		true,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_10,				"PN_10",		"PN 10.0",					0,		true,		false,		1,			"#/0.1l"},


			// I2C Auxiliary Sensors
			// SCK Gases Board for Alphasense (3 Gas sensor Slots, + SHT31 Temp-Humidity)
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_1A,		"GB_1A",		"Gases Board 1A",				65,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_1W,		"GB_1W",		"Gases Board 1W",				64,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_2A,		"GB_2A",		"Gases Board 2A",				62,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_2W, 		"GB_2W",		"Gases Board 2W",				61,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_3A, 		"GB_3A",		"Gases Board 3A",				68,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_3W, 		"GB_3W",		"Gases Board 3W",				67,		false,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_TEMPERATURE, 		"GB_TEMP",		"Gases Board Temperature", 			79,		false,		false,		1,			"C"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_HUMIDITY, 		"GB_HUM",		"Gases Board Humidity",				80,		false,		false,		1,			"%"},

			// Groove I2C ADC
			OneSensor { BOARD_AUX,		100,	SENSOR_GROOVE_I2C_ADC,			"GR_ADC",		"Groove ADC",					25,		false,		false,		1,			"V"},

			// Adafruit INA291 High Side DC Current Sensor
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_BUSVOLT,			"INA_VBUS",		"INA219 Bus voltage",				0,		false,		false,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_SHUNT,			"INA_VSHUNT",		"INA219 Shunt voltage",				0,		false,		false,		1,			"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_CURRENT,			"INA_CURR",		"INA219 Current",				0,		false,		false,		1,			"mA"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_LOADVOLT,			"INA_VLOAD",		"INA219 Load voltage",				0,		false,		false,		1,			"V"},

			OneSensor { BOARD_AUX,		100,	SENSOR_WATER_TEMP_DS18B20,		"DS_WAT_TEMP",		"DS18B20 Water temperature",			42,		false,		false,		1,			"C"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_ATLAS_TEMPERATURE, 		"AS_TEMP", 		"Atlas Temperature", 				51, 		false, 		false, 		1,			"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_PH,			"AS_PH",		"Atlas PH",					43,		false,		true,		1,			"pH"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC,			"AS_COND",		"Atlas Conductivity",				45,		false,		true,		1,			"uS/cm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_SG,			"AS_SG",		"Atlas Specific gravity",			46,		false,		true,		1,			},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO,			"AS_DO",		"Atlas Dissolved Oxygen",			48,		false,		true,		1,			"mg/L"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO_SAT,			"AS_DO_SAT",		"Atlas DO Saturation",				49,		false,		true,		1,			"%"},

			// I2C Moisture Sensor (chirp)
			// https://github.com/Miceuz/i2c-moisture-sensor
			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_MOISTURE, 			"CHRP_MOIS", 		"Soil Moisture", 				50, 		false, 		true, 		1,			},
			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_TEMPERATURE, 		"CHRP_TEMP", 		"Soil Temperature", 				0, 		false, 		true, 		1,			"C"},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PM_1,			"EXT_PM_1",		"Ext PM 1.0",					89,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PM_25,			"EXT_PM_25",		"Ext PM 2.5",					87,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PM_10,			"EXT_PM_10",		"Ext PM 10.0",					88,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_03,			"EXT_PN_03",		"Ext PN 0.3",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_05,			"EXT_PN_05",		"Ext PN 0.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_1,			"EXT_PN_1",		"Ext PN 1.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_25,			"EXT_PN_25",		"Ext PN 2.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_5,			"EXT_PN_5",		"Ext PN 5.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_PN_10,			"EXT_PN_10",		"Ext PN 10.0",					0,		false,		false,		1,			"#/0.1l"},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_1,			"EXT_PM_A_1",		"Ext PM_A 1.0",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_25,			"EXT_PM_A_25",		"Ext PM_A 2.5",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_10,			"EXT_PM_A_10",		"Ext PM_A 10.0",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_03,			"EXT_PN_A_03",		"Ext PN_A 0.3",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_05,			"EXT_PN_A_05",		"Ext PN_A 0.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_1,			"EXT_PN_A_1",		"Ext PN_A 1.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_25,			"EXT_PN_A_25",		"Ext PN_A 2.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_5,			"EXT_PN_A_5",		"Ext PN_A 5.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_10,			"EXT_PN_A_10",		"Ext PN_A 10.0",					0,		false,		false,		1,			"#/0.1l"},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_1,			"EXT_PM_B_1",		"Ext PM_B 1.0",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_25,			"EXT_PM_B_25",		"Ext PM_B 2.5",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_10,			"EXT_PM_B_10",		"Ext PM_B 10.0",					0,		false,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_03,			"EXT_PN_B_03",		"Ext PN_B 0.3",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_05,			"EXT_PN_B_05",		"Ext PN_B 0.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_1,			"EXT_PN_B_1",		"Ext PN_B 1.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_25,			"EXT_PN_B_25",		"Ext PN_B 2.5",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_5,			"EXT_PN_B_5",		"Ext PN_B 5.0",					0,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_10,			"EXT_PN_B_10",		"Ext PN_B 10.0",				0,		false,		false,		1,			"#/0.1l"},

			OneSensor { BOARD_AUX,		0,	SENSOR_PM_DALLAS_TEMP,			"PM_DALLAS_TEMP",	"PM board Dallas Temperature",			96,		false,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_DALLAS_TEMP,			"DALLAS_TEMP",		"Direct Dallas Temperature",			96,		false,		false,		1,			"C"},

			OneSensor { BOARD_AUX,		0,	SENSOR_SHT31_TEMP,			"EXT_TEMP",		"Ext Temperature",				79,		false,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_SHT31_HUM,			"EXT_HUM",		"Ext Humidity",					80,		false,		false,		1,			"%"},

			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_LIGHT,			"EXT_RANGE_LIGHT",	"Ext Range Light",				0,		false,		false,		1,			"Lux"},
			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_DISTANCE,			"EXT_RANGE_DIST",	"Ext Range Distance",				98,		false,		false,		1,			"mm"},

			OneSensor { BOARD_AUX,		0,	SENSOR_BME680_TEMPERATURE,		"BME680_TEMP",		"Temperature BME680",				0,		false,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_BME680_HUMIDITY,			"BME680_HUM",		"Humidity BME680",				0,		false,		false,		1,			"%"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_PRESSURE,			"BME680_PRESS",		"Barometric pressure BME680",			0,		false,		false,		1,			"kPa"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_VOCS,			"BME680_VOCS",		"VOC Gas BME680",				0,		false,		false,		1,			"Ohms"},

			OneSensor { BOARD_AUX,		100,	SENSOR_CCS811_VOCS,			"CCS811_VOCS",		"VOC Gas CCS811",				0,		false,		true,		1,			"ppb"},
			OneSensor { BOARD_AUX,		100,	SENSOR_CCS811_ECO2,			"CCS811_ECO2",		"eCO2 Gas CCS811",				0,		false,		true,		1,			"ppm"},

			// Later this will be moved to a Actuators.h file
			// Groove I2C Oled Display 96x96
			OneSensor { BOARD_AUX,		250,	SENSOR_GROOVE_OLED,			"GR_OLED",		"Groove OLED",					0,		false,		false,		1,			},
			OneSensor { BOARD_BASE, 	0,	SENSOR_COUNT,				"NOT_FOUND",		"Not found",					0,		false,		false,		1,			}

			// Add New Sensor Here!!!

		};

		OneSensor & operator[](SensorType type) {
			return list[type];
		}

		OneSensor ordered(uint8_t place);
		SensorType getTypeFromString(String strIn);
		String removeSensorName(String strIn);
		SensorType sensorsPriorized(uint8_t index);
	private:
		uint8_t countMatchedWords(String baseString, String input);
		SensorType prioSortedList[SENSOR_COUNT+1];
		bool sorted = false;
};
