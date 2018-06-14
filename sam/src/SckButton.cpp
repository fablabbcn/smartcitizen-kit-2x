#include "SckBase.h"

void SckBase::buttonEvent()
{
	buttonLastEvent = millis();

	if (!butState){
		// Button Down
		sckOut("Button Down", PRIO_LOW);

		if (st.sleeping) {

			// TODO wakeup
			st.sleeping = false;
			wakingUp = true;

		}


	} else {
		// Button Up
		sckOut("Button Up", PRIO_LOW);

		if (st.sleeping) {

			// TODO go to sleep

		} else if (st.onSetup || wakingUp) {

			st.onSetup = false;
			wakingUp = false;
			if (!sendMessage(ESPMES_STOP_AP, "")) ESPcontrol(ESP_REBOOT);
			if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_SOFT);
			else {
				if (st.mode == MODE_NOT_CONFIGURED) st.mode = MODE_NET;
				led.update(led.BLUE, led.PULSE_SOFT);
			}

		} else enterSetup();
	}
}
void SckBase::buttonStillDown()
{
	uint32_t pressedTime = millis() - buttonLastEvent;

	if (pressedTime >= buttonLong && !st.sleeping) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

		st.sleeping = true;
		led.off();

		ESPcontrol(ESP_OFF);

	} else if (pressedTime >= buttonVeryLong && st.sleeping) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Very long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

		st.sleeping = false;

		// Factory defaults
		saveConfig(true);
		reset();
	}
}
void SckBase::butFeedback()
{
	if (!butState){
		if (st.sleeping || !st.onSetup) {
			if (st.mode == MODE_NET) led.update(led.BLUE2, led.PULSE_STATIC);
			else if (st.mode == MODE_SD) led.update(led.PINK2, led.PULSE_STATIC);
		} else if (st.onSetup) led.update(led.RED2, led.PULSE_STATIC);
	}
}
