#pragma once

const uint32_t minimal_publish_interval = 60;
const uint32_t default_publish_interval = 60;
const uint32_t max_publish_interval = 86400;

struct Configuration {
	bool valid = false;
	SCKmodes mode;
	SCKmodes workingMode = MODE_NET;							// This mode only changes on user configuration, it can only be MODE_SD or MODE_NET
	uint32_t publishInterval = default_publish_interval; 		// in seconds
	// SensorConfig sensor[SENSOR_COUNT];
	char ssid[64];
	char pass[64];
	char token[8] = "null";
};
