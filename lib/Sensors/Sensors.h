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
	SENSOR_VOLTIN,

	// Urban Sensors
	SENSOR_LIGHT,
	SENSOR_TEMPERATURE,
	SENSOR_HUMIDITY,
	SENSOR_CO,
	SENSOR_CO_RESISTANCE,
	SENSOR_CO_HEAT_TIME,
	SENSOR_NO2,
	SENSOR_NO2_RESISTANCE,
	SENSOR_NO2_HEAT_TIME,
	SENSOR_NO2_LOAD_RESISTANCE,
	/* SENSOR_NOISE_DBA, */
	/* SENSOR_NOISE_DBC, */
	/* SENSOR_NOISE_DBZ, */
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

	// I2C Auxiliary Sensors
	SENSOR_GASESBOARD_SLOT_1A,
	SENSOR_GASESBOARD_SLOT_1W,
	SENSOR_GASESBOARD_SLOT_1_CAL,
	SENSOR_GASESBOARD_SLOT_2A,
	SENSOR_GASESBOARD_SLOT_2W,
	SENSOR_GASESBOARD_SLOT_2_CAL,
	SENSOR_GASESBOARD_SLOT_3A,
	SENSOR_GASESBOARD_SLOT_3W,
	SENSOR_GASESBOARD_SLOT_3_CAL,
	SENSOR_GASESBOARD_TEMPERATURE,
	SENSOR_GASESBOARD_HUMIDITY,

	SENSOR_GROOVE_I2C_ADC,

	SENSOR_INA219_BUSVOLT,
	SENSOR_INA219_SHUNT,
	SENSOR_INA219_CURRENT,
	SENSOR_INA219_LOADVOLT,

	SENSOR_WATER_TEMP_DS18B20,
	SENSOR_ATLAS_PH,
	SENSOR_ATLAS_EC,
	SENSOR_ATLAS_EC_SG,
	SENSOR_ATLAS_DO,
	SENSOR_ATLAS_DO_SAT,

	SENSOR_EXT_PM_1,
	SENSOR_EXT_PM_25,
	SENSOR_EXT_PM_10,
	
	SENSOR_SHT31_TEMP,
	SENSOR_SHT31_HUM,

	// Actuators (This is temp)
	SENSOR_GROOVE_OLED,

	SENSOR_COUNT
};

const uint32_t minimal_sensor_reading_interval = 10;
const uint32_t default_sensor_reading_interval = 30;
const uint32_t max_sensor_reading_interval = 86400;		// One day

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
		uint32_t interval;
		bool enabled;
		bool defaultEnabled;
		bool busy;

		OneSensor(SensorLocation nLocation, SensorType nType, const char *nShortTitle, const char *nTitle, uint8_t nId=0, bool nEnabled=false, bool nControllable=false, const char *nUnit="") {
			location = nLocation;
			type = nType;
			shortTitle = nShortTitle;
			title = nTitle;
			unit = nUnit;
			reading = "none";
			lastReadingTime = 0;
			valid = false;
			controllable = nControllable;
			id = nId;
			interval = default_sensor_reading_interval;
			enabled = nEnabled; 
			defaultEnabled = nEnabled;
			busy = false;
		}
};

class AllSensors
{
	public:

		OneSensor list[SENSOR_COUNT+1] {

			//		SensorLocation 		SensorType 				shortTitle		title 						id		enabled		controllable	unit

			// Base Sensors
			OneSensor { BOARD_BASE, 		SENSOR_BATT_PERCENT,			"BATT",			"Battery", 					10,		true,		false,			"%"},
			OneSensor { BOARD_BASE, 		SENSOR_BATT_VOLTAGE,			"BATT_VOLT",		"Battery voltage",				0,		false,		false,			"V"},
			OneSensor { BOARD_BASE, 		SENSOR_BATT_CHARGE_RATE,		"BATT_CHG_RATE",	"Battery charge rate",				0,		false,		false,			"mA"},
			OneSensor { BOARD_BASE, 		SENSOR_VOLTIN,				"INPUT_VOLT",		"Input voltage", 				0,		false,		false,			"V"},

			// Urban Sensors
			OneSensor { BOARD_URBAN, 		SENSOR_LIGHT, 				"LIGHT",		"Light", 					14,		true,		false,			"Lux"},
			OneSensor { BOARD_URBAN, 		SENSOR_TEMPERATURE, 			"TEMP",			"Temperature", 					55,		true,		false,			"C"},
			OneSensor { BOARD_URBAN, 		SENSOR_HUMIDITY,			"HUM",			"Humidity", 					56,		true,		false,			"%"},
			OneSensor { BOARD_URBAN, 		SENSOR_CO, 				"CO_MICS",		"Carbon monoxide", 				0,		false,		true,			"ppm"},
			OneSensor { BOARD_URBAN, 		SENSOR_CO_RESISTANCE,			"CO_MICS_RAW",		"Carbon monoxide resistance", 			82,		true,		true,			"kOhm"},
			/* OneSensor { BOARD_URBAN, 		SENSOR_CO_RESISTANCE,			"CO_MICS_RAW",		"Carbon monoxide resistance", 			16,		false,		true,			"kOhm"}, */
			OneSensor { BOARD_URBAN, 		SENSOR_CO_HEAT_TIME, 			"CO_MICS_THEAT",	"Carbon monoxide heat time",			0,		false,		false,			"sec"},
			OneSensor { BOARD_URBAN, 		SENSOR_NO2, 				"NO2_MICS",		"Nitrogen dioxide",				0,		false,		true,			"ppb"},
			OneSensor { BOARD_URBAN, 		SENSOR_NO2_RESISTANCE,			"NO2_MICS_RAW",		"Nitrogen dioxide resistance",			81,		true,		true,			"kOhm"},
			/* OneSensor { BOARD_URBAN, 		SENSOR_NO2_RESISTANCE,			"NO2_MICS_RAW",		"Nitrogen dioxide resistance",			15,		false,		true,			"kOhm"}, */
			OneSensor { BOARD_URBAN, 		SENSOR_NO2_HEAT_TIME, 			"NO2_MICS_THEAT",	"Nitrogen dioxide heat time",			0,		false,		false,			"sec"},
			OneSensor { BOARD_URBAN, 		SENSOR_NO2_LOAD_RESISTANCE, 		"NO2_MICS_RLOAD",	"Nitrogen dioxide load resistance",		0,		false,		false,			"Ohms"},
			/* OneSensor { BOARD_URBAN, 		SENSOR_NOISE_DBA, 			"NOISE_A",		"Noise dBA", 					53,		true,		false,			"dBA"}, */
			/* OneSensor { BOARD_URBAN, 		SENSOR_NOISE_DBC, 			"NOISE_B",		"Noise dBC", 					0,		false,		false,			"dBC"}, */
			/* OneSensor { BOARD_URBAN, 		SENSOR_NOISE_DBZ, 			"NOISE_Z",		"Noise dBZ", 					0,		false,		false,			"dB"}, */
			OneSensor { BOARD_URBAN, 		SENSOR_ALTITUDE, 			"ALT", 			"Altitude", 					0,		false,		false,			"M"},
			OneSensor { BOARD_URBAN, 		SENSOR_PRESSURE, 			"PRESS",		"Barometric pressure",				58,		true,		false,			"kPa"},
			OneSensor { BOARD_URBAN, 		SENSOR_PRESSURE_TEMP,			"PRESS_TEMP",		"Pressure internal temperature", 		0,		false,		false,			"C"},
			OneSensor { BOARD_URBAN, 		SENSOR_PARTICLE_RED, 			"DUST_RED",		"Dust particle Red Channel",	 		0,		false,		false,			},
			OneSensor { BOARD_URBAN, 		SENSOR_PARTICLE_GREEN,			"DUST_GREEN",		"Dust particle Green Channel",	 		0,		false,		false,			},
			OneSensor { BOARD_URBAN, 		SENSOR_PARTICLE_IR,			"DUST_IR",		"Dust particle InfraRed Channel",	 	0,		false,		false,			},
			OneSensor { BOARD_URBAN, 		SENSOR_PARTICLE_TEMPERATURE,		"DUST_TEMP",		"Dust particle internal temperature",		0,		false,		false,			"C"},
			OneSensor { BOARD_URBAN,		SENSOR_PM_1,				"PM_1",			"PM 1.0",					89,		true,		false,			"ug/m3"},
			OneSensor { BOARD_URBAN,		SENSOR_PM_25,				"PM_25",		"PM 2.5",					87,		true,		false,			"ug/m3"},
			OneSensor { BOARD_URBAN,		SENSOR_PM_10,				"PM_10",		"PM 10.0",					88,		true,		false,			"ug/m3"},


			// I2C Auxiliary Sensors
			// SCK Gases Board for Alphasense (3 Gas sensor Slots, + SHT31 Temp-Humidity)
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_1A,		"GB_1A",		"Gases Board 1A",				68,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_1W,		"GB_1W",		"Gases Board 1W",				67,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_1_CAL,		"GB_CO",		"Gases Board CO",				84,		false,		true,			"ppm"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_2A,		"GB_2A",		"Gases Board 2A",				62,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_2W, 		"GB_2W",		"Gases Board 2W",				61,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_2_CAL,		"GB_NO2",		"Gases Board NO2",				83,		false,		true,			"ppb"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_3A, 		"GB_3A",		"Gases Board 3A",				65,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_3W, 		"GB_3W",		"Gases Board 3W",				64,		false,		true,			"mV"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_SLOT_3_CAL,		"GB_O3",		"Gases Board O3",				85,		false,		true,			"ppb"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_TEMPERATURE, 		"GB_TEMP",		"Gases Board Temperature", 			79,		false,		false,			"C"},
			OneSensor { BOARD_AUX, 			SENSOR_GASESBOARD_HUMIDITY, 		"GB_HUM",		"Gases Board Humidity",				80,		false,		false,			"%"},

			// Groove I2C ADC
			OneSensor { BOARD_AUX,			SENSOR_GROOVE_I2C_ADC,			"GR_ADC",		"Groove ADC",					0,		false,		false,			"V"},

			// Adafruit INA291 High Side DC Current Sensor
			OneSensor { BOARD_AUX,			SENSOR_INA219_BUSVOLT,			"INA_VBUS",		"INA219 Bus voltage",				0,		false,		false,			"V"},
			OneSensor { BOARD_AUX,			SENSOR_INA219_SHUNT,			"INA_VSHUNT",		"INA219 Shunt voltage",				0,		false,		false,			"mV"},
			OneSensor { BOARD_AUX,			SENSOR_INA219_CURRENT,			"INA_CURR",		"INA219 Current",				0,		false,		false,			"mA"},
			OneSensor { BOARD_AUX,			SENSOR_INA219_LOADVOLT,			"INA_VLOAD",		"INA219 Load voltage",				0,		false,		false,			"V"},

			OneSensor { BOARD_AUX,			SENSOR_WATER_TEMP_DS18B20,		"DS_WAT_TEMP",		"DS18B20 Water temperature",			42,		false,		false,			"C"},
			OneSensor { BOARD_AUX,			SENSOR_ATLAS_PH,			"AS_PH",		"Atlas PH",					43,		false,		true,			"pH"},
			OneSensor { BOARD_AUX,			SENSOR_ATLAS_EC,			"AS_COND",		"Atlas Conductivity",				45,		false,		true,			"uS/cm"},
			OneSensor { BOARD_AUX,			SENSOR_ATLAS_EC_SG,			"AS_SG",		"Atlas Specific gravity",			46,		false,		true,			},
			OneSensor { BOARD_AUX,			SENSOR_ATLAS_DO,			"AS_DO",		"Atlas Dissolved Oxygen",			48,		false,		true,			"mg/L"},
			OneSensor { BOARD_AUX,			SENSOR_ATLAS_DO_SAT,			"AS_DO_SAT",		"Atlas DO Saturation",				49,		false,		true,			"%"},

			OneSensor { BOARD_AUX,			SENSOR_EXT_PM_1,			"EXT_PM_1",		"External PM 1.0",				89,		false,		false,			"ug/m3"},
			OneSensor { BOARD_AUX,			SENSOR_EXT_PM_25,			"EXT_PM_25",		"External PM 2.5",				87,		false,		false,			"ug/m3"},
			OneSensor { BOARD_AUX,			SENSOR_EXT_PM_10,			"EXT_PM_10",		"External PM 10.0",				88,		false,		false,			"ug/m3"},

			OneSensor { BOARD_AUX,			SENSOR_SHT31_TEMP,			"EXT_TEMP",		"External Temperature",				0,		false,		false,			"C"},
			OneSensor { BOARD_AUX,			SENSOR_SHT31_HUM,			"EXT_HUM",		"External Humidity",				0,		false,		false,			"%"},
			// Later this will be moved to a Actuators.h file
			// Groove I2C Oled Display 96x96
			OneSensor { BOARD_AUX,			SENSOR_GROOVE_OLED,			"GR_OLED",		"Groove OLED",					0,		false,		false,			},
			OneSensor { BOARD_BASE, 		SENSOR_COUNT,				"NOT_FOUND",		"Not found",					0,		false,		false,			}

			// Add New Sensor Here!!!

		};

		OneSensor & operator[](SensorType type) {
			return list[type];
		}

		SensorType getTypeFromString(String strIn);

	private:
		uint8_t countMatchedWords(String baseString, String input);

};
