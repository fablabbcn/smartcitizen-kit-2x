#include "SckESP.h"

SckESP esp;

void setup() {
	esp.setup();
}

void loop() {
	esp.update();
}


// void LedToggleLeft() {
// 	esp.ledToggle(esp.ledLeft);G
// }
// void LedToggleRight() {
// 	esp.ledToggle(esp.ledRight);
// }
// time_t ntpProvider() {
// 	return esp.getNtpTime();
// }
// void extSet() {
// 	esp.webSet();
// }
// void extStatus() {
// 	esp.webStatus();
// }
