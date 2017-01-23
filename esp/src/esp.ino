#include "sckESP.h"

SckESP esp;

void setup() {
  
	esp.setup();
	esp.start();

}

void loop() {

	// Serial.println(esp.fs_info.totalBytes);
	// Serial.println(esp.fs_info.blockSize);
	esp.update(); 
	// delay(2000);
}


void LedToggleLeft() {
	esp.ledToggle(esp.ledLeft);
}
void LedToggleRight() {
	esp.ledToggle(esp.ledRight);
}
