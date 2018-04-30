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

const uint32_t serialBaudrate = 230400;

enum ESPMessage {

	ESPMES_SET_TOKEN,				// SAM->ESP, ESP sets new token and returns setted token. "null" parameter means clear token
	ESPMES_GET_TOKEN,				// SAM->ESP, Esp return current token.

	ESPMES_COUNT
};

enum SAMMessage {

	SAMMES_TOKEN,					// ESP -> SAM, Setted token on ESP


	SAMMES_COUNT	
};

enum NetPacks { PART_NUMBER = 0, TOTAL_PARTS = 1 };
#define NETPACK_TOTAL_SIZE 60
#define NETPACK_CONTENT_SIZE (NETPACK_TOTAL_SIZE - 2)	// 60 - 2 = 58 bytes

#define SAM_ADDRESS 1
#define ESP_ADDRESS 2
