#include "sckBase.h"

SckBase base;

// Button interrupt handler
void ISR_button() {
	base.buttonEvent();
}
// void ISR_alarm() {
// 	base.wakeUp();
// };

// Timer 4 interrupt handler
void TC4_Handler (void) {
	base.buttonStillDown();
	TC4->COUNT16.INTFLAG.bit.MC0 = 1;
}
// Timer 5 interrupt handler
void TC5_Handler (void) {
	base.led.tick();
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

void setup() {
	base.setup();
}

void loop() {
	base.update();
}
