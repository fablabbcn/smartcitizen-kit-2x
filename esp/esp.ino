#include "sckESP.h"

SckESP esp;


void ledToggleLeft() {
	esp.ledToggle(esp.ledLeft);
}
void ledToggleRight() {
	esp.ledToggle(esp.ledRight);
}

void setup() {
  
	esp.setup();
	esp.start();

}

void loop() {


  // ts.update()
  
}