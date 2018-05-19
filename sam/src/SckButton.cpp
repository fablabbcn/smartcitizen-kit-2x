#include "SckBase.h"

void SckBase::buttonEvent()
{
	buttonLastEvent = millis();

	if (!butState){
		// Button Down
		sckOut("Button Down", PRIO_LOW);

		if (state.sleeping) {
			
			// TODO wakeup
			state.sleeping = false;
			wakingUp = true;
		
		}


	} else {
		// Button Up
		sckOut("Button Up", PRIO_LOW);
		
		if (state.sleeping) {

			// TODO go to sleep
			
		} else if (state.onSetup || wakingUp) {
			
			state.onSetup = false;
			wakingUp = false;
			if (!sendMessage(ESPMES_STOP_AP, "")) ESPcontrol(ESP_REBOOT);
			if (state.mode == MODE_SD) led.update(led.PINK, led.PULSE_SOFT);	
			else { 
				if (state.mode == MODE_NOT_CONFIGURED) state.mode = MODE_NET;
				led.update(led.BLUE, led.PULSE_SOFT);	
			}

		} else enterSetup();
	}
}
void SckBase::buttonStillDown()
{
	uint32_t pressedTime = millis() - buttonLastEvent;

	if (pressedTime >= buttonLong && !state.sleeping) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

		state.sleeping = true;
		led.off();

		ESPcontrol(ESP_OFF);

	} else if (pressedTime >= buttonVeryLong && state.sleeping) {

		sprintf(outBuff, "Button pressed for %lu milliseconds: Very long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

		state.sleeping = false;

		// Factory defaults
		Configuration defaultConfig;
		saveConfig(defaultConfig);
		reset();
	}
}
void SckBase::butFeedback()
{
	if (!butState){
		if (state.sleeping) {
			if (state.mode == MODE_NET) led.update(led.BLUE2, led.PULSE_STATIC);
			else if (state.mode == MODE_SD) led.update(led.PINK2, led.PULSE_STATIC);
		} else if (state.onSetup) led.update(led.RED2, led.PULSE_STATIC);
	}
}
