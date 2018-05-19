#include "SckBase.h"

SckBase base;

// Led update interrupt
void TC5_Handler (void) {
	base.led.tick();
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}
// Button events interrupt
void ISR_button() {
	base.butState = digitalRead(pinBUTTON);
	base.butFeedback();
}
// Battery events interrupt
void ISR_battery() {
	base.batteryEvent();
}
// Card detect interrupt
void ISR_sdDetect() {
	base.sdDetect();
}
// Battery charger interrupt
void ISR_charger() {
	base.chargerEvent();
}
void TC3_Handler (void) {
	base.timerAlarm();
	TC3->COUNT16.INTFLAG.bit.MC0 = 1;
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

void serialEventRun(){
	base.inputUpdate();
}
