#pragma once
#include "Sensors.h"

static const uint32_t minimal_publish_interval = 30;
static const uint32_t default_publish_interval = 30;
static const uint32_t max_publish_interval = 3600;		// One hour

enum SCKmodes {
	MODE_NOT_CONFIGURED,
	MODE_NET,
	MODE_SD,
	MODE_SLEEP,

	MODE_COUNT
};


struct SensorConfig { bool enabled; uint32_t interval; };
struct Credentials { bool set=false; char ssid[64]="null"; char pass[64]="null"; };
struct Token { bool set=false; char token[7]="null"; };

struct Configuration {
	bool valid = true;
	SCKmodes mode = MODE_NOT_CONFIGURED;				// This mode only changes on user configuration, it can only be MODE_SD or MODE_NET or MODE_NOT_CONFIGURED
	uint32_t publishInterval = default_publish_interval; 		// in seconds
	Credentials credentials;
	Token token;
	SensorConfig sensors[SENSOR_COUNT]; 				// Not used on ESP
};
