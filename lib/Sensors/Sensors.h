/* 	-----------------
 	|	 Sensors 	|
 	-----------------
*/

enum SensorType {

	SENSOR_TIME,
	SENSOR_NETWORKS,
	SENSOR_NOISE,
	SENSOR_HUMIDITY,
	SENSOR_TEMPERATURE,
	SENSOR_BATTERY,
	SENSOR_LIGHT,
	SENSOR_CO,
	SENSOR_NO2,
	SENSOR_VOLTIN,
	SENSOR_ALPHADELTA_AE1,
	SENSOR_ALPHADELTA_WE1,
	SENSOR_ALPHADELTA_AE2,
	SENSOR_ALPHADELTA_WE2,
	SENSOR_ALPHADELTA_AE3,
	SENSOR_ALPHADELTA_WE3,
	SENSOR_ALPHADELTA_TEMPERATURE,
	SENSOR_ALPHADELTA_HUMIDITY,

	SENSOR_COUNT	
};

struct SingleSensorData {
	float value;
	uint32_t lastReadingTime;
	bool valid = false;
};

class Sensors {
public:
	
	bool ready = false;

	String titles[SENSOR_COUNT] {
		"Time",
		"Wifi networks",
		"Noise",
		"Humidity",
		"Temperature",
		"Battery",
		"Light",
		"Carbon monoxide",
		"Nitrogen dioxide",
		"Input voltage",
		"AlphaDelta AE1",
		"AlphaDelta WE1",
		"AlphaDelta AE2",
		"AlphaDelta WE2",
		"AlphaDelta AE3",
		"AlphaDelta WE3",
		"AlphaDelta Temperature",
		"AlphaDelta Humidity"
	};
	


	String units[SENSOR_COUNT] {
		" ",
		"#",
		"dB",
		"%",
		"C",
		"mV",
		"Lux",
		"kOhm/ppm",
		"kOhm/ppm",
		"V",
		"U",			// Generic Units - for now
		"U",
		"U",
		"U",
		"U",
		"U",
		"C",
		"%"
	};

	// Number of seconds between readings
	uint16_t interval[SENSOR_COUNT] {
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	// Number of seconds between readings (0 = sensor disabled)
	bool enabled[SENSOR_COUNT] {
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false
	};

	
	SingleSensorData readings[SENSOR_COUNT];
private:
	
};