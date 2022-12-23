#pragma once

#include <Arduino.h>
#include <Wire.h>

enum DeviceLocation
{
	BOARD_BASE,
	BOARD_URBAN,
	BOARD_AUX,

	BOARD_COUNT
};


class OneSensor
{// TODO reemplazar todas las variables que se guarden en el config y dejar esto meramente como un sensor sensorCatalog
	public:
		DeviceLocation location;
		uint8_t priority; 		// 0-249 lower first. 250 is reserved for sensor we don't want to read (actuators, ej. oled screen)
		MetricType type;
		const char *shortTitle;
		const char *title;
		const char *unit;
		String reading; // TODO remove this once Active Sensor is working
		uint32_t lastReadingTime;// TODO remove this once Active Sensor is working
		bool controllable;
		uint8_t id;
		uint8_t everyNint; 	 	// Read this sensor every N intervals (default 1)
		uint8_t defaultEveryNint;
		bool enabled;// TODO remove this once Active Sensor is working
		bool defaultEnabled;
		int16_t state; 		// -1:error on reading, 0:reading OK, >0:number of seconds until the reading is OK// TODO remove this once Active Sensor is working
		bool oled_display;// TODO remove this once Active Sensor is working

		OneSensor(DeviceLocation nLocation, uint8_t nPriority, MetricType nType, const char *nShortTitle, const char *nTitle, uint8_t nId=0, bool nEnabled=false, bool nControllable=false, uint8_t nEveryNint=1, const char *nUnit="", bool nOled_display=true) {
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

// Esto debe estar asociado a cada metrica pero cambia en runtime
// state
// lastReadingTime
// reading ? debería decidir si voy a mantenerme en String o paso a otro tipo y retornar directamente desde getReading.



class AllSensors
{
	public:


		OneSensor list[SENSOR_COUNT+1] {

			//	DeviceLocation			prio	MetricType						shortTitle		title										id		en		ctrl 	everyNintervals	unit

			// Base Sensors
			OneSensor { BOARD_BASE,		100,	SENSOR_BATT_PERCENT,			"BATT",			"Battery",									10,		true,	false,	1,	"%"},
			OneSensor { BOARD_BASE,		100,	SENSOR_BATT_VOLTAGE,			"BATT_VOLT",	"Battery voltage",							0,		false,	false,	1,	"V"},
			OneSensor { BOARD_BASE,		100,	SENSOR_SDCARD,					"SDCARD",		"SDcard present",							0,		false,	false,	1,	"Present"},

			// Urban Sensors
			OneSensor { BOARD_URBAN,	100,	SENSOR_LIGHT,					"LIGHT",		"Light",									14,		true,	false,	1,	"Lux"},
			OneSensor { BOARD_URBAN,	0,		SENSOR_TEMPERATURE,				"TEMP",			"Temperature",								55,		true,	false,	1,	"C"},
			OneSensor { BOARD_URBAN,	0,		SENSOR_HUMIDITY,				"HUM",			"Humidity",									56,		true,	false,	1,	"%"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_NOISE_DBA,				"NOISE_A",		"Noise dBA",								53,		true,	true,	1,	"dBA"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_NOISE_DBC,				"NOISE_B",		"Noise dBC",								0,		false,	true,	1,	"dBC"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_NOISE_DBZ,				"NOISE_Z",		"Noise dBZ",								0,		false,	true,	1,	"dB"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_NOISE_FFT,				"NOISE_FFT",	"Noise FFT",								0,		false,	true,	1,	},
			OneSensor { BOARD_URBAN,	100,	SENSOR_ALTITUDE,				"ALT",			"Altitude",									0,		false,	false,	1,	"M"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_PRESSURE,				"PRESS",		"Barometric pressure",						58,		true,	false,	1,	"kPa"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_PRESSURE_TEMP,			"PRESS_TEMP",	"Pressure internal temperature",			0,		false,	false,	1,	"C"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_CCS811_VOCS,				"CCS811_VOCS",	"VOC Gas CCS811",							113,	true,	true,	1,	"ppb"},
			OneSensor { BOARD_URBAN,	100,	SENSOR_CCS811_ECO2,				"CCS811_ECO2",	"eCO2 Gas CCS811",							112,	true,	true,	1,	"ppm"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_1,					"PM_1",			"PM 1.0",									89,		true,	true,	5,	"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_25,					"PM_25",		"PM 2.5",									87,		true,	true,	5,	"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PM_10,					"PM_10",		"PM 10.0",									88,		true,	true,	5,	"ug/m3"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_03,					"PN_03",		"PN 0.3",									0,		false,	true,	1,	"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_05,					"PN_05",		"PN 0.5",									0,		false,	true,	1,	"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_1,					"PN_1",			"PN 1.0",									0,		false,	true,	1,	"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_25,					"PN_25",		"PN 2.5",									0,		false,	true,	1,	"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_5,					"PN_5",			"PN 5.0",									0,		false,	true,	1,	"#/0.1l"},
			OneSensor { BOARD_URBAN,	240,	SENSOR_PN_10,					"PN_10",		"PN 10.0",									0,		false,	true,	1,	"#/0.1l"},

			// I2C Auxiliary Sensors
			// SCK Gases Board for Alphasense (3 Gas sensor Slots, + SHT31 Temp-Humidity)
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_1A,		"GB_1A",		"Gases Board 1A",							65,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_1W,		"GB_1W",		"Gases Board 1W",							64,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_2A,		"GB_2A",		"Gases Board 2A",							62,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_2W,		"GB_2W",		"Gases Board 2W",							61,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_3A,		"GB_3A",		"Gases Board 3A",							68,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_SLOT_3W,		"GB_3W",		"Gases Board 3W",							67,		true,	true,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_TEMPERATURE,	"GB_TEMP",		"Gases Board Temperature",					79,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GASESBOARD_HUMIDITY,		"GB_HUM",		"Gases Board Humidity",						80,		true,	false,	1,	"%"},

			// Groove I2C ADC
			OneSensor { BOARD_AUX,		100,	SENSOR_GROOVE_I2C_ADC,			"GR_ADC",		"Groove ADC",								25,		true,	false,	1,	"V"},

			// Adafruit INA291 High Side DC Current Sensor
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_BUSVOLT,			"INA_VBUS",		"INA219 Bus voltage",						0,		true,	false,	1,	"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_SHUNT,			"INA_VSHUNT",	"INA219 Shunt voltage",						0,		true,	false,	1,	"mV"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_CURRENT,			"INA_CURR",		"INA219 Current",							0,		true,	false,	1,	"mA"},
			OneSensor { BOARD_AUX,		100,	SENSOR_INA219_LOADVOLT,			"INA_VLOAD",	"INA219 Load voltage",						0,		true,	false,	1,	"V"},

			OneSensor { BOARD_AUX,		100,	SENSOR_WATER_TEMP_DS18B20,		"DS_WAT_TEMP",	"DS18B20 Water temperature",				42,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_TEMPERATURE,		"AS_TEMP",		"Atlas Temperature",						51,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_PH,				"AS_PH",		"Atlas PH",									43,		true,	true,	1,	"pH"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC,				"AS_COND",		"Atlas Conductivity",						45,		true,	true,	1,	"uS/cm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_TDS,			"AS_TDS",		"Atlas Total Dissolved Solids",				122,	true,	true,	1,	"ppm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_SAL,			"AS_SAL",		"Atlas Salinity",							123,	true,	true,	1,	"PSU(ppt)"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_EC_SG,				"AS_SG",		"Atlas Specific gravity",					46,		true,	true,	1,	},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO,				"AS_DO",		"Atlas Dissolved Oxygen",					48,		true,	true,	1,	"mg/L"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ATLAS_DO_SAT,			"AS_DO_SAT",	"Atlas DO Saturation",						49,		true,	true,	1,	"%"},

			// I2C Moisture Sensor (chirp)
			// https://github.com/Miceuz/i2c-moisture-sensor

			OneSensor { BOARD_AUX,		100,	SENSOR_CHIRP_MOISTURE_RAW,		"CHRP_MOIS_RAW",	"Soil Moisture Raw",					0,		true,	true,	1,	},
			OneSensor { BOARD_AUX,		100,	SENSOR_CHIRP_MOISTURE,			"CHRP_MOIS",		"Soil Moisture Percent",				50,		true,	true,	1,	"%"},
			OneSensor { BOARD_AUX,		100,	SENSOR_CHIRP_TEMPERATURE,		"CHRP_TEMP",		"Soil Temperature",						0,		true,	true,	1,	"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_CHIRP_LIGHT,	 			"CHRP_LIGHT",		"Soil Light",							0,		true,	true,	1,	},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_1,				"EXT_PM_A_1",		"Ext PM_A 1.0",							71,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_25,				"EXT_PM_A_25",		"Ext PM_A 2.5",							72,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PM_10,				"EXT_PM_A_10",		"Ext PM_A 10.0",						73,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_03,				"EXT_PN_A_03",		"Ext PN_A 0.3",							99,		false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_05,				"EXT_PN_A_05",		"Ext PN_A 0.5",							100,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_1,				"EXT_PN_A_1",		"Ext PN_A 1.0",							101,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_25,				"EXT_PN_A_25",		"Ext PN_A 2.5",							102,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_5,				"EXT_PN_A_5",		"Ext PN_A 5.0",							103,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_A_PN_10,				"EXT_PN_A_10",		"Ext PN_A 10.0",						104,	false,	false,	1,	"#/0.1l"},

			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_1,				"EXT_PM_B_1",		"Ext PM_B 1.0",							75,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_25,				"EXT_PM_B_25",		"Ext PM_B 2.5",							76,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PM_10,				"EXT_PM_B_10",		"Ext PM_B 10.0",						77,		true,	false,	1,	"ug/m3"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_03,				"EXT_PN_B_03",		"Ext PN_B 0.3",							105,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_05,				"EXT_PN_B_05",		"Ext PN_B 0.5",							106,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_1,				"EXT_PN_B_1",		"Ext PN_B 1.0",							107,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_25,				"EXT_PN_B_25",		"Ext PN_B 2.5",							108,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_5,				"EXT_PN_B_5",		"Ext PN_B 5.0",							109,	false,	false,	1,	"#/0.1l"},
			OneSensor { BOARD_AUX,		200,	SENSOR_EXT_B_PN_10,				"EXT_PN_B_10",		"Ext PN_B 10.0",						110,	false,	false,	1,	"#/0.1l"},

			OneSensor { BOARD_AUX,		0,		SENSOR_PM_DALLAS_TEMP,			"PM_DALLAS_TEMP",	"PM board Dallas Temperature",			96,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		0,		SENSOR_DALLAS_TEMP,				"DALLAS_TEMP",		"Direct Dallas Temperature",			96,		true,	false,	1,	"C"},

			OneSensor { BOARD_AUX,		0,		SENSOR_SHT31_TEMP,				"EXT_TEMP",			"Ext Temperature",						79,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		0,		SENSOR_SHT31_HUM,				"EXT_HUM",			"Ext Humidity",							80,		true,	false,	1,	"%"},
			OneSensor { BOARD_AUX,		0,		SENSOR_SHT35_TEMP,				"EXT_TEMP_SHT35",	"Ext SHT35 Temperature",				0,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		0,		SENSOR_SHT35_HUM,				"EXT_HUM_SHT35",	"Ext SHT35 Humidity",					0,		true,	false,	1,	"%"},

			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_LIGHT,				"EXT_RANGE_LIGHT",	"Ext Range Light",						0,		true,	false,	1,	"Lux"},
			OneSensor { BOARD_AUX,		100,	SENSOR_RANGE_DISTANCE,			"EXT_RANGE_DIST",	"Ext Range Distance",					98,		true,	false,	1,	"mm"},

			OneSensor { BOARD_AUX,		0,		SENSOR_BME680_TEMPERATURE,		"BME680_TEMP",		"Temperature BME680",					0,		true,	false,	1,	"C"},
			OneSensor { BOARD_AUX,		0,		SENSOR_BME680_HUMIDITY,			"BME680_HUM",		"Humidity BME680",						0,		true,	false,	1,	"%"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_PRESSURE,			"BME680_PRESS",		"Barometric pressure BME680",			0,		true,	false,	1,	"kPa"},
			OneSensor { BOARD_AUX,		100,	SENSOR_BME680_VOCS,				"BME680_VOCS",		"VOC Gas BME680",						0,		true,	false,	1,	"Ohms"},

			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_FIX_QUALITY,			"GPS_FIX",			"GPS Fix Quality",						128,	true,	false,	1,	},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_LATITUDE,			"GPS_LAT",			"GPS Latitude",							125,	true,	false,	1,	"Deg"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_LONGITUDE,			"GPS_LONG",			"GPS Longitude",						126,	true,	false,	1,	"Deg"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_ALTITUDE,			"GPS_ALT",			"GPS Altitude",							127,	true,	false,	1,	"m"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_SPEED,				"GPS_SPEED",		"GPS Speed",							129,	true,	false,	1,	"m/s"},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_HDOP,				"GPS_HDOP",			"GPS Horizontal Dilution of Position",	131,	true,	false,	1,	},
			OneSensor { BOARD_AUX,		100,	SENSOR_GPS_SATNUM,				"GPS_SATNUM",		"GPS Traked Satellites",				130,	true,	false,	1,	},

			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_0,			"ADC_48_0",			"ADS1x15 ADC 0x48 Ch0",					133,	true,	true,	1,	"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_1,			"ADC_48_1",			"ADS1x15 ADC 0x48 Ch1",					134,	true,	true,	1,	"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_2,			"ADC_48_2",			"ADS1x15 ADC 0x48 Ch2",					135,	true,	true,	1,	"V"},
			OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_3,			"ADC_48_3",			"ADS1x15 ADC 0x48 Ch3",					136,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_0,			"ADC_48_0",			"ADS1x15 ADC 0x48 Ch0",					133,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_1,			"ADC_48_1",			"ADS1x15 ADC 0x48 Ch1",					134,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_2,			"ADC_48_2",			"ADS1x15 ADC 0x48 Ch2",					135,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_48_3,			"ADC_48_3",			"ADS1x15 ADC 0x48 Ch3",					136,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_0,			"ADC_49_0",			"ADS1x15 ADC 0x49 Ch0",					138,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_1,			"ADC_49_1",			"ADS1x15 ADC 0x49 Ch1",					139,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_2,			"ADC_49_2",			"ADS1x15 ADC 0x49 Ch2",					140,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_49_3,			"ADC_49_3",			"ADS1x15 ADC 0x49 Ch3",					141,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_0,			"ADC_4A_0",			"ADS1x15 ADC 0x4A Ch0",					143,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_1,			"ADC_4A_1",			"ADS1x15 ADC 0x4A Ch1",					144,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_2,			"ADC_4A_2",			"ADS1x15 ADC 0x4A Ch2",					145,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4A_3,			"ADC_4A_3",			"ADS1x15 ADC 0x4A Ch3",					146,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_0,			"ADC_4B_0",			"ADS1x15 ADC 0x4B Ch0",					148,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_1,			"ADC_4B_1",			"ADS1x15 ADC 0x4B Ch1",					149,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_2,			"ADC_4B_2",			"ADS1x15 ADC 0x4B Ch2",					150,	true,	true,	1,	"V"},
			// OneSensor { BOARD_AUX,		100,	SENSOR_ADS1X15_4B_3,			"ADC_4B_3",			"ADS1x15 ADC 0x4B Ch3",					151,	true,	true,	1,	"V"},

			OneSensor { BOARD_AUX,		100,	SENSOR_SCD30_CO2,				"SCD30_CO2",		"SCD30 CO2",							158,	true,	true,	1,	"ppm"},
			OneSensor { BOARD_AUX,		100,	SENSOR_SCD30_TEMP,				"SCD30_TEMP",		"SCD30 Temperature",					160,	true,	true,	1,	"C"},
			OneSensor { BOARD_AUX,		100,	SENSOR_SCD30_HUM,				"SCD30_HUM",		"SCD30 Humidity",						161,	true,	true,	1,	"%"},

			// Later this will be moved to a Actuators.h file
			// Groove I2C Oled Display 96x96
			OneSensor { BOARD_AUX,		250,	SENSOR_GROVE_OLED,				"GR_OLED",			"Groove OLED",							0,		true,	false,	1,	},
			OneSensor { BOARD_BASE, 	0,		SENSOR_COUNT,					"NOT_FOUND",		"Not found",							0,		false,	false,	1,	}

			// Add New Sensor Here!!!

		};

		OneSensor & operator[](MetricType type) {
			return list[type];
		}

		OneSensor ordered(uint8_t place);
		MetricType getTypeFromString(String strIn);
		String removeSensorName(String strIn);
		MetricType sensorsPriorized(uint8_t index);
	private:
		uint8_t countMatchedWords(String baseString, String input);
		MetricType prioSortedList[SENSOR_COUNT+1];
		bool sorted = false;
};

#include <i2cBuses.h>
#include <Measurements.h>

class SckBase;

enum DeviceType 
{
	DEVICE_BH1730,

	// DEVICE_INA219,
	// DEVICE_VL6180,
	DEVICE_ADS1X15,
	DEVICE_SCD30,

	DEVICE_COUNT
};

// esto es lo que se debe guardar en flash para cada metric existente (se podria hacer solo cuando es diferente al default)
struct MetricConfig
{
	bool enabled = true;
	uint8_t priority = 100;
	uint8_t intervals = 1;
	uint8_t precision = 2;
	// bool oled_display -> esto se debe mover al Device de la OLED, por defecto todos se muestran y si manualmente se desahabilitan en el Device oled hay una disabledList
};

struct Metric
{
	const MetricType type;
	const char *name;
	const char *shortName;
	const char *unit;
	const uint8_t precision; // La precision es algo que puede variar, asi que debe ir en el config
	const uint8_t id; // TODO Resolver como se van a manejar los id de plataforma si hay multiples devices 
};

struct DeviceInfo
{
	const DeviceType type;
	const char * name;
	const byte addressList[8];
	const uint8_t providedNum;
	const Metric providedList[];
};

struct I2cBus
{
	TwoWire * _wire;
	const char * name;
};

extern TwoWire auxWire;

struct DeviceConfig
{
	bool debug = false;
};

class Device
{
	public:
		virtual bool start(TwoWire * _wire, byte address) = 0;
		// TODO en el stop llevar la cuenta de las metricas apagadas para ver si se apaga el device
		// se podria pasar un parametro al stop para que se le aplicara a todas las metricas... (asi se resolveria por ejemplo la cuestion de los grupos de PM's??)
		virtual bool stop() = 0;
		virtual int8_t getReading(Metric metric, char * buff) = 0; // returns: -1: error; 0: OK -> reading is in the buffer; >0: number of seconds until the reading is ready
		virtual void control(char * line) = 0;
		DeviceInfo * info;

		byte devAddr = 0;
		TwoWire * wireBus;
		uint8_t muxAddr = 0;
		uint8_t channel = 0;
};

#define READING_BUFF_SIZE 32

class Sensors
{
	public:
		Sensors();
		void detect();
		int8_t getReading(Device * dev, Metric metric);
		DeviceInfo * deviceCatalog[DEVICE_COUNT];

		static const uint8_t MAX_ACTIVE_SENSORS = 64;
		Device * activeList[MAX_ACTIVE_SENSORS];
		uint8_t total = 0;
		char readingBuff[READING_BUFF_SIZE];
	
	private:
		void selectMuxChannel(TwoWire * _wire, byte muxAddr, uint8_t channel);
		bool newDevice(TwoWire * _wire, DeviceInfo * info, byte address);
		const byte MUX_BASE_ADDRESS = 0x70;
		I2cBus buses[2] = { 
			{ &Wire, "Wire" }, 
			{ &auxWire, "AuxWire" }};
};

bool I2Cdetect(TwoWire * _Wire, byte address);
