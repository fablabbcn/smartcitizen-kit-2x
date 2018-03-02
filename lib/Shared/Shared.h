#pragma once

enum SCKmodes {
	MODE_SETUP,
	MODE_NET,
	MODE_SD,
	MODE_SLEEP,
	MODE_COUNT
};

// const char *modeTitles[MODE_COUNT] PROGMEM = {
// 	"setup",		// modeTitles[MODE_SETUP]
// 	"network",		// modeTitles[MODE_NET]
// 	"sdcard",		// modeTitles[MODE_SD]
// 	"sleep"			// modeTitles[MODE_SLEEP]
// };

// Serial Comunication (2400 funciona)
const uint32_t serialBaudrate = 2400;
