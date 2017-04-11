#include "sckBase.h"

SckBase base;

// Button interrupt handler
void ISR_button() {
	base.buttonEvent();
};
void ISR_alarm() {
	base.wakeUp();
}
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

	base.update();

	// If publish action takes to much time ()
	if (base.ESPon && millis() - base.espLastOn > (base.configuration.readInterval - 2) * 1000 && (base.mode == MODE_NET || base.mode == MODE_SD)) {
		base.sckOut(F("Network publish timeout!!!"));
		base.ESPcontrol(base.ESP_OFF);
		base.ESPpublishPending = false;
		bool platformPublishedOK = false;
		// Publish to SD
		base.publishToSD(platformPublishedOK);
	}

	// Publish
	if ((millis() - publish_timer) > base.configuration.readInterval * 1000) {

		base.sckOut(F("Time to publish..."), base.PRIO_LOW);
		publish_timer = millis();

		if (base.mode == MODE_NET || base.mode == MODE_SD) {

			base.sckOut(F("Starting publish..."));
			base.publish();

		} else {
			base.sckOut(F("Cancelled publish because we are not in the right mode!"), base.PRIO_LOW);
		}
	}
}
