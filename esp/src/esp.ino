#include "sckESP.h"

SckESP esp;

void setup() {
	esp.setup();
}

void loop() {
	esp.update();
}


void LedToggleLeft() {
	esp.ledToggle(esp.ledLeft);
}
void LedToggleRight() {
	esp.ledToggle(esp.ledRight);
}
time_t ntpProvider() {
	return esp.getNtpTime();
}
void extRoot() {
	esp.webRoot();	
}
void extSet() {
	esp.webSet();
}
void extShow() {
	esp.webShow();	
}
