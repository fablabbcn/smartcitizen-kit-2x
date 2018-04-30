#pragma once

struct SensorConfig {
	bool enabled;
	uint32_t interval;
};

// const char *modeTitles[MODE_COUNT] PROGMEM = {
// 	"setup",		// modeTitles[MODE_SETUP]
// 	"network",		// modeTitles[MODE_NET]
// 	"sdcard",		// modeTitles[MODE_SD]
// 	"sleep"			// modeTitles[MODE_SLEEP]
// };

const uint32_t serialBaudrate = 115200;
