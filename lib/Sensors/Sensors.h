#pragma once

/* 	-----------------
 	|	 Sensors 	|
 	-----------------
*/
enum SensorLocation {
	BOARD_BASE,
	BOARD_URBAN,
	BOARD_AUX
};

enum SensorType {

	// Base Sensors
	SENSOR_TIME,
	SENSOR_BATTERY,
	SENSOR_VOLTIN,

	// Esp Sensors
	SENSOR_NETWORKS,

	// Urban Sensors
	SENSOR_NOISE,
	SENSOR_HUMIDITY,
	SENSOR_TEMPERATURE,
	SENSOR_LIGHT,
	SENSOR_CO,
	SENSOR_CO_HEAT_TIME,
	SENSOR_CO_HEAT_CURRENT,
	SENSOR_CO_HEAT_SUPPLY_VOLTAGE,
	SENSOR_CO_HEAT_DROP_VOLTAGE,
	SENSOR_CO_LOAD_RESISTANCE,
	SENSOR_NO2,
	SENSOR_NO2_HEAT_TIME,
	SENSOR_NO2_HEAT_CURRENT,
	SENSOR_NO2_HEAT_SUPPLY_VOLTAGE,
	SENSOR_NO2_HEAT_DROP_VOLTAGE,
	SENSOR_NO2_LOAD_RESISTANCE,

	// I2C Auxiliary Sensors
	SENSOR_ALPHADELTA_AE1,
	SENSOR_ALPHADELTA_WE1,
	SENSOR_ALPHADELTA_AE2,
	SENSOR_ALPHADELTA_WE2,
	SENSOR_ALPHADELTA_AE3,
	SENSOR_ALPHADELTA_WE3,
	SENSOR_ALPHADELTA_TEMPERATURE,
	SENSOR_ALPHADELTA_HUMIDITY,

	SENSOR_GROOVE_I2C_ADC,

	SENSOR_INA219_BUSVOLT,
	SENSOR_INA219_SHUNT,
	SENSOR_INA219_CURRENT,
	SENSOR_INA219_LOADVOLT,

	// Actuators (This is temp)
	SENSOR_GROOVE_OLED,

	SENSOR_COUNT	
};

class OneSensor {
public:
	SensorType type;
	String title;
	String unit;
	uint32_t interval;
	float reading;
	uint32_t lastReadingTime;
	bool enabled;
	bool valid;
	bool controllable;
	SensorLocation location;
	uint8_t id;


	OneSensor(SensorLocation nLocation, SensorType nType, String nTitle, uint8_t nId=0, bool nControllable=false, String nUnit="") {
		type = nType;
		title = nTitle;
		unit = nUnit;
		interval = 0;
		reading = 0;
		lastReadingTime = 0;
		enabled = false;
		valid = false;
		controllable = nControllable;
		location = nLocation;
		id = nId;
	}
};

class AllSensors {
public:
	
	OneSensor list[SENSOR_COUNT] {
	
		//			SensorLocation 		SensorType 							title 								id			controllable	unit 		
						
		// Base Sensors						
		OneSensor {	BOARD_BASE, 		SENSOR_TIME, 						"Time",									0,			false,			},
		OneSensor {	BOARD_BASE, 		SENSOR_BATTERY, 					"Battery", 								10,			false,			"%"},
		OneSensor {	BOARD_BASE, 		SENSOR_VOLTIN,						"Input voltage", 						0,			false,			"mV"},

		// ESP Sensors
		OneSensor {	BOARD_BASE, 		SENSOR_NETWORKS, 					"Wifi Networks",						21,			false,			},

		// Urban Sensors
		OneSensor {	BOARD_URBAN, 		SENSOR_NOISE, 						"Noise", 								29,			false,			"dBc"},
		OneSensor {	BOARD_URBAN, 		SENSOR_HUMIDITY,					"Humidity", 							13,			false,			"%"},
		OneSensor {	BOARD_URBAN, 		SENSOR_TEMPERATURE, 				"Temperature", 							12,			false,			"C"},
		OneSensor {	BOARD_URBAN, 		SENSOR_LIGHT, 						"Light", 								14,			false,			"Lux"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO, 							"Carbon monoxide", 						16,			true,			"kOhm/ppm"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO_HEAT_TIME, 				"Carbon monoxide heat time",			0,			false,			"sec"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO_HEAT_CURRENT, 			"Carbon monoxide heat current",			0,			false,			"mA"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO_HEAT_SUPPLY_VOLTAGE, 		"Carbon monoxide heat supply voltage",	0,			false,			"mV"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO_HEAT_DROP_VOLTAGE, 		"Carbon monoxide heat drop voltage",	0,			false,			"mV"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO_LOAD_RESISTANCE, 			"Carbon monoxide load resistance",		0,			false,			"Ohms"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2, 						"Nitrogen dioxide",						15,			true,			"kOhm/ppm"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2_HEAT_TIME, 				"Nitrogen dioxide heat time",			0,			false,			"sec"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2_HEAT_CURRENT, 			"Nitrogen dioxide heat current",		0,			false,			"mA"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2_HEAT_SUPPLY_VOLTAGE, 	"Nitrogen dioxide heat supply voltage",	0,			false,			"mV"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2_HEAT_DROP_VOLTAGE, 		"Nitrogen dioxide heat drop voltage",	0,			false,			"mV"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2_LOAD_RESISTANCE, 		"Nitrogen dioxide load resistance",		0,			false,			"Ohms"},

		// I2C Auxiliary Sensors
		// Alphasense Delta board (3 Gas sensor Slots, + SHT31 Temp-Humidity)
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE1, 				"AlphaDelta AE1",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE1, 				"AlphaDelta WE1",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE2, 				"AlphaDelta AE2",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE2, 				"AlphaDelta WE2",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE3, 				"AlphaDelta AE3",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE3, 				"AlphaDelta WE3",						0,			true,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_TEMPERATURE, 		"AlphaDelta Temperature", 				0,			false,			"C"},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_HUMIDITY, 		"AlphaDelta Humidity",					0,			false,			"%"},

		// Groove I2C ADC
		OneSensor { BOARD_AUX,			SENSOR_GROOVE_I2C_ADC,				"Groove ADC",							0,			false,			"V"},

		// Adafruit INA291 High Side DC Current Sensor
		OneSensor { BOARD_AUX,			SENSOR_INA219_BUSVOLT,				"ina219 busVoltage",					0,			false,			"V"},
		OneSensor { BOARD_AUX,			SENSOR_INA219_SHUNT,				"ina219 shuntVoltage",					0,			false,			"mV"},
		OneSensor { BOARD_AUX,			SENSOR_INA219_CURRENT,				"ina219 current",						0,			false,			"mA"},
		OneSensor { BOARD_AUX,			SENSOR_INA219_LOADVOLT,				"ina219 loadVoltage",					0,			false,			"V"},

		// Later this will be moved to a Actuators.h file
		// Groove I2C Oled Display 96x96
		OneSensor { BOARD_AUX,			SENSOR_GROOVE_OLED,					"Groove OLED",							0,			false,			}

		//-----------------------
		// Add New Sensor Here!!!

	};

	OneSensor & operator[](SensorType type) {
    	return list[type];
	}

private:
	
};