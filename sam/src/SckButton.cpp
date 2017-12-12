#include "SckBase.h"

void SckBase::buttonEvent() {

	buttonLastEvent = millis();
	buttonStage = BUT_STARTED;

	if (!digitalRead(pinBUTTON)) {

		// Button Down
		setAlarm_TC4(100);
		sckOut("Button Down", PRIO_LOW);

	} else {

		// Button Up
		setAlarm_TC4();
		sckOut("Button Up", PRIO_LOW);

	}
}
void SckBase::buttonStillDown() {

	uint32_t pressedTime = millis() - buttonLastEvent;

	if (pressedTime >= buttonLong && buttonStage == BUT_STARTED) {

		buttonStage = BUT_LONG_PRESS;
		sprintf(outBuff, "Button pressed for %lu milliseconds: Long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

	} else if (pressedTime >= buttonVeryLong && buttonStage == BUT_LONG_PRESS) {
	
		buttonStage = BUT_VERY_LONG;
		sprintf(outBuff, "Button pressed for %lu milliseconds: Very long press", millis() - buttonLastEvent);
		sckOut(PRIO_LOW);

	}
}
void SckBase::setAlarm_TC4(uint16_t lapse) {

	if(alarmRunning_TC4 || lapse == 0) {

		TC4->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		alarmRunning_TC4 = false;

	} else {

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

		TC4->COUNT16.CC[0].reg = uint32_t(46.875 * lapse);
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

		alarmRunning_TC4 = true;
	}
}