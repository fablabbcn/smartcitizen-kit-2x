#include "SckBase.h"

SckBase base;

// Led update interrupt
void TC5_Handler (void) {
	base.led.tick();
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}
// Button events interrupt
void ISR_button() {
	base.buttonEvent();
}
// Card detect interrupt
void ISR_cardDetect() {
	base.cardDetect();
}
// Button alarm interrupt
void TC4_Handler (void) {
	base.buttonStillDown();
	TC4->COUNT16.INTFLAG.bit.MC0 = 1;
}
// void ISR_alarm() {
// 	base.wakeUp();
// };

void setup() {
	base.setup();
}

void loop() {
	base.update();
}
