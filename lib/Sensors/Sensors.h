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
	SENSOR_NO2,

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
	SensorLocation location;
	uint8_t id;


	OneSensor(SensorLocation nLocation, SensorType nType, String nTitle, uint8_t nId=0, String nUnit="") {
		type = nType;
		title = nTitle;
		unit = nUnit;
		interval = 0;
		reading = 0;
		lastReadingTime = 0;
		enabled = false;
		valid = false;
		location = nLocation;
		id = nId;
	}
};

class AllSensors {
public:
	
	OneSensor list[SENSOR_COUNT] {
	
		//			SensorLocation 		SensorType 							title 						id			unit

		// Base Sensors
		OneSensor {	BOARD_BASE, 		SENSOR_TIME, 						"Time",						0,			},
		OneSensor {	BOARD_BASE, 		SENSOR_BATTERY, 					"Battery", 					10,			"%"},
		OneSensor {	BOARD_BASE, 		SENSOR_VOLTIN,						"Input voltage", 			0,			"mV"},

		// ESP Sensors
		OneSensor {	BOARD_BASE, 		SENSOR_NETWORKS, 					"Wifi Networks",			21,			},

		// Urban Sensors
		OneSensor {	BOARD_URBAN, 		SENSOR_NOISE, 						"Noise", 					29,			"dBc"},
		OneSensor {	BOARD_URBAN, 		SENSOR_HUMIDITY,					"Humidity", 				13,			"%"},
		OneSensor {	BOARD_URBAN, 		SENSOR_TEMPERATURE, 				"Temperature", 				12,			"C"},
		OneSensor {	BOARD_URBAN, 		SENSOR_LIGHT, 						"Light", 					14,			"Lux"},
		OneSensor {	BOARD_URBAN, 		SENSOR_CO, 							"Carbon monoxide", 			16,			"kOhm/ppm"},
		OneSensor {	BOARD_URBAN, 		SENSOR_NO2, 						"Nitrogen dioxide",			15,			"kOhm/ppm"},
		
		// I2C Auxiliary Sensors
		// Alphasense Delta board (3 Gas sensor Slots, + SHT31 Temp-Humidity)
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE1, 				"AlphaDelta AE1",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE1, 				"AlphaDelta WE1",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE2, 				"AlphaDelta AE2",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE2, 				"AlphaDelta WE2",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_AE3, 				"AlphaDelta AE3",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_WE3, 				"AlphaDelta WE3",			0,			},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_TEMPERATURE, 		"AlphaDelta Temperature", 	0,			"C"},
		OneSensor {	BOARD_AUX, 			SENSOR_ALPHADELTA_HUMIDITY, 		"AlphaDelta Humidity",		0,			"%"},

		// Groove I2C ADC
		OneSensor { BOARD_AUX,			SENSOR_GROOVE_I2C_ADC,				"Groove ADC",				0,			}

		//-----------------------
		// Add New Sensor Here!!!

	};

	OneSensor & operator[](SensorType type) {
    	return list[type];
	}

private:
	
};