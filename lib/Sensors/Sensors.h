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
	SENSOR_SDCARD,

	// Urban Sensors
	SENSOR_LIGHT,
	SENSOR_TEMPERATURE,
	SENSOR_HUMIDITY,
	SENSOR_NOISE_DBA,
	SENSOR_NOISE_DBC,
	SENSOR_NOISE_DBZ,
	SENSOR_NOISE_FFT,
	SENSOR_ALTITUDE,
	SENSOR_PRESSURE,
	SENSOR_PRESSURE_TEMP,
	SENSOR_CCS811_VOCS,
	SENSOR_CCS811_ECO2,
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
	SENSOR_ATLAS_EC_TDS,
	SENSOR_ATLAS_EC_SAL,
	SENSOR_ATLAS_EC_SG,
	SENSOR_ATLAS_DO,
	SENSOR_ATLAS_DO_SAT,

	SENSOR_CHIRP_MOISTURE_RAW,
	SENSOR_CHIRP_MOISTURE,
	SENSOR_CHIRP_TEMPERATURE,
	SENSOR_CHIRP_LIGHT,

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

	SENSOR_GPS_FIX_QUALITY,
	SENSOR_GPS_LATITUDE,
	SENSOR_GPS_LONGITUDE,
	SENSOR_GPS_ALTITUDE,
	SENSOR_GPS_SPEED,
	SENSOR_GPS_HDOP,
	SENSOR_GPS_SATNUM,

	SENSOR_ADS1X15_48_0,
	SENSOR_ADS1X15_48_1,
	SENSOR_ADS1X15_48_2,
	SENSOR_ADS1X15_48_3,
	SENSOR_ADS1X15_49_0,
	SENSOR_ADS1X15_49_1,
	SENSOR_ADS1X15_49_2,
	SENSOR_ADS1X15_49_3,
	SENSOR_ADS1X15_4A_0,
	SENSOR_ADS1X15_4A_1,
	SENSOR_ADS1X15_4A_2,
	SENSOR_ADS1X15_4A_3,
	SENSOR_ADS1X15_4B_0,
	SENSOR_ADS1X15_4B_1,
	SENSOR_ADS1X15_4B_2,
	SENSOR_ADS1X15_4B_3,

	// Actuators (This is temp)
	SENSOR_GROVE_OLED,

	SENSOR_COUNT
};

class OneSensor
{
	public:
		SensorLocation location;
		uint8_t priority; 		// 0-249 lower first. 250 is reserved for sensor we don't want to read (actuators, ej. oled screen)
		SensorType type;
		const char *shortTitle;
		const char *title;
		const char *unit;
		String reading;
		uint32_t lastReadingTime;
		bool controllable;
		uint8_t id;
		uint8_t everyNint; 	 	// Read this sensor every N intervals (default 1)
		uint8_t defaultEveryNint;
		bool enabled;
		bool defaultEnabled;
		int16_t state; 		// -1:error on reading, 0:reading OK, >0:number of seconds until the reading is OK
		bool oled_display;

		OneSensor(SensorLocation nLocation, uint8_t nPriority, SensorType nType, const char *nShortTitle, const char *nTitle, uint8_t nId=0, bool nEnabled=false, bool nControllable=false, uint8_t nEveryNint=1, const char *nUnit="", bool nOled_display=true) {
			location = nLocation;
			priority = nPriority; 		// 0-250, 0:Max priority -> 250:Min priority
			type = nType;
			shortTitle = nShortTitle;
			title = nTitle;
			unit = nUnit;
			reading = "null";
			lastReadingTime = 0;
			controllable = nControllable;
			id = nId;
			everyNint = nEveryNint;
			defaultEveryNint = nEveryNint;
			enabled = nEnabled;
			defaultEnabled = nEnabled;
			state = -1;
			oled_display = nOled_display;
		}
};

class AllSensors
{
	public:


		OneSensor list[SENSOR_COUNT+1] {

			//	SensorLocation 	priority	SensorType 				shortTitle		title 						id		enabled		controllable	everyNintervals		unit

			// Base Sensors
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_PERCENT,			"BATT",			"Battery", 					10,		true,		false,		1, 		 	"%"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_BATT_VOLTAGE,			"BATT_VOLT",		"Battery voltage",				0,		false,		false,		1, 			"V"},
			OneSensor { BOARD_BASE, 	100,	SENSOR_SDCARD,				"SDCARD",		"SDcard present", 				0,		false,		false,		1, 			"Present"},

			// Urban Sensors
			OneSensor { BOARD_URBAN, 	100,	SENSOR_LIGHT, 				"LIGHT",		"Light", 					14,		true,		false,		1, 			"Lux"},
			OneSensor { BOARD_URBAN, 	0,	SENSOR_TEMPERATURE, 			"TEMP",			"Temperature", 					55,		true,		false,		1, 			"C"},
			OneSensor { BOARD_URBAN, 	0,	SENSOR_HUMIDITY,			"HUM",			"Humidity", 					56,		true,		false,		1, 			"%"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBA, 			"NOISE_A",		"Noise dBA", 					53,		true,		true,		1, 			"dBA"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBC, 			"NOISE_B",		"Noise dBC", 					0,		false,		true,		1, 			"dBC"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_DBZ, 			"NOISE_Z",		"Noise dBZ", 					0,		false,		true,		1, 			"dB"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_NOISE_FFT, 			"NOISE_FFT",		"Noise FFT", 					0,		false,		true,		1, 			},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_ALTITUDE, 			"ALT", 			"Altitude", 					0,		false,		false,		1, 			"M"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PRESSURE, 			"PRESS",		"Barometric pressure",				58,		true,		false,		1, 			"kPa"},
			OneSensor { BOARD_URBAN, 	100,	SENSOR_PRESSURE_TEMP,			"PRESS_TEMP",		"Pressure internal temperature", 		0,		false,		false,		1, 			"C"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_CCS811_VOCS,			"CCS811_VOCS",		"VOC Gas CCS811",				113,		true,		true,		1, 			"ppb"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_CCS811_ECO2,			"CCS811_ECO2",		"eCO2 Gas CCS811",				112,		true,		true,		1, 			"ppm"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_1,				"PM_1",			"PM 1.0",					89,		true,		false,		5, 			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_25,				"PM_25",		"PM 2.5",					87,		true,		false,		5, 			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_10,				"PM_10",		"PM 10.0",					88,		true,		false,		5, 			"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_03,				"PN_03",		"PN 0.3",					0,		false,		false,		1, 			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_05,				"PN_05",		"PN 0.5",					0,		false,		false,		1, 			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_1,				"PN_1",			"PN 1.0",					0,		false,		false,		1, 			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_25,				"PN_25",		"PN 2.5",					0,		false,		false,		1, 			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_5,				"PN_5",			"PN 5.0",					0,		false,		false,		1, 			"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_10,				"PN_10",		"PN 10.0",					0,		false,		false,		1, 			"#/0.1l"},

			// I2C Auxiliary Sensors
			// SCK Gases Board for Alphasense (3 Gas sensor Slots, + SHT31 Temp-Humidity)
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_1A,		"GB_1A",		"Gases Board 1A",				65,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_1W,		"GB_1W",		"Gases Board 1W",				64,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_2A,		"GB_2A",		"Gases Board 2A",				62,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_2W, 		"GB_2W",		"Gases Board 2W",				61,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_3A, 		"GB_3A",		"Gases Board 3A",				68,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_SLOT_3W, 		"GB_3W",		"Gases Board 3W",				67,		true,		true,		1,			"mV"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_TEMPERATURE, 		"GB_TEMP",		"Gases Board Temperature", 			79,		true,		false,		1,			"C"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_GASESBOARD_HUMIDITY, 		"GB_HUM",		"Gases Board Humidity",				80,		true,		false,		1,			"%"},

			// Groove I2C ADC
			OneSensor { BOARD_AUX,		100,	SENSOR_GROOVE_I2C_ADC,			"GR_ADC",		"Groove ADC",					25,		true,		false,		1,			"V"},

			// Adafruit INA291 High Side DC Current Sensor
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_BUSVOLT,			"INA_VBUS",		"INA219 Bus voltage",				0,		true,		false,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_SHUNT,			"INA_VSHUNT",		"INA219 Shunt voltage",				0,		true,		false,		1,			"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_CURRENT,			"INA_CURR",		"INA219 Current",				0,		true,		false,		1,			"mA"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_LOADVOLT,			"INA_VLOAD",		"INA219 Load voltage",				0,		true,		false,		1,			"V"},

			OneSensor { BOARD_AUX,		100,	SENSOR_WATER_TEMP_DS18B20,		"DS_WAT_TEMP",		"DS18B20 Water temperature",			42,		true,		false,		1,			"C"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_ATLAS_TEMPERATURE, 		"AS_TEMP", 		"Atlas Temperature", 				51, 		true, 		false, 		1,			"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_PH,			"AS_PH",		"Atlas PH",					43,		true,		true,		1,			"pH"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC,			"AS_COND",		"Atlas Conductivity",				45,		true,		true,		1,			"uS/cm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_TDS,			"AS_TDS",		"Atlas Total Dissolved Solids",			122,		true,		true,		1,			"ppm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_SAL,			"AS_SAL",		"Atlas Salinity",				123,		true,		true,		1,			"PSU(ppt)"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_SG,			"AS_SG",		"Atlas Specific gravity",			46,		true,		true,		1,			},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO,			"AS_DO",		"Atlas Dissolved Oxygen",			48,		true,		true,		1,			"mg/L"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO_SAT,			"AS_DO_SAT",		"Atlas DO Saturation",				49,		true,		true,		1,			"%"},

			// I2C Moisture Sensor (chirp)
			// https://github.com/Miceuz/i2c-moisture-sensor

			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_MOISTURE_RAW, 		"CHRP_MOIS_RAW",	"Soil Moisture Raw", 				0, 		true, 		true, 		1,			},
			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_MOISTURE, 			"CHRP_MOIS", 		"Soil Moisture Percent",			50, 		true, 		true, 		1,			"%"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_TEMPERATURE, 		"CHRP_TEMP", 		"Soil Temperature", 				0, 		true, 		true, 		1,			"C"},
			OneSensor { BOARD_AUX, 		100,	SENSOR_CHIRP_LIGHT, 	 		"CHRP_LIGHT", 		"Soil Light", 					0, 		true, 		true, 		1,			},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_1,			"EXT_PM_A_1",		"Ext PM_A 1.0",					71,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_25,			"EXT_PM_A_25",		"Ext PM_A 2.5",					72,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_10,			"EXT_PM_A_10",		"Ext PM_A 10.0",				73,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_03,			"EXT_PN_A_03",		"Ext PN_A 0.3",					99,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_05,			"EXT_PN_A_05",		"Ext PN_A 0.5",					100,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_1,			"EXT_PN_A_1",		"Ext PN_A 1.0",					101,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_25,			"EXT_PN_A_25",		"Ext PN_A 2.5",					102,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_5,			"EXT_PN_A_5",		"Ext PN_A 5.0",					103,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_10,			"EXT_PN_A_10",		"Ext PN_A 10.0",				104,		false,		false,		1,			"#/0.1l"},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_1,			"EXT_PM_B_1",		"Ext PM_B 1.0",					75,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_25,			"EXT_PM_B_25",		"Ext PM_B 2.5",					76,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_10,			"EXT_PM_B_10",		"Ext PM_B 10.0",				77,		true,		false,		1,			"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_03,			"EXT_PN_B_03",		"Ext PN_B 0.3",					105,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_05,			"EXT_PN_B_05",		"Ext PN_B 0.5",					106,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_1,			"EXT_PN_B_1",		"Ext PN_B 1.0",					107,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_25,			"EXT_PN_B_25",		"Ext PN_B 2.5",					108,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_5,			"EXT_PN_B_5",		"Ext PN_B 5.0",					109,		false,		false,		1,			"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_10,			"EXT_PN_B_10",		"Ext PN_B 10.0",				110,		false,		false,		1,			"#/0.1l"},

			OneSensor { BOARD_AUX,		0,	SENSOR_PM_DALLAS_TEMP,			"PM_DALLAS_TEMP",	"PM board Dallas Temperature",			96,		true,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_DALLAS_TEMP,			"DALLAS_TEMP",		"Direct Dallas Temperature",			96,		true,		false,		1,			"C"},

			OneSensor { BOARD_AUX,		0,	SENSOR_SHT31_TEMP,			"EXT_TEMP",		"Ext Temperature",				79,		true,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_SHT31_HUM,			"EXT_HUM",		"Ext Humidity",					80,		true,		false,		1,			"%"},

			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_LIGHT,			"EXT_RANGE_LIGHT",	"Ext Range Light",				0,		true,		false,		1,			"Lux"},
			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_DISTANCE,			"EXT_RANGE_DIST",	"Ext Range Distance",				98,		true,		false,		1,			"mm"},

			OneSensor { BOARD_AUX,		0,	SENSOR_BME680_TEMPERATURE,		"BME680_TEMP",		"Temperature BME680",				0,		true,		false,		1,			"C"},
			OneSensor { BOARD_AUX,		0,	SENSOR_BME680_HUMIDITY,			"BME680_HUM",		"Humidity BME680",				0,		true,		false,		1,			"%"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_PRESSURE,			"BME680_PRESS",		"Barometric pressure BME680",			0,		true,		false,		1,			"kPa"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_VOCS,			"BME680_VOCS",		"VOC Gas BME680",				0,		true,		false,		1,			"Ohms"},

			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_FIX_QUALITY,			"GPS_FIX",		"GPS Fix Quality",				128,		true,		false,		1,			},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_LATITUDE,			"GPS_LAT",		"GPS Latitude",					125,		true,		false,		1,			"Deg"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_LONGITUDE,			"GPS_LONG",		"GPS Longitude",				126,		true,		false,		1,			"Deg"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_ALTITUDE,			"GPS_ALT",		"GPS Altitude",					127,		true,		false,		1,			"m"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_SPEED,			"GPS_SPEED",		"GPS Speed",					129,		true,		false,		1,			"m/s"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_HDOP,			"GPS_HDOP",		"GPS Horizontal Dilution of Position",		131,		true,		false,		1,			},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_SATNUM,			"GPS_SATNUM",		"GPS Traked Satellites",			130,		true,		false,		1,			},

			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_0,			"ADC_48_0",		"ADS1x15 ADC 0x48 Ch0",				133,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_1,			"ADC_48_1",		"ADS1x15 ADC 0x48 Ch1",				134,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_2,			"ADC_48_2",		"ADS1x15 ADC 0x48 Ch2",				135,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_3,			"ADC_48_3",		"ADS1x15 ADC 0x48 Ch3",				136,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_0,			"ADC_49_0",		"ADS1x15 ADC 0x49 Ch0",				138,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_1,			"ADC_49_1",		"ADS1x15 ADC 0x49 Ch1",				139,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_2,			"ADC_49_2",		"ADS1x15 ADC 0x49 Ch2",				140,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_3,			"ADC_49_3",		"ADS1x15 ADC 0x49 Ch3",				141,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_0,			"ADC_4A_0",		"ADS1x15 ADC 0x4A Ch0",				143,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_1,			"ADC_4A_1",		"ADS1x15 ADC 0x4A Ch1",				144,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_2,			"ADC_4A_2",		"ADS1x15 ADC 0x4A Ch2",				145,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_3,			"ADC_4A_3",		"ADS1x15 ADC 0x4A Ch3",				146,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_0,			"ADC_4B_0",		"ADS1x15 ADC 0x4B Ch0",				148,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_1,			"ADC_4B_1",		"ADS1x15 ADC 0x4B Ch1",				149,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_2,			"ADC_4B_2",		"ADS1x15 ADC 0x4B Ch2",				150,		true,		true,		1,			"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_3,			"ADC_4B_3",		"ADS1x15 ADC 0x4B Ch3",				151,		true,		true,		1,			"V"},

			// Later this will be moved to a Actuators.h file
			// Groove I2C Oled Display 96x96
			OneSensor { BOARD_AUX,		250,	SENSOR_GROVE_OLED,			"GR_OLED",		"Groove OLED",					0,		true,		false,		1,			},
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
