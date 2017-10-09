#include "sckBase.h"

SckBase base;

// Button interrupt handler
void ISR_button() {
	base.buttonEvent();
};
void ISR_alarm() {
	base.wakeUp();
};
// Timer 5 interrupt handler
void TC5_Handler (void) {
	base.led.tick();
    // Clear the interrupt
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
};

void setup() {

	base.setup();

}

uint32_t publish_timer = millis();

void loop() {

	// delay(10);
	// base.getReading(SENSOR_INA219_CURRENT);
	// base.sckOut(String(base.sensors[SENSOR_INA219_CURRENT].reading));

	base.update();
}
