#include "SckBase.h"

#ifdef testing
#include "SckTest.h"
#endif

SckBase base;

#ifdef testing
SckTest sckTest(&base);
#endif

// Led update interrupt
void TC5_Handler (void) {
	base.led.tick();
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}
// Button events interrupt
void ISR_button() {
	base.butState = digitalRead(pinBUTTON);
	base.butFeedback();

#ifdef testing
	sckTest.test_button();
#endif
}
// Battery events interrupt
void ISR_battery() {
	base.battPendingEvent = true;
}
// Card detect interrupt
void ISR_sdDetect() {
	base.sdDetect();
}
// Battery charger interrupt
void ISR_charger() {
	base.chargerPendingEvent = true;
}
// void ISR_alarm() {
// 	base.wakeUp();
// };

void setup() {

#ifdef testing
	base.outputLevel = OUT_SILENT;
#endif

	base.setup();

#ifdef testing
	sckTest.test_full();
#endif
}

void loop() {
	base.update();
}

void serialEventRun(){
	base.inputUpdate();
}
