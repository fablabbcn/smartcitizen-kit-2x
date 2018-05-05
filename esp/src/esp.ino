#include "SckESP.h"

SckESP esp;

void setup() {
	esp.setup();
}

void loop() {
	esp.update();
}


void ledToggle() {
	esp._ledToggle();
}

// time_t ntpProvider() {
// 	return esp.getNtpTime();
// }
// void extSet() {
// 	esp.webSet();
// }
// void extStatus() {
// 	esp.webStatus();
// }
