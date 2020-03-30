#include "SckBase.h"

void SckBase::buttonEvent()
{
	if (!butState){
		// Button Down
		sckOut("Button Down", PRIO_LOW);

	} else {
		// Button Up
		sckOut("Button Up", PRIO_LOW);

		if (sckOFF) {
			
			goToSleep();

		} else if (st.sleeping) {

			if (st.mode == MODE_NOT_CONFIGURED) enterSetup();
			else if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_SOFT);
			else if (st.mode == MODE_NET) led.update(led.BLUE, led.PULSE_SOFT);
			st.sleeping = false;

		} else if (st.onSetup) {

			st.onSetup = false;
			if (st.mode == MODE_NET) {
				ESPcontrol(ESP_REBOOT);
				led.update(led.BLUE, led.PULSE_SOFT);
			} else if (st.mode == MODE_SD) {
				led.update(led.PINK, led.PULSE_SOFT);
			} else {
				if (st.mode == MODE_NOT_CONFIGURED) st.mode = MODE_NET;
				led.update(led.BLUE, led.PULSE_SOFT);
			}

		} else enterSetup();
	}
}
void SckBase::buttonStillDown()
{
	uint32_t pressedTime = millis() - lastUserEvent;

	if (pressedTime >= buttonVeryLong) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Very long press", pressedTime);
		sckOut(PRIO_LOW);

		sckOFF = false;

		// Factory defaults
		saveConfig(true);
		sck_reset();

	} else if (pressedTime >= buttonLong && !sckOFF) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Long press", pressedTime);
		sckOut(PRIO_LOW);

		sckOFF = true;
		led.off();

		ESPcontrol(ESP_OFF);
	}

}
void SckBase::butFeedback()
{
	lastUserEvent = millis();
	if (!butState){
		if (sckOFF) sck_reset();
		if (!st.onSetup) {
			if (st.mode == MODE_NET) led.update(led.BLUE2, led.PULSE_STATIC);
			else if (st.mode == MODE_SD) led.update(led.PINK2, led.PULSE_STATIC);
		} else if (st.onSetup) led.update(led.RED2, led.PULSE_STATIC);
	}
}
