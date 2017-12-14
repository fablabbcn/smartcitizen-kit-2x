#pragma once

enum SCKmodes {
	MODE_SETUP,
	MODE_NET,
	MODE_SD,
	MODE_SLEEP,
	MODE_COUNT
};

// Serial Comunication
const uint32_t serialBaudrate = 115200;
