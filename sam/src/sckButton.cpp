#include "sckBase.h"

void SckBase::buttonEvent() {

	butLastEvent = millis();
	butStage = BUT_STARTED;

	if (!digitalRead(pinBUTTON)) {

		// Button Down
		configureAlarm_TC4(100);
		sckOut("Button Down", PRIO_LOW);

		// 	switch (config.mode) {
	// 		case MODE_FLASH: {
	// 			softReset();
	// 			break;

	// 		} case MODE_SETUP: {
	// 			changeMode(config.persistentMode);
	// 			break;

	// 		} case MODE_OFF: {

	// 			wakeUp();
	// 			break;

	// 		} default: {
	// 			changeMode(MODE_SETUP);
	// 		}
	// 	}


	} else {

		// Button Up
		disableAlarm_TC4();
		sckOut("Button Up", PRIO_LOW);

		// 	if (config.mode == MODE_OFF) {
// 		sckOut("Sleeping!!");
// 		timerSet(ACTION_SLEEP, 500);
// 	}
	}
}
void SckBase::buttonStillDown() {

	uint32_t pressedTime = millis() - butLastEvent;

	if (pressedTime >= veryLongPressInterval && butStage == BUT_LONG_PRESS) veryLongPress();
	else if (pressedTime >= longPressInterval && butStage == BUT_STARTED) longPress();
}
void SckBase::longPress() {

	butStage = BUT_LONG_PRESS;
	sprintf(outBuff, "Button pressed for %lu milliseconds: Long press", millis() - butLastEvent);
	sckOut(PRIO_LOW);
	// changeMode(MODE_OFF);	
}
void SckBase::veryLongPress() {
	
	butStage = BUT_VERY_LONG;
	sprintf(outBuff, "Button pressed for %lu milliseconds: Very long press", millis() - butLastEvent);
	sckOut(PRIO_LOW);
	// factoryReset();
}
void SckBase::configureAlarm_TC4(uint16_t periodMS) {

	if (!alarmConfigured_TC4) {

		// Clock the timer with the core cpu clock (48MHz)
		GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
		while(GCLK->STATUS.bit.SYNCBUSY);

		// Reset the TC
		TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		while(TC4->COUNT16.CTRLA.bit.SWRST);

		// Set Timer counter Mode to 16 bits
		TC4->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

		// Set TC5 mode as match frequency
		TC4->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

		TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE;

		TC4->COUNT16.CC[0].reg = uint32_t(46.875 * periodMS);
		// TC4->COUNT16.CC[0].reg = uint16_t(95 * periodMS);

		// TC5->COUNT16.CC[0].reg = (uint16_t) 187; //0.5us = 240 clock cycle at 48MHz (core clock)
													// 4ms = 187 clock cycle at 46875
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		// Configure interrupt request
		NVIC_DisableIRQ(TC4_IRQn);
		NVIC_ClearPendingIRQ(TC4_IRQn);
		NVIC_SetPriority(TC4_IRQn, 2); //you can change priority between 0 (Highest priority) and 2 (lowest)
		NVIC_EnableIRQ(TC4_IRQn);

		// Enable the TC5 interrupt request
		TC4->COUNT16.INTENSET.bit.MC0 = 1;
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		//enable the counter (from now your getting interrupt)
		TC4->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);

		alarmConfigured_TC4 = true;
	}
}
void SckBase::disableAlarm_TC4() {

	if(alarmConfigured_TC4) {
		//use this to disable the counter :
		TC4->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		alarmConfigured_TC4 = false;
	}
}