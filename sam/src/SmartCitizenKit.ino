#include "SckBase.h"

#ifdef testing
#include "SckTest.h"
#endif

SckBase base;

#ifdef testing
SckTest sckTest(&base);
#endif

bool reset_pending = false;

// Led update interrupt
void TC5_Handler (void) {
	base.led.tick();
	TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}
// Button events interrupt
void ISR_button() {
	base.butState = digitalRead(pinBUTTON);
#ifdef testing
	sckTest.test_button();
#else
	base.butFeedback();
#endif
}
// Card detect interrupt
void ISR_sdDetect() {
	base.sdDetect();
}
// void ISR_alarm() {
// 	base.wakeUp();
// };

void setup() {

	base.setup();

#ifdef testing
	sckTest.test_full();
#endif
}

void loop() {
	if (reset_pending) {
		if (base.rtc.getHours() == base.wakeUP_H && base.rtc.getMinutes() >= base.wakeUP_M && millis() > 82800000) base.sck_reset();
	}
	base.update();
}

void serialEventRun() {
	base.inputUpdate();
}
void ext_reset() {
	reset_pending = true;
}
